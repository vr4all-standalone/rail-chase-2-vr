param()
$ErrorActionPreference='Stop'
$taskRoot='D:/VR4ALL-RailChase2-Build/private/startup-probe'
$taskJava='E:/.gradle/jdks/eclipse_adoptium-21-amd64-windows.2'
$taskSdk='E:/AndroidSdk'
$taskTools=Join-Path $taskSdk 'build-tools/36.0.0'
$taskAndroid=Join-Path $taskSdk 'platforms/android-36.1/android.jar'
$taskProject=Split-Path -Parent $PSScriptRoot
foreach($taskDirectory in @($taskRoot,(Join-Path $taskRoot 'classes'),(Join-Path $taskRoot 'dex'))) {
    New-Item -ItemType Directory -Path $taskDirectory -Force | Out-Null
}
$env:TEMP=$env:TMP='D:/VR4ALL-RailChase2-Build/build/test-temp'
$env:JAVA_HOME=$taskJava
@'
<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.vrlightgunhub.rchase2.startupcheck">
 <uses-sdk android:minSdkVersion="29" android:targetSdkVersion="36"/>
 <application android:label="Rail Chase 2 startup check" android:hasCode="true"/>
 <instrumentation android:name="com.vrlightgunhub.rchase2.startupcheck.StartupProbe" android:targetPackage="com.vrlightgunhub.rchase2"/>
</manifest>
'@ | Set-Content -LiteralPath (Join-Path $taskRoot 'AndroidManifest.xml') -Encoding utf8
& (Join-Path $taskJava 'bin/javac.exe') --release 11 -classpath $taskAndroid -d (Join-Path $taskRoot 'classes') (Join-Path $taskProject 'android/tests/StartupProbe.java')
if($LASTEXITCODE){throw 'Startup probe compilation failed'}
& (Join-Path $taskTools 'd8.bat') --lib $taskAndroid --min-api 29 --output (Join-Path $taskRoot 'dex') (Join-Path $taskRoot 'classes/com/vrlightgunhub/rchase2/startupcheck/StartupProbe.class')
if($LASTEXITCODE){throw 'Startup probe dex failed'}
& (Join-Path $taskTools 'aapt.exe') package -f -M (Join-Path $taskRoot 'AndroidManifest.xml') -I $taskAndroid -F (Join-Path $taskRoot 'unsigned.apk')
if($LASTEXITCODE){throw 'Startup probe manifest failed'}
Push-Location -LiteralPath (Join-Path $taskRoot 'dex')
try { & (Join-Path $taskTools 'aapt.exe') add (Join-Path $taskRoot 'unsigned.apk') classes.dex }
finally { Pop-Location }
if($LASTEXITCODE){throw 'Startup probe packaging failed'}
& (Join-Path $taskTools 'zipalign.exe') -f 4 (Join-Path $taskRoot 'unsigned.apk') (Join-Path $taskRoot 'aligned.apk')
if($LASTEXITCODE){throw 'Startup probe alignment failed'}
& (Join-Path $taskTools 'apksigner.bat') sign --ks D:/VR4ALL-RailChase2-Build/private/rchase2.keystore --ks-key-alias rchase2 --ks-pass pass:android --key-pass pass:android --out (Join-Path $taskRoot 'probe.apk') (Join-Path $taskRoot 'aligned.apk')
if($LASTEXITCODE){throw 'Startup probe signing failed'}
Write-Output (Join-Path $taskRoot 'probe.apk')
