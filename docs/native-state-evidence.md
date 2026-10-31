# Native Rail Chase 2 evidence

Source pin: MAME 0.289 `f34f02505e32c1993c6a782b6814232cbfc74e36`. Original program is the verified Revision A set. Private evidence lives under `D:/VR4ALL-RailChase2-Build/research`; no game bytes or screenshots are part of source or public packages.

## Media and native baseline

`game-set.json` declares 22 game payloads from `model2.cpp` plus the separately declared `segabill.cpp` device firmware. All 23 pass size, CRC32 and SHA1 in `media-complete-with-device.json`. Original archives retain their before/after SHA256 values. The older Emulator v1.1a archive is missing `epr-17895.ic8`; the owned modern `model2/SegaM2/roms/rchase2.zip` supplies it. Existing owned `segabill.zip` supplies `epr-18022.ic2`. No commercial data was downloaded.

The first 22-payload boot correctly failed with missing billboard firmware. After completing the actual device dependency, `native-gameplay` ran 60 native seconds, exit 0, no Lua errors, 179.17% reported headless native speed. `native-service` ran 25 seconds, exit 0 and visibly entered TEST MENU. These runs used the read-only existing pinned Model 2 executable without enabling its Gunblade VR hooks; all writable state was isolated here. They do not prove the new engine patches or headset rendering.

## State and credits

The loaded Revision A program dispatcher at `0x27b4..0x27cc` loads mode `0x501d84` and calls its table at `0x26d0`; submode is `0x501d90`. Direct native observations identify boot 0, warning 1, pricing 8, gameplay 9 and service 14. The Start handler at `0x11fa0..0x120d8` explicitly rejects mode <= 2, service 14, disabled gate `0x501f50` and the native inhibit bit in `0x501d94`. Input eligibility follows those original guards. The active-seat mask is the low two bits of byte `0x501d88`; history counters `0x501f30/34` survive a start and are not substituted for current active seats. Boolean active-seat eligibility is used for the controller display; numerical lives/health are not decoded.

The inherited MAME field names say “1 Player Start” and “2 Players Start”. Actual native behavior is independently verified: `native-dual-credit` supplies two real Coin 1 edges then Start 1 at 20 seconds, producing active mask 1 and FLINT's HUD. Two real Coin 2 edges then Start 2 at 34 seconds produce mask 3 and both FLINT/MARY HUDs. Credits drop from 2 to 0 at each acknowledgement and history becomes `(1,1)`. This establishes independent P1/P2 seat behavior; it does not establish death/continue play testing.

Native routines `0x26d88..0x26e58` and `0x26e60..0x2706c` select price from halfword `0x501d1c` for a fresh first seat, `0x501d28` when the other seat has already played, and `0x501d2a` for that seat's continue. Shared credit balance is halfword `0x1d00022`; free play is bit 3 of byte `0x501d18`. Factory values observed are initial 2, join 2 and continue 1. Production reads the values and bounds non-free-play costs/balance; the defaults are not hardcoded as the protocol. `NativeCreditPolicy` inserts separated real coin edges only while the measured balance is insufficient, starts only after payment, cancels at acknowledgement/focus/menu/session changes, and times out after four seconds. Unknown fingerprints/pricing fail closed.

Eight instruction/address words identify the dispatcher, Start and price routines before state interpretation. Tests cover mismatches, corrupt modes/pricing, free play and active-seat transitions. No program RAM is written by a state reader.

## Calibrated aim

Driver ADC channels are P2 X, P1 X, P2 Y, P1 Y. Native latched bytes are `0x501d30..33` in that order. Calibration min/max is initialized and serviced by native input-test code; committed P1 bytes are `0x501d0f..12`, P2 `0x501d13..16`. Factory values are `c0 40 40 c0` per player. Revision A raw axes decrease left-to-right and top-to-bottom.

Native player initialization at `0x41500` / `0x41610` computes separate positive/negative gains around raw 128. P1 horizontal extents are 186/310, P2 310/186; vertical extents are 192/144. Native reticle routines `0x41720` / `0x41d20` negate raw displacement, apply those piecewise gains and player X offsets -62/+62. The original TGP multiplies pixel-relative coordinates by 6 and divides by 768, producing native target values at `0x501f58..64`. This yields target depth 6, native pixel center `(248,192)`, and pixel-to-target factor 1/128. It is a different transform from Gunblade's angular aiming.

`tools/aim_probe.lua` records real ADC inputs and read-only target outputs in `native-aim-probe/aim.tsv`. The C++ production helper and inverse are checked by `tests/native_aim_evidence_test.cpp`: 2,222 stable native player samples, 50 distinct player/ADC positions, maximum world-coordinate error **0**, and exact ADC round trips. The first read after an ADC change is excluded because the native output belongs to the preceding latch. This establishes the original calibrated target transform. Enemy-hit precision and physical controller calibration remain untested.

## Native projectile creation

Two independent 72-second cabinet traces identify original projectile creation at `0x41a24..0x41c74` (P1) and `0x42038..0x422c8` (P2). The original `stl` reports owner/child at `0x524990/0x524994`; observed CPU PCs after that store are `0x41c7c` and `0x422d0`. The player roots have callbacks `0x3f570` and `0x3fac0`; a newly created projectile has callback `0x423c0`. The original allocator has already linked `owner+0x10 == child` and `child+0x14 == owner`. Cleanup writes the same addresses from PC `0x107b34` and is not a shot.

The research trace includes 6,164 original writes and exactly **517 projectile creations: P1 317, P2 200**. Production `NativeShotJournal` replays all writes with that exact result; all six native gameplay/service screenshots match across both traces. Events are observed individually, preserving simultaneous player creation and reused object addresses. ROM fingerprints, exact instruction source/full-word store, active native player, callback identity and linked ownership must all match. Unknown state or ownership stays closed. Native service and new sessions invalidate/reset the counters; existing delivered-hand ownership and focus/menu/release guards control feedback.

The engine owns the observer and counters outside `SharedRuntime`, preserving ABI `0x52433201` and the existing shot fields. Its scoped tap explicitly unregisters before the observer dies because the MAME handle itself is weak. `native_shot_test.cpp` covers event rejection, counts, resets, differential replay and that teardown lifetime. Evidence is private in `research/native-shot-guard-evidence`. Production live-host and platform validation are recorded separately in `validation.md`.

This establishes native projectile acceptance, not enemy-hit detection or physical haptic/recoil alignment. Physical feedback and controller timing remain untested.

## Unmeasured states

Numeric health/lives, weapon extraction and automatic service-page/calibration activity detection remain unsupported. Native projectile events now have the evidence above; feedback is allowed only through their verified counters and delivered hand ownership. Service controls use native Test/Service/Start and raw calibrated guns; no gun-specific reload or weapon switching is added. External motion telemetry is deferred; the drive-board handlers and native command simulation are unchanged.


## Scene optics and final adapter integration

Eleven fresh captures from the new pinned instrumented engine pass complete resources/foreground checks. Seven gameplay frames contain 1,126-3,540 source polygons; every non-culled world group uses window 1, focus 768/768 and center 248/320 with raster yoffset 128. `native-camera-evidence.json` retains all groups. Camera reference stays at those measured optics and honors an explicit frame lens; a foreign layer's polygon count cannot change world stereo scale.

The complete captured scene path passes production GL neutral and both-eye replay with at least 99.206674% exact neutral pixels, RGB MAE at most 0.0261677 and zero foreground mismatches. Seven frames establish actual world stereo, four startup frames are explicitly native raster. Original and instrumented screenshots match at all fourteen baseline checkpoints.

After final Windows header synchronization, a new engine + host GL simulation with no external Lua gameplay inputs runs 60 native seconds, exits 0 and observes both native players active through the VR adapter's own calibrated input/native-credit policies. This establishes the software bridge, not headset play, native shot acknowledgement or actual death/continue play.
