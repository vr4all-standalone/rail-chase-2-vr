param(
    [string]$MsysRoot='D:/msys64',
    [string]$PythonPath='D:/Python314/python.exe',
    [string]$WorkRoot,
    [string]$WorkEngine='D:/VR4ALL-RailChase2-Build/engine',
    [string]$AndroidSdk,
    [string]$JavaHome,
    [string]$CMakePath,
    [string]$NinjaPath,
    [string]$SdlRoot,
    [string]$SdlSource,
    [string]$OpenXRLoader,
    [string]$OpenXRSource,
    [string]$EngineLibrary,
    [string]$HostLibrary,
    [ValidateRange(1,32)][int]$Jobs=2,
    [switch]$Offline,
    [switch]$CheckOnly,
    [switch]$SkipEngineBuild,
    [switch]$SkipHostBuild
)
$ErrorActionPreference='Stop'
$taskProject=Split-Path -Parent $PSScriptRoot
$taskWorkspace=Split-Path -Parent (Split-Path -Parent $taskProject)
$taskCacheItem=Get-Item -LiteralPath (Join-Path $taskWorkspace '.cache') -ErrorAction SilentlyContinue
$taskCache=if($taskCacheItem -and $taskCacheItem.Target){$taskCacheItem.Target}else{Join-Path $taskWorkspace '.cache'}
if(!$WorkRoot){$WorkRoot='D:/VR4ALL-RailChase2-Build/quest'}
$taskWork=[IO.Path]::GetFullPath($WorkRoot)
function Assert-OwnedPath([string]$path,[string]$expected) {
    $taskResolved=[IO.Path]::GetFullPath($path).TrimEnd('\','/')
    if($taskResolved -ne [IO.Path]::GetFullPath($expected).TrimEnd('\','/')) {throw "RailChase2 owns only $expected for this build stage."}
    for($taskAncestor=$taskResolved;$taskAncestor;$taskAncestor=Split-Path -Parent $taskAncestor) {
        if((Test-Path -LiteralPath $taskAncestor) -and ((Get-Item -LiteralPath $taskAncestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {throw 'Private build paths must not traverse junctions or symbolic links.'}
        if($taskAncestor -eq [IO.Path]::GetPathRoot($taskAncestor)){break}
    }
}
Assert-OwnedPath $taskWork 'D:/VR4ALL-RailChase2-Build/quest'
Assert-OwnedPath $WorkEngine 'D:/VR4ALL-RailChase2-Build/engine'
$taskHostBuild='D:/VR4ALL-RailChase2-Build/host-android'
$taskGradleHome='D:/VR4ALL-RailChase2-Build/gradle-user-home'
$taskPrivate='D:/VR4ALL-RailChase2-Build/private'
Assert-OwnedPath $taskHostBuild 'D:/VR4ALL-RailChase2-Build/host-android'
Assert-OwnedPath $taskGradleHome 'D:/VR4ALL-RailChase2-Build/gradle-user-home'
Assert-OwnedPath $taskPrivate 'D:/VR4ALL-RailChase2-Build/private'
if(!$AndroidSdk){
    $taskProperties=Join-Path $taskProject 'android/local.properties'
    $taskSdkLine=if(Test-Path -LiteralPath $taskProperties){Get-Content -LiteralPath $taskProperties | Where-Object {$_ -match '^sdk\.dir='} | Select-Object -First 1}
    $AndroidSdk=if($taskSdkLine){$taskSdkLine.Substring(8).Replace('\\', '\')}elseif($env:ANDROID_HOME){$env:ANDROID_HOME}else{'E:/AndroidSdk'}
}
if(!$JavaHome){$JavaHome=if($env:JAVA_HOME){$env:JAVA_HOME}else{'E:/.gradle/jdks/eclipse_adoptium-21-amd64-windows.2'}}
if(!$CMakePath){$CMakePath='C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe';if(!(Test-Path -LiteralPath $CMakePath)){$CMakePath=Join-Path $AndroidSdk 'cmake/3.22.1/bin/cmake.exe'}}
if(!$NinjaPath){$NinjaPath=Join-Path $AndroidSdk 'cmake/3.22.1/bin/ninja.exe'}
if(!$SdlRoot){$SdlRoot=Join-Path $taskCache 'carnevil-port/sdl-android'}
if(!$OpenXRLoader){$OpenXRLoader=Join-Path $taskCache 'carnevil-port/openxr-android/src/loader/libopenxr_loader.so'}
if(!$SdlSource){$SdlSource='<local-development-tree>/death-crimson-ox-vr/core/deps/SDL'}
if(!$OpenXRSource){$OpenXRSource='<local-development-tree>/death-crimson-ox-vr/build-win/_deps/openxr_sdk-src'}
if(!$EngineLibrary){$EngineLibrary=Join-Path $WorkEngine 'android-project/app/src/main/libs/arm64-v8a/libmain.so'}
if(!$HostLibrary){$HostLibrary=Join-Path $taskHostBuild 'librchase2_xr.so'}
$taskNdk=Join-Path $AndroidSdk 'ndk/29.0.14206865'
$taskSigner=Join-Path $AndroidSdk 'build-tools/36.0.0/apksigner.bat'
$taskNm=Join-Path $taskNdk 'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-nm.exe'
foreach($taskFile in @($CMakePath,$NinjaPath,$PythonPath,(Join-Path $JavaHome 'bin/java.exe'),(Join-Path $JavaHome 'bin/javac.exe'),(Join-Path $MsysRoot 'usr/bin/make.exe'),(Join-Path $taskNdk 'build/cmake/android.toolchain.cmake'),(Join-Path $AndroidSdk 'platforms/android-36.1/android.jar'),$taskSigner,$taskNm)){
    if(!(Test-Path -LiteralPath $taskFile -PathType Leaf)){throw "Missing build dependency: $taskFile"}
}
if($CheckOnly){
    [ordered]@{project=$taskProject;work=$taskWork;work_engine=$WorkEngine;host_build=$taskHostBuild;gradle_user_home=$taskGradleHome;android_sdk=$AndroidSdk;ndk=$taskNdk;java=$JavaHome;cmake=$CMakePath;ninja=$NinjaPath;python=$PythonPath;msys=$MsysRoot;sdl=$SdlRoot;openxr_loader=$OpenXRLoader;engine=$EngineLibrary;host=$HostLibrary;engine_exists=(Test-Path -LiteralPath $EngineLibrary);host_exists=(Test-Path -LiteralPath $HostLibrary)} | ConvertTo-Json
    return
}
New-Item -ItemType Directory -Path $taskWork -Force | Out-Null
$taskCommon=@('-G','Ninja',"-DCMAKE_MAKE_PROGRAM=$NinjaPath",'-DCMAKE_BUILD_TYPE=Release',"-DCMAKE_TOOLCHAIN_FILE=$taskNdk/build/cmake/android.toolchain.cmake",'-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-29','-DANDROID_STL=c++_shared')
function Configure-Build([string]$source,[string]$build,[string[]]$options,[string[]]$target=@()) {
    & $CMakePath -S $source -B $build @taskCommon @options
    if($LASTEXITCODE){throw "CMake configuration failed: $source"}
    $taskBuildArgs=@('--build',$build,'-j',"$Jobs");if($target){$taskBuildArgs+='--target';$taskBuildArgs+=$target}
    & $CMakePath @taskBuildArgs
    if($LASTEXITCODE){throw "Compilation failed: $source"}
}
# Reuse the accepted, compiled dependencies read-only. New dependency builds use
# this port's cache; never reconfigure or install into another port's cache.
if(!(Test-Path -LiteralPath (Join-Path $SdlRoot 'lib/libSDL2.so'))){
    if(!(Test-Path -LiteralPath (Join-Path $SdlSource 'CMakeLists.txt'))){throw "Missing SDL source: $SdlSource"}
    $SdlRoot=Join-Path $taskWork 'sdl-android'
    Configure-Build -source $SdlSource -build (Join-Path $taskWork 'sdl-android-build') -options @('-DSDL_SHARED=ON','-DSDL_STATIC=OFF','-DSDL_TEST=OFF','-DSDL_TESTS=OFF',"-DCMAKE_INSTALL_PREFIX=$SdlRoot")
    & $CMakePath --install (Join-Path $taskWork 'sdl-android-build')
    if($LASTEXITCODE){throw 'SDL install failed.'}
}
if(!(Test-Path -LiteralPath $OpenXRLoader)){
    if(!(Test-Path -LiteralPath (Join-Path $OpenXRSource 'CMakeLists.txt'))){throw "Missing OpenXR source: $OpenXRSource"}
    Configure-Build -source $OpenXRSource -build (Join-Path $taskWork 'openxr-android') -options @('-DDYNAMIC_LOADER=ON','-DBUILD_API_LAYERS=OFF','-DBUILD_TESTS=OFF','-DBUILD_TESTING=OFF') -target 'openxr_loader'
    $OpenXRLoader=Join-Path $taskWork 'openxr-android/src/loader/libopenxr_loader.so'
}
if(!$SkipEngineBuild){
    & (Join-Path $PSScriptRoot 'build-engine.ps1') -MsysRoot $MsysRoot -PythonPath $PythonPath -WorkEngine $WorkEngine -Platform Android -AndroidNdk $taskNdk -SdlRoot $SdlRoot -Jobs $Jobs
    if($LASTEXITCODE){throw 'MAME ARM64 build failed.'}
}
if(!$SkipHostBuild){Configure-Build -source $taskProject -build $taskHostBuild -options @() -target @('rchase2_xr','rchase2_audio')}
$taskDestination=Join-Path $taskWork 'jniLibs/arm64-v8a'
New-Item -ItemType Directory -Path $taskDestination -Force | Out-Null
$taskCpp=Join-Path $taskNdk 'toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so'
$taskAudio=Join-Path $taskHostBuild 'librchase2_audio.so'
$taskLibraries=@{ 'libmain.so'=$EngineLibrary; 'libSDL2.so'=(Join-Path $SdlRoot 'lib/libSDL2.so'); 'libc++_shared.so'=$taskCpp; 'librchase2_xr.so'=$HostLibrary; 'librchase2_audio.so'=$taskAudio; 'libopenxr_loader.so'=$OpenXRLoader }
foreach($taskName in $taskLibraries.Keys){
    $taskFile=$taskLibraries[$taskName]
    if(!(Test-Path -LiteralPath $taskFile -PathType Leaf)){throw "Native library is missing: $taskFile"}
    Copy-Item -LiteralPath $taskFile -Destination (Join-Path $taskDestination $taskName) -Force
}
$taskExports=& $taskNm -D --defined-only $HostLibrary
if($LASTEXITCODE){throw 'Cannot inspect native host exports.'}
foreach($taskExport in @('rchase2_xr_start','rchase2_xr_stop','rchase2_xr_android_init')){
    if(!($taskExports -match "\b$taskExport$")){throw "Native host export is missing: $taskExport"}
}
$taskAudioExports=& $taskNm -D --defined-only $taskAudio
if($LASTEXITCODE){throw 'Cannot inspect audio worker exports.'}
foreach($taskExport in @('create','push','read','pause','stats','destroy')){
    if(!($taskAudioExports -match "\brchase2_audio_$taskExport$")){throw "Audio worker export missing: $taskExport"}
}
$taskKey=Join-Path $taskPrivate 'rchase2.keystore'
if(!(Test-Path -LiteralPath $taskKey)){
    New-Item -ItemType Directory -Path (Split-Path -Parent $taskKey) -Force | Out-Null
    & (Join-Path $JavaHome 'bin/keytool.exe') -genkeypair -keystore $taskKey -storepass android -keypass android -alias rchase2 -keyalg RSA -keysize 3072 -validity 10000 -dname 'CN=Rail Chase 2 VR Local Development'
    if($LASTEXITCODE){throw 'Local development signing key creation failed.'}
}
$taskOldJava,$taskOldAndroid,$taskOldGradle=$env:JAVA_HOME,$env:ANDROID_HOME,$env:GRADLE_USER_HOME
try {
    $env:JAVA_HOME=$JavaHome; $env:ANDROID_HOME=$AndroidSdk; $env:GRADLE_USER_HOME=$taskGradleHome
    $taskGradle=@('-p',(Join-Path $taskProject 'android'),':app:assembleRelease',"-Prchase2NativeDir=$(Split-Path -Parent $taskDestination)",'--no-daemon','--console=plain')
    if($Offline){$taskGradle+='--offline'}
    & (Join-Path $taskProject 'android/gradlew.bat') @taskGradle
    if($LASTEXITCODE){throw 'Quest APK assembly failed.'}
    $taskApk=Join-Path $taskWork 'android/app/outputs/apk/release/app-release.apk'
    & $PythonPath (Join-Path $PSScriptRoot 'package.py') --check-apk $taskApk --apksigner $taskSigner
    if($LASTEXITCODE){throw 'APK signature/content validation failed.'}
    $taskBadging=& (Join-Path $AndroidSdk 'build-tools/36.0.0/aapt2.exe') dump badging $taskApk
    if($LASTEXITCODE -or !($taskBadging -match "package: name='com.vrlightgunhub.rchase2'")){throw 'APK package identity validation failed.'}
    Write-Output $taskApk
} finally { $env:JAVA_HOME=$taskOldJava; $env:ANDROID_HOME=$taskOldAndroid; $env:GRADLE_USER_HOME=$taskOldGradle }
