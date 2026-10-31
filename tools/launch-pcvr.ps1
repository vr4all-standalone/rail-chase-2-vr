param([string]$RomDirectory = (Join-Path $PSScriptRoot 'roms'))
$ErrorActionPreference='Stop'
$taskRoot=$PSScriptRoot
if(!(Test-Path -LiteralPath (Join-Path $taskRoot 'rchase2.exe'))) {$taskRoot=Split-Path -Parent $PSScriptRoot}
$taskRom=[IO.Path]::GetFullPath($RomDirectory)
foreach($taskFile in @('rchase2.zip')) {
    if(!(Test-Path -LiteralPath (Join-Path $taskRom $taskFile) -PathType Leaf)) {throw "Missing owned game file: $taskRom\$taskFile"}
}
Push-Location -LiteralPath $taskRoot
try {
    & (Join-Path $taskRoot 'rchase2.exe') -rompath $taskRom -verifyroms rchase2
    if($LASTEXITCODE){throw 'MAME rejected the game data. Use the original Rail Chase 2 Revision A arcade set with all 23 required payloads, including the Z80 I/O firmware. No CHD is used.'}
    $taskData=Join-Path $taskRoot 'userdata'
    foreach($taskDirectory in @('nvram','cfg','diff','state','snap')) {New-Item -ItemType Directory -Path (Join-Path $taskData $taskDirectory) -Force | Out-Null}
    $env:RCHASE2_VR='1'
    $env:RCHASE2_FAST_RASTER='1'
    $env:RCHASE2_UART_COALESCE='1'
    $env:RCHASE2_AUDIO_STRETCH='1'
    $env:RCHASE2_AUDIO_LIBRARY=Join-Path $taskRoot 'rchase2_audio.dll'
    $env:RCHASE2_XR_LIBRARY=Join-Path $taskRoot 'rchase2_xr.dll'
    $env:RCHASE2_ASSETS=Join-Path $taskRoot 'assets'
    $env:RCHASE2_CUSTOM_GUN=Join-Path $taskRoot 'models/guns/custom.glb'
    $env:RCHASE2_SETTINGS=Join-Path $taskData 'settings.ini'
    $env:RCHASE2_XR_LOG=Join-Path $taskData 'rchase2-xr.log'
    $env:RCHASE2_AUDIO_LOG=Join-Path $taskData 'rchase2-audio.log'
    & (Join-Path $taskRoot 'rchase2.exe') rchase2 -rompath $taskRom -nvram_directory (Join-Path $taskData 'nvram') `
        -cfg_directory (Join-Path $taskData 'cfg') -diff_directory (Join-Path $taskData 'diff') `
        -state_directory (Join-Path $taskData 'state') -snapshot_directory (Join-Path $taskData 'snap') `
        -video none -sound sdl -skip_gameinfo -noreadconfig
    $taskPlayExit=$LASTEXITCODE
    if(Test-Path -LiteralPath $env:RCHASE2_XR_LOG) {Get-Content -LiteralPath $env:RCHASE2_XR_LOG -Tail 15}
    if($taskPlayExit){throw "Rail Chase 2 session exited with code $taskPlayExit."}
} finally {Pop-Location}
