from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
JAVA=ROOT/'android/app/src/main/java/com/vrlightgunhub/rchase2/GameStorage.java'
JDK=Path('E:/.gradle/jdks/eclipse_adoptium-21-amd64-windows.2')
TEMP=Path('D:/VR4ALL-RailChase2-Build/private/platform-tests')

class ScopedStorageTests(unittest.TestCase):
    def test_owned_folder_without_shared_storage_permission(self):
        self.assertTrue(JAVA.is_file(), 'App-owned game folder selector is missing')
        TEMP.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(dir=TEMP) as directory:
            classes=Path(directory)/'classes'; classes.mkdir()
            compiled=subprocess.run([str(JDK/'bin/javac.exe'),'-d',str(classes),str(JAVA),
                str(ROOT/'android/tests/GameStorageHarness.java')],capture_output=True,text=True)
            self.assertEqual(compiled.returncode,0,compiled.stderr)
            result=subprocess.run([str(JDK/'bin/java.exe'),'-cp',str(classes),
                'com.vrlightgunhub.rchase2.GameStorageHarness',directory],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__=='__main__': unittest.main()
