param(
    [Parameter(Mandatory=$true)][string]$MsysRoot,
    [Parameter(Mandatory=$true)][string]$PythonPath,
    [Parameter(Mandatory=$true)][string]$WorkEngine,
    [int]$Jobs = 2,
    [ValidateSet('Windows','Android')][string]$Platform = 'Windows',
    [string]$AndroidNdk,
    [string]$SdlRoot,
    [switch]$Regenerate
)
$ErrorActionPreference = 'Stop'
$taskProject = Split-Path -Parent $PSScriptRoot
$taskSource = Join-Path $taskProject 'engine'
$taskReferenceSource = Join-Path $taskProject '../carnevil-vr/engine'
if (!(Test-Path -LiteralPath $taskSource) -and (Test-Path -LiteralPath (Join-Path $taskReferenceSource '.git'))) { $taskSource = $taskReferenceSource }
$taskLock = Get-Content -LiteralPath (Join-Path $taskProject 'engine-lock.json') -Raw | ConvertFrom-Json
$taskWork = [IO.Path]::GetFullPath($WorkEngine)
if ($taskWork -match '\s') { throw 'MAME requires a build source path without spaces. Use a dedicated compiler cache directory.' }
$taskExpectedWork=[IO.Path]::GetFullPath('D:/VR4ALL-RailChase2-Build/engine')
if($taskWork.TrimEnd('\','/') -ne $taskExpectedWork) { throw 'This development project exclusively owns D:/VR4ALL-RailChase2-Build/engine. Refusing another compiler tree.' }
for($taskAncestor=$taskWork;$taskAncestor;$taskAncestor=Split-Path -Parent $taskAncestor) {
    if(Test-Path -LiteralPath $taskAncestor) {
        if((Get-Item -LiteralPath $taskAncestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {throw 'Private compiler paths must not traverse junctions or symbolic links.'}
    }
    if($taskAncestor -eq [IO.Path]::GetPathRoot($taskAncestor)){break}
}
$taskForbidden=@($taskProject,$taskSource,(Join-Path $taskProject '../vcop2-vr'),(Join-Path $taskProject '../carnevil-vr'),(Join-Path $taskProject '../virtua-cop-3-vr'))
foreach($taskPath in $taskForbidden) {
    $taskResolved=[IO.Path]::GetFullPath($taskPath).TrimEnd('\','/')
    if($taskWork -eq $taskResolved -or $taskWork.StartsWith($taskResolved+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Use a separate RailChase2 compiler checkout outside all port/reference source trees.' }
}
if ($Jobs -lt 1 -or $Jobs -gt 32) { throw 'Jobs must be between 1 and 32.' }
$taskMake = Join-Path $MsysRoot 'usr\bin\make.exe'
$taskCompiler = Join-Path $MsysRoot 'mingw64\bin\g++.exe'
foreach ($taskTool in @($taskMake,$taskCompiler,$PythonPath)) {
    if (!(Test-Path -LiteralPath $taskTool -PathType Leaf)) { throw "Missing build tool: $taskTool" }
}
if (!(Test-Path -LiteralPath (Join-Path $taskSource '.git'))) {
    $taskBundle = Join-Path $taskProject 'engine-source.bundle'
    if(Test-Path -LiteralPath $taskBundle) { & git clone $taskBundle $taskSource }
    else { & git clone --depth 1 --branch $taskLock.tag --single-branch $taskLock.repository $taskSource }
    if ($LASTEXITCODE) { throw 'MAME source download failed.' }
}
if (!(Test-Path -LiteralPath $taskWork)) {
    & git clone --no-hardlinks --single-branch $taskSource $taskWork
    if ($LASTEXITCODE) { throw 'Compiler source copy failed.' }
}
$taskHead = & git -C $taskWork rev-parse HEAD
if ($LASTEXITCODE -or $taskHead.Trim() -ne $taskLock.commit) {
    throw 'Compiler source must be the exact pinned MAME commit. Do not reuse another project checkout.'
}
$taskGitRoot=& git -C $taskWork rev-parse --show-toplevel
if($LASTEXITCODE -or [IO.Path]::GetFullPath($taskGitRoot.Trim()) -ne $taskWork.TrimEnd('\','/')) {throw 'Compiler directory must be the Git checkout root.'}
$taskOwnerFile=Join-Path $taskWork '.rchase2-build-owner'
if(Test-Path -LiteralPath $taskOwnerFile) {
    if((Get-Content -LiteralPath $taskOwnerFile -Raw).Trim() -ne $taskProject) {throw 'Compiler tree is owned by another RailChase2 project.'}
} else {
    $taskDirty=& git -C $taskWork status --porcelain --untracked-files=no
    if($LASTEXITCODE -or $taskDirty) {throw 'Refusing to claim an existing modified compiler tree.'}
    $taskOwnerStream=[IO.File]::Open($taskOwnerFile,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
    try {$taskOwnerBytes=[Text.Encoding]::UTF8.GetBytes($taskProject);$taskOwnerStream.Write($taskOwnerBytes,0,$taskOwnerBytes.Length)}finally{$taskOwnerStream.Dispose()}
}
$taskNative = Join-Path $taskWork 'src\lib\rchase2'
New-Item -ItemType Directory -Path $taskNative -Force | Out-Null
Get-ChildItem -LiteralPath (Join-Path $taskProject 'native') -Filter '*.h' |
    ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $taskNative $_.Name) }
# Reconstruct the ordered patch stack against the pin. Later live hooks overlap
# earlier capture hooks, so reversing each individual patch is not idempotent.
& $PythonPath (Join-Path $PSScriptRoot 'sync_engine_patches.py') --engine $taskWork --patches (Join-Path $taskProject 'patches') --commit $taskLock.commit
if($LASTEXITCODE){throw 'Private engine source has unknown edits or the patch stack is invalid.'}
$taskOldPath, $taskOldMingw, $taskOldNdk = $env:PATH, $env:MINGW64, $env:ANDROID_NDK_HOME
$taskOldTemp,$taskOldTmp=$env:TEMP,$env:TMP
$taskCompilerTemp=Join-Path $taskWork 'build/compiler-temp'
New-Item -ItemType Directory -Path $taskCompilerTemp -Force | Out-Null
try {
    $env:TEMP=$env:TMP=$taskCompilerTemp
    $env:PATH = (Join-Path $MsysRoot 'mingw64\bin')+';'+(Split-Path -Parent $PythonPath)+';'+(Join-Path $MsysRoot 'usr\bin')+';'+$taskOldPath
    $env:MINGW64 = (Join-Path $MsysRoot 'mingw64').Replace('\','/')
    Push-Location -LiteralPath $taskWork
    try {
        $taskArgs=@("-j$Jobs", "SUBTARGET=$($taskLock.subtarget)", "SOURCES=$($taskLock.sources)",
            'USE_QTDEBUG=0','USE_TAPTUN=0','USE_PCAP=0','NO_USE_PORTAUDIO=1','USE_BGFX=0',
            "PYTHON_EXECUTABLE=$([IO.Path]::GetFileName($PythonPath))",'NOWERROR=1','SYMBOLS=0','IGNORE_GIT=1')
        if($Regenerate) {$taskArgs+='REGENIE=1'}
        if($Platform -eq 'Windows') {
            # The adaptive output adapter lives in the SDL sound provider.
            # Native Windows OSD otherwise compiles that provider as a stub.
            $taskArgs+=@('USE_SDL=1','REGENIE=1')
            $taskSdlCache=Join-Path $taskWork 'build/mingw-gcc/obj/x64/Release/osd_windows/src/osd'
            $taskSdlObject=Join-Path $taskSdlCache 'modules/sound/sdl_sound.o'
            if(Test-Path -LiteralPath $taskSdlObject) {
                $taskSdlSymbols=& (Join-Path $MsysRoot 'mingw64/bin/nm.exe') -C $taskSdlObject
                if($LASTEXITCODE){throw 'Cannot inspect Windows SDL provider cache.'}
                if(!($taskSdlSymbols -match 'sound_sdl::stream_sink_open')) {
                    # Make does not track changed preprocessor options. Refresh
                    # only the OSD units affected by USE_SDL/USE_XINPUT.
                    foreach($taskUnit in @('modules/sound/sdl_sound.o','modules/sound/sdl3_sound.o','modules/lib/osdobj_common.o','modules/input/input_winsdl.o','modules/input/input_x11.o','modules/input/input_xinput.o')) {
                        $taskObject=[IO.Path]::GetFullPath((Join-Path $taskSdlCache $taskUnit))
                        if(!$taskObject.StartsWith([IO.Path]::GetFullPath($taskSdlCache)+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Unexpected compiler object path.'}
                        if(Test-Path -LiteralPath $taskObject){Remove-Item -LiteralPath $taskObject -Force}
                    }
                }
            }
        }
        if($Platform -eq 'Android') {
            if(!(Test-Path -LiteralPath (Join-Path $AndroidNdk 'build/cmake/android.toolchain.cmake'))) {throw 'A valid Android NDK is required.'}
            if(!(Test-Path -LiteralPath (Join-Path $SdlRoot 'lib/libSDL2.so'))) {throw 'A compiled ARM64 SDL install is required.'}
            $env:ANDROID_NDK_HOME=$AndroidNdk.Replace('\','/')
            $taskNm=Join-Path $AndroidNdk 'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-nm.exe'
            $taskDrc=Join-Path $taskWork 'build/android/obj/arm64/Release/src/devices/cpu/drcuml.o'
            if(Test-Path -LiteralPath $taskDrc) {
                $taskFactory=& $taskNm -C $taskDrc
                if(!($taskFactory -match 'make_drcbe_arm64')) {
                    # An old NOASM precompiled header can silently retain the C backend.
                    foreach($taskStale in @($taskDrc,(Join-Path $taskWork 'build/android/obj/arm64/Release/emu.h.gch'))){if(Test-Path -LiteralPath $taskStale){Remove-Item -LiteralPath $taskStale -Force}}
                }
            }
            $taskAndroidLinkFlags='-Wl,-Bsymbolic-non-weak-functions'
            $taskArgs+=@('android-arm64','NO_X11=1','NO_USE_XINPUT=1','NO_OPENGL=1','USE_BGFX=0','NO_USE_PULSEAUDIO=1','NO_USE_PIPEWIRE=1',"SDL_INSTALL_ROOT=$($SdlRoot.Replace('\','/'))","LDFLAGS=$taskAndroidLinkFlags")
        }
        & $taskMake @taskArgs
        if ($LASTEXITCODE) { throw "MAME compiler failed with exit code $LASTEXITCODE" }
        if($Platform -eq 'Android') {
            # Make does not track command-line linker flag changes. Force only
            # the final link, retaining the compiled ARM64 objects. Defined
            # strong functions bind locally; weak identities and imported SDL,
            # JNI and C++ runtime functions retain their original bindings.
            $taskLinkProject=Join-Path $taskWork 'build/projects/sdl/mamerchase2/gmake-android-arm64'
            & $taskMake -C $taskLinkProject -f rchase2.make 'config=release' "LDFLAGS=$taskAndroidLinkFlags" `
                '--what-if=../../../../android/obj/arm64/Release/src/osd/sdl/android_main.o' `
                '../../../../../android-project/app/src/main/libs/arm64-v8a/libmain.so'
            if($LASTEXITCODE){throw 'RailChase2 ARM64 final link failed.'}
        }
        # Model 2 includes DSP recompilers; a linked DRC must use ARM64 on Quest.
        if($Platform -eq 'Android' -and (Test-Path -LiteralPath $taskDrc) -and !((& $taskNm -C $taskDrc) -match 'make_drcbe_arm64')){throw 'A linked DRC must use the ARM64 backend.'}
    } finally { Pop-Location }
} finally { $env:PATH=$taskOldPath; $env:MINGW64=$taskOldMingw; $env:ANDROID_NDK_HOME=$taskOldNdk; $env:TEMP=$taskOldTemp; $env:TMP=$taskOldTmp }
$taskExe = Join-Path $taskWork $(if($Platform -eq 'Android') {'android-project/app/src/main/libs/arm64-v8a/libmain.so'}else{'rchase2.exe'})
if (!(Test-Path -LiteralPath $taskExe)) { throw 'Compiler returned success but no engine executable was found.' }
Write-Output $taskExe

