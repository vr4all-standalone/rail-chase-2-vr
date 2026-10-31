"""Exercise the production hidden-launch adapter without a device or app install."""
from pathlib import Path
import os
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
APP = ROOT / 'android/app/src/main/java/com/vrlightgunhub/rchase2'
JDK = Path(os.environ.get('JAVA_HOME', 'E:/.gradle/jdks/eclipse_adoptium-21-amd64-windows.2'))


class FoveationExperimentTests(unittest.TestCase):
    def test_actual_forwarding_allowlist_and_medium_default_reset(self):
        with tempfile.TemporaryDirectory(prefix='rchase2-ffr-') as temporary:
            work = Path(temporary)
            stub = work / 'android/content/Intent.java'
            stub.parent.mkdir(parents=True)
            stub.write_text('''package android.content;
import java.util.HashMap;
public final class Intent {
  private final HashMap<String,Object> extras=new HashMap<>();
  public Intent putExtra(String name,String value){extras.put(name,value);return this;}
  public Intent putExtra(String name,int value){extras.put(name,value);return this;}
  public Intent putExtra(String name,boolean value){extras.put(name,value);return this;}
  public boolean hasExtra(String name){return extras.containsKey(name);}
  public String getStringExtra(String name){Object value=extras.get(name);return value instanceof String?(String)value:null;}
}''')
            harness = work / 'FoveationExperimentHarness.java'
            harness.write_text('''package com.vrlightgunhub.rchase2;
import android.content.Intent;
import java.util.HashMap;
public final class FoveationExperimentHarness {
  static void require(boolean condition){if(!condition)throw new AssertionError();}
  public static void main(String[] args){
    for(Intent malformed:new Intent[]{new Intent().putExtra("rchase2_ffr",7),new Intent().putExtra("rchase2_ffr",true),new Intent().putExtra("rchase2_ffr",(String)null)}){
      Intent output=new Intent();FoveationExperiment.forward(malformed,output);
      if(!"raw".equals(output.getStringExtra("rchase2_ffr")))throw new AssertionError("Present wrong-type/null extra must remain Raw when Android returns null");
    }
    for(String value:new String[]{"raw","none","medium","high","cycle","cycle-high","MEDIUM","HIGH","cycle-high "," medium","",null}){
      Intent input=new Intent().putExtra("rchase2_ffr",value),output=new Intent();
      FoveationExperiment.forward(input,output);
      String expected="none".equals(value)||"medium".equals(value)||"high".equals(value)||"cycle".equals(value)||"cycle-high".equals(value)?value:"raw";
      require(expected.equals(output.getStringExtra("rchase2_ffr")));
      HashMap<String,String> environment=new HashMap<>();
      environment.put("RCHASE2_FFR_EXPERIMENT","cycle-high");
      FoveationExperiment.configure(output,environment::put);
      require(expected.equals(environment.get("RCHASE2_FFR_EXPERIMENT")));
      FoveationExperiment.configure(new Intent(),environment::put);
      require("medium".equals(environment.get("RCHASE2_FFR_EXPERIMENT")));
    }
    Intent ordinary=new Intent();FoveationExperiment.forward(new Intent(),ordinary);
    require("medium".equals(ordinary.getStringExtra("rchase2_ffr")));
    require("medium".equals(FoveationExperiment.mode(null)));
    System.out.println("PASS FFR forwarding, allowlist and medium default reset");
  }
}''')
            compile_result = subprocess.run([str(JDK / 'bin/javac.exe'), '-d', str(work),
                str(stub), str(APP / 'FoveationExperiment.java'), str(harness)], capture_output=True, text=True)
            self.assertEqual(compile_result.returncode, 0, compile_result.stdout + compile_result.stderr)
            result = subprocess.run([str(JDK / 'bin/java.exe'), '-cp', str(work),
                'com.vrlightgunhub.rchase2.FoveationExperimentHarness'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('PASS FFR', result.stdout)
        self.assertIn('FoveationExperiment.forward(getIntent(),intent);', (APP / 'SetupActivity.java').read_text())
        self.assertIn('FoveationExperiment.configure(getIntent(),', (APP / 'GameActivity.java').read_text())


if __name__ == '__main__':
    unittest.main()
