param([string]$PythonPath='D:/Python314/python.exe',[string]$MsysRoot='D:/msys64')
$ErrorActionPreference='Stop'
$taskProject=Split-Path -Parent $PSScriptRoot
$taskBuild='D:/VR4ALL-RailChase2-Build/build'
$taskTemp=Join-Path $taskBuild 'test-temp'
New-Item -ItemType Directory -Path $taskTemp -Force | Out-Null
$taskOldPath,$taskOldTemp,$taskOldTmp=$env:PATH,$env:TEMP,$env:TMP
try {
    $env:PATH=(Join-Path $MsysRoot 'mingw64/bin')+';'+$env:PATH
    $env:TEMP=$env:TMP=$taskTemp
    & $PythonPath -B -m unittest discover -s (Join-Path $taskProject 'tests') -v
    if($LASTEXITCODE){throw 'Python regressions failed.'}
    foreach($taskName in @('cabinet_io','game_state','credit_policy','input_policy','host_input_policy','native_shot','settings_io','aim_policy','camera_lens','gun_pose','capture','launch_policy','raster_policy','frame_pool','uart_clock_policy','native_reticle','audio_probe','adaptive_audio_bridge','cart_bottom','foveation_policy')) {
        $taskExe=Join-Path $taskBuild ($taskName+'_test.exe')
        & (Join-Path $MsysRoot 'mingw64/bin/g++.exe') -std=c++20 -Wall -Wextra "-I$taskProject/vendor/glm" "-I$taskProject/vendor/openxr/include" (Join-Path $taskProject ('tests/'+$taskName+'_test.cpp')) -o $taskExe
        if($LASTEXITCODE){throw "Cannot compile $taskName"}
        & $taskExe
        if($LASTEXITCODE){throw "Native regression failed: $taskName"}
    }
} finally {$env:PATH=$taskOldPath;$env:TEMP=$taskOldTemp;$env:TMP=$taskOldTmp}
