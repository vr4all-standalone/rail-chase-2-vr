# Performance status

## Current: versionCode9 graphics/cache update, 2026-10-30

Ordinary Quest launches select static Medium peripheral foveation with full recommended eye dimensions and conservative ordinary-swapchain fallback. On one paused Quest3S scene,24samples per condition average59.29displayFPS with Medium versus53.92 with None; the user accepted image/menu clarity. This is approximately10%higher display cadence in that comparison, not guaranteed72FPS or faster native emulation.

Follow-up High testing reaches72.48displayFPS versus61.58None on a fixed scene and72.38 over60later gameplay samples. The user reports nearly unchanged/slightly blurry graphics and no felt performance gain, so High is not promoted. Native execution in the played section remains approximately82–90%; compositor cadence does not establish full-speed game motion. No additional native CPU fix is claimed from the compile-only follow-up. Cart aiming/shooting is now user-accepted.

An exact resource/raster upload cache skips unchanged bytes while retaining palette, fallback, dimensional and frame-lifetime correctness. Actual-Quest ABBA offscreen preparation over11owned captures falls14.14%; final pixels are identical. Preparation time is distinct from live VR FPS. A schedule-arithmetic experiment was1.54%slower in desktop gameplay and was removed; the original13engine patches and accepted Android engine/audio bytes remain unchanged.

Combined same-signed version9 is installed and hash-verified. It preserves current HUD/reticle/reset settings and includes the accepted original cart-rim aim-through fix.32CTests,32Python,20standalone C++ and16Android checks pass; Windows/Quest builds pass. Version9 actual gameplay is observed; further performance improvement and PCVR physical acceptance remain unresolved/untested respectively. See `overseer/changes/2026-10-30-rchase2-performance3/report.md` in the overseer workspace for receipts and limits. No publication.

## Historical: versionCode4 FPS/audio update, user-accepted 2026-10-30

The user reports “performance is amazing now.” Actual heavy-gameplay intervals in the observed session average84.83% native speed, range78.85–93.59%; this is not a claim of100% native speed. A bounded immutable-frame pool, guarded CPU-raster bypass, avoided unused fallback uploads and timing-preserving UART batching are enabled. Windows and actual ARM64 original/optimized runs match180 RAM snapshots (including the I/O CPU), complete native synthesized WAV, per-frame state, timed events and11 owned scenes; four retained native rasters match. The service pair matches75 RAM snapshots/audio/state/4scenes with no raster bypasses.

The independent pitch-preserving output worker is active in the real Quest OpenSL ES stream at48000Hz stereo/512-frame callbacks.183 observed audio intervals have zero non-warmup missing frames and zero input overflow. Output samples discarded on pause/resume are intentional old-generation flushes; steady running intervals show no output drops. Actual ARM64 DSP/worker and module-lifetime tests pass. Native synthesis/WAV recording remain unchanged. The silent Windows SDL adapter also activates the worker and has no late missing frames in its bounded test. Physical Windows OpenXR/audio listening remains untested.

Reticle replacement is explicitly deferred by the user's latest instruction: they are building a new GunbladeNY reticle system to port later. The prototype is parked behind an internal default-Off gate, has no normal XR hookup, and a real GL regression proves targets cannot change normal rendering. The current installed version4 leaves the native reticle unchanged. The newly requested optional cart bottom is separate follow-on work.

Exact evidence: `D:/VR4ALL-RailChase2-Build/research/performance2-20261029`; workspace report: `overseer/changes/2026-10-30-rchase2-performance-audio-reticle/report.md`. No publication.

## Quest 3S investigation, 2026-10-29

The user confirmed game entry but reported poor FPS. Actual gameplay logs show approximately 63–71% native speed (later profile samples 62–76%). The 25-second on-device simpleperf capture contains 9,339 CPU-cycle samples: scheduler9.46%, billboard Z80 execution7.92%, i960 execute_op5.07%, plus substantial software scanline/texture rendering. XR prepare/submit timing alone did not explain native slowdown.

Patch0008 removes the duplicate CPU polygon raster only for recognized Revision A mode9 gameplay, a ready VR host, a complete viewport update and the matching owned scene with valid material resources and visible world geometry. Boot, service, continuation mode11, direct framebuffer, partial updates, stale/invalid captures and unavailable hosts retain the original renderer. Native geometry is left unprojected when skipped, so later CPU fallback can render it correctly. `RCHASE2_CPU_RASTER` (presence, including0) forces original rendering; `RCHASE2_SCENE_DIR` also retains it. Quest and PCVR launchers request `RCHASE2_FAST_RASTER=1`. Original clocks, scheduling, sound synthesis and game actions are unchanged.

Independent60-native-second Windows and actual ARM64 reference/optimized pairs match all120 RAM snapshots, the complete synthesized WAV, native timed events and per-frame state. Each skips2,275 duplicate raster passes. Eleven owned scenes match exactly apart from the intentionally absent CPU world raster; four native fallback images match. Production GL replay of all eleven scenes produces55 byte-identical neutral/stereo/wider-view outputs between variants. The25-second service pair matches50 RAM snapshots, WAV, state, all four scenes and native images, with zero bypasses. ARM64 headless gameplay speed samples rise from about49–58% to74–79%; these omit the XR session's performance request and are fidelity/capacity evidence, not physical headset FPS acceptance.

VersionCode3 with the existing package/signing identity is installed on Quest3S340YC10GB00YWB. Actual updated headset gameplay performance remains pending. Windows OpenXR physical play remains untested. The native reticle currently uses near-camera geometry; moving it to the real laser endpoint remains separate work behind FPS verification.

Exact logs, baseline profile, paired captures and output hashes are private under `D:/VR4ALL-RailChase2-Build/research/performance-20261029`. See the workspace report `overseer/changes/2026-10-29-rchase2-performance/report.md`.

## Earlier desktop baseline

The 60-second native gameplay baseline reported 179.17% speed on this development PC, with native software rendering and audio output disabled by the bounded harness. The measured dual-player and aiming probes also exited successfully. These measurements establish useful native research throughput, not headset frame pacing, audio quality or Windows OpenXR performance.

This predates the guarded raster optimization and the Quest measurements above. No UART or sound-synthesis optimization is enabled. Compiler jobs remain capped at two and writable compiler/temp output belongs on D:.
