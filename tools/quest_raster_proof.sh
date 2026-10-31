#!/system/bin/sh
# Isolated, bounded native benchmark. No app install, state edits or XR session.
set -eu
root=/data/local/tmp/vr4all-rchase2-performance
if [ "${RCHASE2_PROOF_AUDIO:-0}" != 0 ]; then
 echo 'SDL output requires an Android Activity; this runner verifies native synthesis to WAV only'; exit 2
fi
if [ "${RCHASE2_HEADSET_RESERVED:-0}" != 1 ] && dumpsys activity activities | grep -E '(topResumedActivity=|ResumedActivity:).*com\.vrlightgunhub\..*/.*GameActivity' >/dev/null; then
 echo 'A VR4ALL game is resumed; reserve the shared headset first'; exit 3
fi
case "${1:-}" in reference|fast) variant="$1";; *) exit 2;; esac
run_name="${2:-$variant}"
case "$run_name" in "$variant"|"$variant"-*) ;; *) exit 2;; esac
case "$run_name" in *[!a-zA-Z0-9_-]*) exit 2;; esac
out="$root/$run_name"
if [ -e "$out" ]; then echo 'Use a fresh output directory'; exit 2; fi
mkdir "$out"
mkdir "$out/nvram" "$out/cfg" "$out/diff" "$out/state" "$out/snap"
export LD_LIBRARY_PATH="$root"
export RCHASE2_BENCH_ROOT="$root" RCHASE2_BENCH_LIBRARY="$root/libmain.so"
export RCHASE2_VR=1 RCHASE2_FAST_RASTER=1
if [ "${RCHASE2_PROOF_UART:-0}" = 1 ]; then export RCHASE2_UART_COALESCE=1; fi
export RCHASE2_XR_LIBRARY="$root/librchase2_bench_host.so"
export RCHASE2_XR_LOG="$out/xr.log" RCHASE2_RASTER_LOG="$out/raster.tsv"
export RCHASE2_RESEARCH_DIR="$out" RCHASE2_DIAGNOSTIC_MODE=gameplay
export RCHASE2_LIVE_PROOF_DIR="$out/scenes"
if [ "$variant" = reference ]; then
 export RCHASE2_CPU_RASTER=0 RCHASE2_UART_REFERENCE=0 RCHASE2_FRAME_POOL=0
 export RCHASE2_AUDIO_REFERENCE=0
else
 unset RCHASE2_CPU_RASTER RCHASE2_UART_REFERENCE RCHASE2_FRAME_POOL RCHASE2_AUDIO_REFERENCE
fi
cd "$out"
taskset 38 "$root/rchase2-native-bench" rchase2 \
 -rompath /sdcard/Android/data/com.vrlightgunhub.rchase2/files/roms \
 -skip_gameinfo -noreadconfig -video none -sound none -numprocessors 2 \
 -joystickprovider none -keyboardprovider none -mouseprovider none -lightgunprovider none \
 -seconds_to_run 60 -nothrottle -noautoframeskip -frameskip 0 \
 -nvram_directory "$out/nvram" -cfg_directory "$out/cfg" -diff_directory "$out/diff" \
 -state_directory "$out/state" -snapshot_directory "$out/snap" -wavwrite "$out/native.wav" \
 -autoboot_script "$root/raster_proof.lua" -autoboot_delay 0 > engine.log 2>&1
tail -n 3 engine.log
