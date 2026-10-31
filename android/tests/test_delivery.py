"""Software-only Android and PCVR delivery regressions; no game data required."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import warnings
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
JAVA = ROOT/'android/app/src/main/java/com/vrlightgunhub/rchase2/GameFiles.java'
JDK = Path(os.environ.get('JAVA_HOME', 'E:/.gradle/jdks/eclipse_adoptium-21-amd64-windows.2'))
TEMP = Path('D:/VR4ALL-RailChase2-Build/private/platform-tests')
ANDROID = '{http://schemas.android.com/apk/res/android}'


def load_package():
    spec = importlib.util.spec_from_file_location('rchase2_package', ROOT/'tools/package.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class DeliveryTests(unittest.TestCase):
    def test_preparation_window_uses_the_approved_wide_panel(self):
        app=ET.parse(ROOT/'android/app/src/main/AndroidManifest.xml').find('application')
        setup=next(a for a in app.findall('activity') if a.get(ANDROID+'name')=='.SetupActivity')
        layout=setup.find('layout')
        self.assertIsNotNone(layout,'Preparation window has the old narrow default')
        self.assertEqual(layout.get(ANDROID+'defaultWidth'),'860dp')
        self.assertEqual(layout.get(ANDROID+'defaultHeight'),'640dp')
        self.assertEqual(setup.get(ANDROID+'resizeableActivity'),'true')

    def test_setup_and_immersive_tasks_have_independent_identity(self):
        path = ROOT/'android/app/src/main/AndroidManifest.xml'
        self.assertTrue(path.is_file(), 'Rail Chase 2 Android manifest is missing')
        app = ET.parse(path).find('application')
        self.assertEqual(app.get(ANDROID+'label'), 'Rail Chase 2 - VR4ALL')
        activities = {item.get(ANDROID+'name'): item for item in app.findall('activity')}
        for name, category in [('.SetupActivity', 'com.oculus.intent.category.2D'), ('.GameActivity', 'org.khronos.openxr.intent.category.IMMERSIVE_HMD')]:
            self.assertIn(category, {item.get(ANDROID+'name') for item in activities[name].findall('intent-filter/category')})
        self.assertEqual(activities['.GameActivity'].get(ANDROID+'taskAffinity'), 'com.vrlightgunhub.rchase2.vr')

    def test_package_rejects_media_captures_and_keys(self):
        self.assertTrue((ROOT/'tools/package.py').is_file(), 'Rail Chase 2 package validator is missing')
        package = load_package()
        for name in ['roms/rchase2.zip', 'assets/epr-18022.ic2', 'assets/mpr-18037.11', 'roms/segabill.zip',
                     'private/a.keystore', 'keys/a.jks', 'userdata/settings.ini', 'captures/frame.png',
                     'scenes/1/metadata.json', 'assets/texture.ram', '../escape', '/escape', 'C:\\escape', 'assets/a.chd']:
            with self.subTest(name=name), self.assertRaises(ValueError): package.check_names([name])
        with self.assertRaises(ValueError): package.check_names(['assets/font.ttf', 'assets/./font.ttf'])

    def test_apk_requires_arm64_host_and_complete_font(self):
        self.assertTrue((ROOT/'tools/package.py').is_file(), 'Rail Chase 2 package validator is missing')
        package = load_package()
        TEMP.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            path = Path(directory)/'test.apk'
            def write(machine=183, omitted=None, notices=True, soundtouch=True):
                with zipfile.ZipFile(path, 'w') as archive:
                    for name in package.REQUIRED_ANDROID_LIBS:
                        if name == omitted: continue
                        header = bytearray(64); header[:7] = b'\x7fELF\x02\x01\x01'
                        struct.pack_into('<H', header, 18, machine)
                        archive.writestr('lib/arm64-v8a/'+name, header)
                    archive.writestr('AndroidManifest.xml', b'manifest')
                    archive.writestr('assets/fonts/Roboto-Bold.ttf', b'\0\1\0\0'+bytes(20000))
                    if notices:
                        archive.writestr('assets/notices/LICENSE', b'software license fixture')
                        archive.writestr('assets/notices/CREDITS.md', b'software attribution fixture')
                        archive.writestr('assets/notices/licenses/SDL.txt', b'SDL license fixture')
                        if soundtouch:
                            archive.writestr('assets/notices/licenses/SoundTouch.txt', b'SoundTouch license fixture')
            write()
            report = package.check_apk(path)
            self.assertFalse(report['game_data_included'])
            self.assertEqual(report['physical_headset_validation'], 'untested')
            write(machine=62)
            with self.assertRaisesRegex(ValueError, 'ABI'): package.check_apk(path)
            write(omitted='librchase2_xr.so')
            with self.assertRaisesRegex(ValueError, 'Missing'): package.check_apk(path)
            write(omitted='librchase2_audio.so')
            with self.assertRaisesRegex(ValueError, 'Missing'): package.check_apk(path)
            write(soundtouch=False)
            with self.assertRaisesRegex(ValueError, 'notices'): package.check_apk(path)
            write(notices=False)
            with self.assertRaisesRegex(ValueError, 'notices'): package.check_apk(path)

    def test_windows_requires_complete_amd64_components(self):
        self.assertTrue((ROOT/'tools/package.py').is_file(), 'Rail Chase 2 package validator is missing')
        package = load_package()
        TEMP.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            path = Path(directory)/'test.zip'
            def write(machine=0x8664, omitted=None):
                with zipfile.ZipFile(path, 'w') as archive:
                    for name in package.REQUIRED_WINDOWS:
                        if name == omitted: continue
                        header = bytearray(70); header[:2] = b'MZ'
                        struct.pack_into('<I', header, 60, 64); header[64:68] = b'PE\0\0'
                        struct.pack_into('<H', header, 68, machine)
                        archive.writestr('rail-chase-2-vr-pcvr/'+name, header)
            write()
            self.assertEqual(package.check_windows(path)['native_abi'], 'x86_64')
            write(machine=0x14c)
            with self.assertRaisesRegex(ValueError, 'ABI'): package.check_windows(path)
            write(omitted='rchase2_xr.dll')
            with self.assertRaisesRegex(ValueError, 'Missing'): package.check_windows(path)

    def test_package_output_stays_in_own_private_build_tree(self):
        package = load_package()
        for path in [ROOT/'build/delivery', Path('D:/VR4ALL-Vcop2-Build/artifacts'), Path('D:/VR4ALL-RailChase2-Build')]:
            with self.subTest(path=path), self.assertRaises(ValueError): package.owned_output(path)
        self.assertEqual(package.owned_output(Path('D:/VR4ALL-RailChase2-Build/artifacts')), Path('D:/VR4ALL-RailChase2-Build/artifacts'))

    def test_windows_package_requires_runtime_dependencies_and_loader_export(self):
        package = load_package()
        TEMP.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            directory = Path(directory)
            binaries = []
            for name in ['rchase2.exe', 'rchase2_xr.dll', 'libopenxr_loader.dll', 'rchase2_audio.dll']:
                path = directory/name
                header = bytearray(70); header[:2] = b'MZ'; struct.pack_into('<I', header, 60, 64)
                header[64:68] = b'PE\0\0'; struct.pack_into('<H', header, 68, 0x8664)
                path.write_bytes(header); binaries.append(path)
            for failure in ['missing_runtime', 'missing_loader_export']:
                def inspect(args, **kwargs):
                    option, source = args[1], Path(args[2])
                    text = ''
                    if option == '--coff-imports' and source.name == 'rchase2_xr.dll' and failure == 'missing_runtime':
                        text = 'Import {\n  Name: missing-runtime.dll\n}\n'
                    elif option == '--coff-exports' and source.name == 'rchase2_audio.dll':
                        text = '\n'.join('  Name: rchase2_audio_'+name for name in ('create','push','read','pause','stats','destroy'))
                    elif option == '--coff-exports' and source.name == 'rchase2_xr.dll':
                        text = '\n'.join('  Name: '+name for name in ['rchase2_xr_start', 'rchase2_xr_stop'])
                    elif option == '--coff-exports' and source.name == 'libopenxr_loader.dll' and failure != 'missing_loader_export':
                        text = '  Name: xrGetInstanceProcAddr\n'
                    return subprocess.CompletedProcess(args, 0, stdout=text)
                with self.subTest(failure=failure), patch.object(package.subprocess, 'run', side_effect=inspect):
                    with self.assertRaisesRegex(ValueError, 'dependency|export'):
                        package.windows(*binaries[:3], directory, directory/'llvm-readobj.exe', directory/'output')

    def test_windows_package_normalizes_dynamic_loader_and_copies_runtime(self):
        package = load_package()
        TEMP.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            directory = Path(directory); binaries = []
            for name in ['rchase2.exe', 'rchase2_xr.dll', 'libopenxr_loader.dll', 'fixture-runtime.dll', 'rchase2_audio.dll']:
                path = directory/name
                header = bytearray(70); header[:2] = b'MZ'; struct.pack_into('<I', header, 60, 64)
                header[64:68] = b'PE\0\0'; struct.pack_into('<H', header, 68, 0x8664)
                path.write_bytes(header); binaries.append(path)
            def inspect(args, **kwargs):
                option, source = args[1], Path(args[2]); text = ''
                if option == '--coff-imports' and source.name == 'rchase2_xr.dll':
                    text = 'Import {\n  Name: fixture-runtime.dll\n}\nImport {\n  Name: KERNEL32.dll\n}\n'
                elif option == '--coff-exports' and source.name == 'rchase2_audio.dll':
                    text = '\n'.join('  Name: rchase2_audio_'+name for name in ('create','push','read','pause','stats','destroy'))
                elif option == '--coff-exports' and source.name == 'rchase2_xr.dll':
                    text = '\n'.join('  Name: '+name for name in ['rchase2_xr_start', 'rchase2_xr_stop'])
                elif option == '--coff-exports' and source.name == 'libopenxr_loader.dll':
                    text = '  Name: xrGetInstanceProcAddr\n'
                return subprocess.CompletedProcess(args, 0, stdout=text)
            with patch.object(package.subprocess, 'run', side_effect=inspect):
                archive = package.windows(*binaries[:3], directory, directory/'llvm-readobj.exe', directory/'output')
            with zipfile.ZipFile(archive) as output:
                names = set(output.namelist())
                self.assertIn('rail-chase-2-vr-pcvr/openxr_loader.dll', names)
                self.assertNotIn('rail-chase-2-vr-pcvr/libopenxr_loader.dll', names)
                self.assertIn('rail-chase-2-vr-pcvr/fixture-runtime.dll', names)
                self.assertIn('rail-chase-2-vr-pcvr/rchase2_audio.dll', names)
                for document in ['design.md', 'implementation-plan.md', 'native-state-evidence.md', 'validation.md']:
                    self.assertIn('rail-chase-2-vr-pcvr/docs/'+document, names)

    def test_windows_launcher_reports_failed_native_session(self):
        compiler = Path('D:/msys64/mingw64/bin/gcc.exe')
        shell = Path('C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe')
        if not compiler.is_file() or not shell.is_file(): self.skipTest('Windows launcher test dependencies unavailable')
        TEMP.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            directory = Path(directory)
            source = directory/'fixture.c'
            source.write_text('#include <string.h>\nint main(int n,char** v){for(int i=1;i<n;i++)if(!strcmp(v[i],"-verifyroms"))return 0;for(int i=1;i+1<n;i++)if(!strcmp(v[i],"-sound")&&!strcmp(v[i+1],"sdl"))return 17;return 19;}\n')
            environment = {**os.environ, 'TEMP':str(directory), 'TMP':str(directory),
                           'PATH':str(compiler.parent)+os.pathsep+os.environ.get('PATH', '')}
            compiled = subprocess.run([str(compiler), str(source), '-o', str(directory/'rchase2.exe')], capture_output=True, text=True, env=environment)
            self.assertEqual(compiled.returncode, 0, compiled.stderr)
            shutil.copy2(ROOT/'tools/launch-pcvr.ps1', directory/'launch-pcvr.ps1')
            (directory/'roms').mkdir(); (directory/'roms/rchase2.zip').write_bytes(b'synthetic fixture')
            result = subprocess.run([str(shell), '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(directory/'launch-pcvr.ps1')], text=True, capture_output=True, env=environment)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('17', result.stderr)


class JavaMediaTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not JAVA.is_file(): return
        if not (JDK/'bin/javac.exe').is_file(): raise unittest.SkipTest('JDK unavailable')
        TEMP.mkdir(parents=True, exist_ok=True)
        cls.temp = tempfile.TemporaryDirectory(dir=TEMP)
        cls.classes = Path(cls.temp.name)/'classes'
        cls.classes.mkdir()
        subprocess.run([str(JDK/'bin/javac.exe'), '-d', str(cls.classes), str(JAVA), str(ROOT/'android/tests/GameFilesHarness.java')], check=True, capture_output=True, text=True)

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, 'temp'): cls.temp.cleanup()

    def test_pinned_23_payloads(self):
        self.assertTrue(JAVA.is_file(), 'Rail Chase 2 Java media validator is missing')
        source = JAVA.read_text()
        required = json.loads((ROOT/'game-set.json').read_text())['files']
        self.assertEqual(len(required), 23)
        for item in required:
            self.assertIn('new Rom("%s", %d, 0x%sL, "%s")' % (item['name'], item['size'], item['crc32'], item['sha1']), source)

    def test_missing_archive_reports_the_selected_folder(self):
        path=TEMP/'app-owned/roms/rchase2.zip'
        result=subprocess.run([str(JDK/'bin/java.exe'),'-cp',str(self.classes),
            'com.vrlightgunhub.rchase2.GameFilesHarness','--missing',str(path)],text=True,capture_output=True)
        self.assertNotEqual(result.returncode,0)
        self.assertIn(str(path),result.stderr)

    def test_split_complete_and_invalid_archives(self):
        self.assertTrue(JAVA.is_file(), 'Rail Chase 2 Java media validator is missing')
        if not hasattr(self, 'classes'): self.skipTest('JDK unavailable')
        def run(paths):
            return subprocess.run([str(JDK/'bin/java.exe'), '-cp', str(self.classes), 'com.vrlightgunhub.rchase2.GameFilesHarness', *map(str, paths)], text=True, capture_output=True)
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            def archive(name, entries):
                path = Path(directory)/name
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', UserWarning)
                    with zipfile.ZipFile(path, 'w') as output:
                        for key, value in entries: output.writestr(key, value)
                return path
            game = archive('rchase2.zip', [('game.rom', b'abc')]); board = archive('segabill.zip', [('board.rom', b'abc')])
            self.assertEqual(run([game, board]).returncode, 0)
            self.assertEqual(run([archive('complete.zip', [('game.rom', b'abc'), ('board.rom', b'abc')])]).returncode, 0)
            for entries, error in [([('game.rom', b'abc')], 'Missing'), ([('game.rom', b'xyz')], 'checksum'),
                                   ([('game.rom', b'abc'), ('GAME.ROM', b'abc')], 'Duplicate'), ([('../game.rom', b'abc')], 'Unsafe'),
                                   ([('game.rom', b'abcd')], 'size')]:
                result = run([archive('invalid.zip', entries)])
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(error.lower(), result.stderr.lower())

    def test_encrypted_and_truncated_archives(self):
        self.assertTrue(JAVA.is_file(), 'Rail Chase 2 Java media validator is missing')
        if not hasattr(self, 'classes'): self.skipTest('JDK unavailable')
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            path = Path(directory)/'invalid.zip'
            with zipfile.ZipFile(path, 'w') as output:
                output.writestr('game.rom', b'abc'); output.writestr('board.rom', b'abc')
            data = bytearray(path.read_bytes())
            for signature, offset in [(b'PK\x03\x04', 6), (b'PK\x01\x02', 8)]:
                start = 0
                while (start := data.find(signature, start)) >= 0:
                    data[start+offset] |= 1; start += 4
            for payload in [data, data[:20]]:
                path.write_bytes(payload)
                result = subprocess.run([str(JDK/'bin/java.exe'), '-cp', str(self.classes), 'com.vrlightgunhub.rchase2.GameFilesHarness', str(path)], text=True, capture_output=True)
                self.assertNotEqual(result.returncode, 0)


if __name__ == '__main__': unittest.main()
