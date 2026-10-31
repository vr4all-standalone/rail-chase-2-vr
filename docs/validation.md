# Rail Chase 2 validation ledger

## Latest: versionCode9 graphics/cache and cart aiming, 2026-10-30

Version9 is built, signed and installed; installed APK SHA256cfb9b782b0b184d9d85a99d690e9ede9f1b32e627a23e49d361f3c794989d2a6 matches the candidate.32/32CTests,32Python,20standalone C++ and16Android checks pass, along with both host builds and the Quest APK build. Accepted engine/audio/runtime libraries are unchanged. Current shared HUD size, separate reticle min/max, calibrated defaults and Reset persist in the combined source.

Medium foveation improves a fixed paused Quest3S comparison53.92→59.29displayFPS; the user reports the image/menu looked fine. Actual-Quest upload-cache preparation improves14.14% in ABBA captured-scene runs with identical output pixels. These measurements do not establish whole-route72FPS or faster native game execution. The slower scheduler experiment was rejected and removed. The High follow-up below remains nondefault after user feedback.

The recognized original cart rim remains visible but no longer intercepts the host reticle/aim ray, regardless of bottomOn/Off. Five captured poses and synthetic GL checks pass, with unchanged drawn pixels and conservative unknown-object behavior. Follow-up version9 gameplay is observed and the user confirms cart aiming is great. High graphics reaches approximately72displayFPS in the measured scene/gameplay window, but the user reports possible slight blur and no felt performance gain. High is not promoted; normal Medium is unchanged. Native game execution remains approximately82–90% in the observed played section. Windows OpenXR physical acceptance is untested. Final read-only check found RC2 closed; installer chat was notified, with direct user authorization, that the Quest is free and code9 must be preserved. See the performance3 overseer report for detailed evidence and the compile-only CPU probe; no tested additional native-speed fix is claimed.

## Latest: versionCode7 cart depth and reticle-control verification, 2026-10-30

Version7 increases the full cart interior depth by50.02% after the user called version6 “amazing” and confirmed the3D reticle effect. FloorY1.5 replaces5.001, underside1.3 replaces4.8, and the wall join remains9.51. Original timber UVs and96vertices/32triangles are unchanged. Parent Windows build,27/27CTests and Quest build pass; same-signed version7 is installed with data preserved. The exact accepted engine/audio/runtime APK libraries remain unchanged. Final version7 cart appearance acceptance is pending; PCVR physical play is untested.

User uncertainty about reticle distance controls was investigated without changing the matching Gunblade policy. Parent reproduced actual menu actions→save/load→Resume→nativeGLuniform/image output. At4m, shrink0.2→6.0 changes276pixels and the marker reference1.0→2.92683; minimum25→100% changes591pixels and reference4.0. Near1m and while the menu stays open, changes are correctly0pixels. All144 policy comparisons match current GunbladeNY exactly. At4m a6.0→6.1 step changes apparent size only0.44%; with panel1.8m/minimum25%/shrink6m the minimum dominates beyond19.8m. These are deterministic captured-scene tests, not measurements of the user's tracked poses. All reticle production files are unchanged from version6. Existing32Python/19standalone/15Android checks, ABI and actual-scene stereo evidence below apply to the same unchanged code; fresh version7 integrated CTests include the new cart dimensions. See the overseer report and distance-controls receipt for exact commands and limits.

## Latest: versionCode6 deeper cart and Gunblade NY reticles, 2026-10-30

Version6 is built, signed, packaged and installed on Quest3S340YC10GB00YWB. The installed APK SHA256 matches the canonical candidate. Engine, audio worker and runtime-library bytes remain identical to the accepted version4 baseline. Parent checks pass:27 CTests,32 Python,19 standalone C++,15 Android; Windows/ARM64 ABI compile assertions match the baseline. Both platform packages pass content/dependency/signature checks. Independent combined source review found no actionable issues.

The initial version5 floor used the correct timber texture but the user found it too high and platform-like. Version6 extends the wooden sides to a recessed floor4.5 native units lower, preserving that texture and the96-vertex/32-triangle budget. `Cart bottom` defaultsOn and explicitOff persists.

The user subsequently requested the latest GunbladeNY reticles. In-game retains RC2's own artwork at the 3D gun-ray endpoint; Custom/None and minimum-size/shrink-distance controls match Gunblade behavior. Logical player/hand eligibility, menu/service pointers and independent Laser control are preserved. Actual GL proof passes11 captured frames, including five complete dual-player sources and six absent-source frames,88 inactive/None comparisons, hollow Custom centers in both eyes and zero changes outside marker bounds. Parent native-view fidelity replay passes all11 frames with additions disabled. Replay diagnostics now retain actual captured matrices.

**Headset appearance/alignment acceptance for version6 is pending.** Installation and current version6 gameplay are verified: OpenGL ES3.2/OpenXR, native shots and the new settings rows are observed. A read-only snapshot of309 audio intervals contains263 steady intervals with zero missing frames, input/output queue drops or underrun callbacks; paused/warmup intervals are separate. A headset screenshot confirms the new menu and Cart bottomOn, but does not establish final depth/alignment acceptance or cold-relaunch persistence. Physical Windows OpenXR remains untested. Unknown or incomplete targeting artwork and full bitmap fallback preserve native pixels where they cannot be isolated safely. Cart recognition coverage is limited to the five measured poses; full-route coverage is unverified. Exact source/build/artifact receipts: `overseer/changes/2026-10-30-rchase2-cart-reticle/`. No publication. Earlier sections record their original milestones and are superseded where noted.

## Latest: versionCode4 FPS/audio, 2026-10-30

Installed on the same Quest3S/package/signature with data preserved. User feedback: “performance is amazing now.”25/25 integrated CTests,31 Python and18 standalone native checks,15 Android/delivery checks, actual ARM64 audio DSP/worker/module tests and exact Windows/ARM64 native-state/audio/scene pairs pass. Live OpenSL ES48000Hz stereo worker activation is confirmed; no non-warmup missing frames in183 observed intervals. Native speed remains below100% in some heavy sections; no physical Windows OpenXR check or publication.

Reticle replacement is parked and disabled in normal gameplay by explicit user instruction, pending their new GunbladeNY system. The next feature is the independently requested optional cart bottom; it is not part of this version4 baseline. See performance.md and `overseer/changes/2026-10-30-rchase2-performance-audio-reticle/report.md` for exact evidence and remaining limits. Earlier pending statements below describe their original milestones.

## Current: Quest performance candidate, 2026-10-29

Same-signed versionCode3 is installed on Quest3S. The user confirmed game entry on the previous startup fix and then reported poor FPS. The guarded duplicate CPU-raster shortcut now passes31 Python,13 standalone C++,16 CTest and15 Android checks, exact Windows/ARM64 state/audio/scene comparisons and55 identical stereo replay outputs. Initial updated headset FPS acceptance is pending; Windows OpenXR physical play is untested. See `performance.md` and the workspace `overseer/changes/2026-10-29-rchase2-performance/report.md`. No publication.

The approved wide purple validated launch panel, Arcade player colors/grey hardware, common angle-zero pose and new `Rail Chase 2 - VR4ALL` compiled application label are present. Missing angle markers now trigger an immediate one-time persisted reset. The near-camera native reticle remains unchanged until FPS verification.

## Earlier evidence



Local development, 2026-10-29. Compilation, native software behavior, stereo replay, physical headset launch/play and publication are separate gates. Earlier no-install/no-play statements below describe their original milestones and are superseded by the current result above.



## Latest Quest preparation fix

The earlier plain preparation page was replaced with the approved wide VR4ALL Play panel, including once-only automatic entry, explicit revalidated Play and returned-session/lifecycle guards. Android versionCode is 2, with the same app ID and signing identity. The current Quest candidate is under `D:/VR4ALL-RailChase2-Build/artifacts/startup-fix`; exact identities are in `build-status.json` and that folder's checksums.

The reported missing-ROM block was reproduced under the installed app's own UID: an ADB-created `roms` folder was shell-owned mode2770, so the app could not traverse it even though the shell saw the complete uploaded archive. Repairing that one folder to2775 changed the actual production validator from missing/unreadable to ready, verifying all23 payloads without changing ROM bytes or app data. A matching uploaded hash alone had not established app access.

All15 Android storage/media/package/lifecycle tests pass, including the real readiness policy and wide-window manifest regressions observed failing before implementation. The updated signed APK is installed on Quest3S. With the headset awake, the actual preparation Activity verifies the files, displays READY and both archive checks, enables Play and uses the correct860×640dp window; screenshot and hierarchy evidence were inspected. Native engine/host bytes are unchanged, so no PCVR or emulator rebuild was needed for this Android-only adapter fix. Full gameplay and user acceptance remain separate. See `overseer/changes/2026-10-29-rchase2-startup-fix/report.md` in the workspace for the diagnostic attempts, commands and limits.

## Earlier native projectile feedback continuation

Sole game ownership transferred to the dedicated Rail Chase 2 chat on 2026-10-29. The continuation report is `overseer/changes/2026-10-29-rchase2-native-feedback/report.md` in the overseer workspace. Final continuation candidates use `D:/VR4ALL-RailChase2-Build/artifacts/native-feedback`; their exact identities are recorded in the current `build-status.json` and candidate `SHA256SUMS.txt`. The earlier parent candidates below remain historical evidence.

Two 72-second original cabinet traces identify 517 native projectile creations (317 P1, 200 P2) among 6,164 owner/child stores. The production decoder reproduces those counts exactly and rejects all cleanup stores. All six corresponding gameplay/service screenshots match byte-for-byte. A third 72-second run with the final production observer and desktop GL host records the same native creation counts and the same published production counters; entering Service invalidates them. These are native software and desktop GL observations, without a physical headset.

Native events now drive existing delivered-hand feedback guards. Unknown ROM/state/source PC, partial stores, inactive players, wrong object callbacks and broken owner links remain closed. The observer explicitly unregisters its memory tap on teardown. Counters remain engine-owned outside shared storage, preserving ABI `0x52433201`. Main policy and tap lifetime regressions were observed failing before implementation/correction, then passing.

Final software checks pass: **15 Windows CTests**, **29 Python + 12 standalone C++** checks, and **13 Android platform tests**. Windows engine and host and ARM64 engine/host plus signed APK rebuilt successfully, with compiler concurrency at most two and all compiler/temp output on D:. Logs are `build/native-feedback-{policy-final,host-final,ctest-final,windows-engine-final,quest,platform}.log`. Android identity and signing certificate are unchanged. Controller feedback simulation and final local packaging results are recorded in the continuation report.

Physical haptics/recoil/muzzle timing, Quest/PCVR rendering, aim, audio and sustained performance remain untested. Original gun extraction, numerical health, automatic service-page detection and actual death/continue/routes remain open. Telemetry remains deferred. No installation, headset launch or GitHub publication was performed.

## Earlier parent review correction

The review correction cancels both `InputPolicy` and `NativeCreditPolicy` from the actual non-FOCUSED OpenXR session-event branch. Direct STOPPING/resume no longer depends on receiving a separate unfocused controls frame to cancel a queued coin or already-paid Start request. `host_input_policy_test.cpp` invokes the production event-path operation, keeps Start/trigger held through a sub-four-second regain, verifies zero resumed coin/Start injection for both insufficient and already-sufficient native credit, then verifies a genuinely fresh release/press works. The old event-path behavior failed this test before the fix.

Controller Start callouts now use the real joined/played-seat target rule: both hands show P1 before first play or P1-only return, both show P2 for a P2-only returning seat, and active/dual-history assignments remain reversible. The regression compares callout targets against actual production `InputPolicy` coin/Start requests across 64 joined/history/layout/hand cases; the first-play offhand case failed before the fix. No source-string assertion is used.

The host-only seam is `native/host_input_policy.h`; existing engine/ABI readers and cabinet headers are unchanged. Eleven shared/engine header pairs were checked byte-for-byte and their SHA256 identities match. MAME engines were not rebuilt. Windows host build plus **14/14 CTests** pass (exit 0), full `tools/test.ps1` passes **29 Python + 11 standalone C++** checks, and the Android/storage/package suite passes **13/13** (exit 0). Logs: `build/review-host-windows.log`, `review-ctest.log`, `review-policy-tests.log` and `review-platform-tests.log`.

`tools/build-quest.ps1 -SkipEngineBuild -Offline -Jobs 2` completed exit 0, rebuilding the ARM64 host and signed APK with the same application ID and signing key. APK identity remains `com.vrlightgunhub.rchase2`, signer certificate SHA256 remains `e2106b4d3903d39b59f78e89bc14dd7a31b046060adad82ce540751b1af9fc4f`. Log: `build/review-quest-build.log`.

The signed APK native engine and host payloads exactly match independent `llvm-strip --strip-unneeded` transforms of the current build products; raw JNI/merged libraries also match those build products. This accounts for normal Gradle symbol stripping. The portable ZIP engine/host bytes match their current verified binaries.

The refreshed canonical APK/PCVR candidates under **`D:/VR4ALL-RailChase2-Build/artifacts/review-fixed`** supersede `artifacts/initial`. Exact source, host, engine, APK and package hashes are in `build-status.json`, `source-handoff-sha256.json` and that directory's `SHA256SUMS.txt`. Earlier results/hashes below are retained as history. No headset use or publication was performed. Telemetry remains deferred and all physical/native-feature limitations still apply.

## Identity and ownership



- New source: `ports/rail-chase-2-vr`; no existing sibling project changed.

- Engine upstream pin: MAME 0.289 `f34f02505e32c1993c6a782b6814232cbfc74e36`, clean local clone with `--no-hardlinks --single-branch --no-checkout`, detached exact pin. No sibling uncommitted patches or build products copied.

- Private compiler root: `D:/VR4ALL-RailChase2-Build/engine`. Build/private/research outputs remain on D:.

- Android `com.vrlightgunhub.rchase2`; Windows `rchase2.exe`, host `rchase2_xr.dll`; Android host `librchase2_xr.so`; native ABI `0x52433201`; independent settings and future signing identity.

- Selected common source/provenance hashes are in `common-provenance.json`. Only stable licensed components and authored assets were imported; game-specific cabinet, state, pricing and aiming policies are separate.



## Evidence completed



Media: all 23 game/device payloads verified by size, CRC32, SHA1; original source archives unchanged by before/after SHA256. `research/media-complete-with-device.json` records exact paths/hashes. The initial incomplete native run failed missing billboard firmware, then the full device dependency was verified and staged separately.



Native read-only existing-pin baseline: `tools/run_diagnostic.py --engine D:/VR4ALL-Gunblade-Build/engine/gunblade.exe --roms D:/VR4ALL-RailChase2-Build/private/complete/roms --output .../research/native-gameplay --seconds 60 --mode gameplay`, exit 0, 179.17% reported headless speed. Service 25 seconds, exit 0, native TEST MENU visible. Dual-credit 60-second run, exit 0, independently verifies P1 then P2 active acknowledgements and both original HUDs. These runs do not verify this port's new patched engine.



Native aim: same bounded runner with `--script tools/aim_probe.lua`, exit 0. `native_aim_evidence_test.exe .../research/native-aim-probe/aim.tsv`, exit 0: 2,222 native samples, 50 distinct player/ADC positions, zero maximum world-coordinate error and exact inverse ADC round trips. See `native-state-evidence.md` for source addresses and interpretation.



TDD: cabinet mapping, fail-closed native reader, native credit gating, game-specific camera selection, shared physical gun pose/color and angle migration were each observed failing before implementation. Standalone production checks pass for cabinet, state, pricing, input transitions, settings, aim guards, camera, pose/color, capture ownership and startup policy. Python media/diagnostic/patch/replay checks pass. Platform/GL and final engine results will be appended after the running builds finish; temporary integration failures are retained in private logs rather than counted as passes.



## Final software verification



Windows engine initially built successfully (exit 0, log `build/windows-engine.log`). Parent then refreshed the existing generated Windows Makefile against the final native reader/input headers with `make -j1 -C D:/VR4ALL-RailChase2-Build/engine/build/projects/windows/mamerchase2/gmake-mingw64-gcc config=release64`; its log is `overseer/changes/2026-10-29-model2-last-three/rchase2-windows-engine-refresh.log`.



The final production engine + `rchase2_xr.dll` ran 60 native seconds with `RCHASE2_VR=1`, `RCHASE2_XR_SIM=1`, `RCHASE2_SIM_GUN_TEST=1`, `RCHASE2_SIM_AIM_CORNERS=1`, native-media verification and the bounded runner in **attract** mode (no Lua coin/start/fire inputs). It exited 0, no script/capture errors, 180.09% headless speed, and logged native valid/ready/playing with both actual players active. This exercises the guarded native reader, current host ABI, bounded native-credit protocol, native calibration, synthetic controller transitions and production GL renderer together. It is explicitly a desktop GL simulation without an OpenXR/headset session. Evidence: `research/native-live-host/run.json`, `native-live-host.log` and `native-live-host/engine.log`.



All **13 Windows CTests** pass, including real desktop GL renderer and presentation. Parent also rebuilt the ARM64 OpenXR host successfully. Logs are `overseer/changes/2026-10-29-model2-last-three/rchase2-{host-build,ctest,android-host-build}-final.log`. `tools/test.ps1` passes **29 Python + 10 standalone C++** checks, final log `build/policy-tests-handoff.log`, without adapter compiler warnings. Android storage/media/package harness passes **13 tests**, log `build/android-tests-final2.log`. Verified native aiming adds the separate 2,222-sample differential test described above.



Camera regression: the first imported renderer suite failed two tests because dynamic polygon-majority selection ignored an explicit frame lens. Actual captures identify a stable Rail Chase 2 reference of **768/768**, window 1, center 248/320, yoffset 128. The corrected policy honors explicit lenses and otherwise uses the measured stable reference; other layers cannot change world scale by polygon count. The new regression was observed failing before this correction, then passing. `research/native-camera-evidence.json` retains all eleven actual capture groups. Live capture and replay do not populate an inherited 400 or 65-degree `NativeFrame::lens`.



All eleven native captures pass completeness/resource bounds and exact foreground composition. Four are native-raster startup frames; seven contain real world geometry. All **14 original/instrumented screenshot checkpoints are byte-identical** (`instrumented-native-comparison.json`). Full capture -> serialization -> production neutral/left/right/wide GPU replay passes the strict >=98% exact, <=1 RGB MAE and zero foreground mismatch gates: worst exact **99.206674%**, maximum RGB MAE **0.0261677**, zero foreground mismatches. Real world frames contain **271–3,007 rendered world triangles**, with **147,641–177,347 pixels differing between eyes**. Exact metrics and outputs remain private in `research/native-scene-replay/metrics.json`.



Replay command (exit 0): set PATH to include `D:/msys64/mingw64/bin`, then run the bundled Codex Python `<local-user>/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe -B tools/replay_model2_trace.py --exe D:/VR4ALL-RailChase2-Build/host-windows/replay_model2_frames.exe --captures D:/VR4ALL-RailChase2-Build/research/native-scene-proof/scenes --output D:/VR4ALL-RailChase2-Build/research/native-scene-replay`. The first attempt with `D:/Python314` lacked NumPy; the next lacked MinGW runtime DLLs on PATH. Both environmental failures were corrected without loosening fidelity thresholds.



## Platform builds and candidate packaging

The full Quest command `tools/build-quest.ps1 -Jobs 2` completed **exit 0**, including ARM64 engine links, ARM64 DRC validation, current OpenXR host compilation, independent APK assembly, package identity and v2 signature verification. Private log: `D:/VR4ALL-RailChase2-Build/build/quest-build.log`. The former exec session 39968 is complete; no running build remains for handoff.

Signed APK: `D:/VR4ALL-RailChase2-Build/quest/android/app/outputs/apk/release/app-release.apk`, SHA256 `e17963245e9c3bc571742d4e5780637918cd8f93802804997066291a30c9f3c5`. Android identity is `com.vrlightgunhub.rchase2`. Local development certificate SHA256 is `e2106b4d3903d39b59f78e89bc14dd7a31b046060adad82ce540751b1af9fc4f`; its private key remains `private/rchase2.keystore`, alias `rchase2`, outside all packages. Native outputs and hashes are recorded in `build-status.json`.

Canonical local candidate names are `rail-chase-2-vr-quest-0.1.0-dev.apk` and `rail-chase-2-vr-pcvr-0.1.0-dev.zip`. `tools/package.py` prepares them under `D:/VR4ALL-RailChase2-Build/artifacts/initial`, validates full PE import closure/exports, ARM64 ELF/signatures and software-only contents, and writes checksums plus `artifact-validation.json`. Original ROMs, textures, captures, state and signing keys are excluded. Physical testing and native feedback/original-gun limitations still apply to these candidates.

Candidate packaging completed exit 0. APK SHA256 is `e17963245e9c3bc571742d4e5780637918cd8f93802804997066291a30c9f3c5`; PCVR ZIP SHA256 is `4cecd1fc26ae64b54408eda665a3ac3ec1ba3d0ec7f94f7d95166401a7e9a09a`. `artifacts/initial/artifact-validation.json` records successful media-free APK/PE validation, v2 signing and untested physical status. The portable ZIP was extracted only into `private/portable-smoke`; with PATH restricted to Windows System32, the packaged engine verified the complete owned native ROM set and exited 0 (one set, one OK). This establishes packaged runtime dependency closure, not physical OpenXR play. The first standalone packager attempt lacked JAVA_HOME; setting the same Java 21 environment as the APK build resolved it.

## Remaining gates and unsupported features



This is not release-ready or physically verified VR. Physical haptics/recoil/muzzle feedback, original game-gun extraction, numerical health/lives, exact automatic service-page/calibration detection, physical death/continue play, physical controller accuracy, Quest performance/audio and Windows OpenXR headset play remain unverified. Native projectile acceptance now has the separate continuation evidence above. P2-only continuation ownership is covered by software policy using native played-seat history, but actual death/continue play is still pending. Native Service entry and raw gun-control mode exist; scripted INPUT TEST/AIM navigation is rejected until verified for this game. Retain original emulation clocks/raster/audio; no Gunblade-specific optimization is enabled. External telemetry transport/exposure remains explicitly deferred while the native motion-board read/write seam is preserved.

