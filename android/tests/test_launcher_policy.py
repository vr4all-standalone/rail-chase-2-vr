from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
JDK=Path('E:/.gradle/jdks/eclipse_adoptium-21-amd64-windows.2')

class LauncherPolicyTests(unittest.TestCase):
    def test_actual_policy_guards_validation_lifecycle_and_manual_play(self):
        source=ROOT/'android/app/src/main/java/com/vrlightgunhub/rchase2/ReadyLaunchPolicy.java'
        self.assertTrue(source.is_file(),'Rail Chase 2 still uses the old launch booleans')
        temporary=Path('D:/VR4ALL-RailChase2-Build/private/platform-tests');temporary.mkdir(parents=True,exist_ok=True)
        with tempfile.TemporaryDirectory(dir=temporary) as classes:
            compiled=subprocess.run([str(JDK/'bin/javac.exe'),'-encoding','UTF-8','-d',classes,
                str(source),str(ROOT/'android/tests/LauncherPolicyHarness.java')],capture_output=True,text=True)
            self.assertEqual(compiled.returncode,0,compiled.stdout+compiled.stderr)
            result=subprocess.run([str(JDK/'bin/java.exe'),'-cp',classes,
                'com.vrlightgunhub.rchase2.LauncherPolicyHarness'],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)
            self.assertIn('PASS launcher readiness',result.stdout)
