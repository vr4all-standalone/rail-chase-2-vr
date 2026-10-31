# Rail Chase 2 VR implementation plan

> **For agentic workers:** Use superpowers:executing-plans to implement these tasks. The user explicitly authorized parallel game agents and continuation of the accepted architecture without another design questionnaire.

**Goal:** A game-specific, tested Rail Chase 2 adapter and supported-platform build candidates, with native/physical limitations reported accurately.

**Architecture:** A clean pinned MAME compiler checkout owns hardware emulation. Selected proven common modules capture native polygons and render per eye; separate Rail Chase 2 policies own ADC/native state and preserve motion-board behavior.

**Tech Stack:** MAME 0.289, C++20, OpenXR/OpenGL ES, Android ARM64, Windows AMD64, Python media/patch tools, PowerShell build tooling.

**Spec:** `docs/design.md`.

## Constraints and review focus

Only this source and `D:/VR4ALL-RailChase2-Build` may be edited. Pin exact source and media identity; no guessed native addresses; no original media in deliveries; jobs at most two; no publication or uncoordinated headset use. Test reversed ADC endpoints/channel ownership, invalid aim, unknown ROM/state, held trigger/start across native acknowledgements, settings migration, and package identities.

### Task 1: Cabinet, media and native baseline

- [x] Write and run failing production-policy tests for Revision A ADC mapping, trigger/system bits and invalid aim.
- [x] Implement `Cabinet`, `in0`, `in1`, `adc` and `map_aim` in `native/cabinet_io.h` using pinned source values.
- [x] Generate exact manifest from pinned ROM declaration; adapt strict verified staging tools and tests.
- [x] Build clean engine with original hardware behavior and isolated state. Run bounded attract/gameplay/service captures from verified supplied media.

### Task 2: Native integration and common VR host

- [x] Write fail-closed native-state tests before adding game observations.
- [x] Import selected stable licensed Model 2 scene/OpenXR/presentation modules, preserving notices and recording source hashes. Adapt only identity and documented native seams.
- [x] Generate game-specific driver hooks with ADC order P2_X/P1_X/P2_Y/P1_Y and a guard for `rchase2`. Keep the native drive-board handlers intact.
- [x] Establish actual native readiness/player/calibration/shot evidence; only implement verified fields. Run production-code transition, settings and renderer checks.

### Task 3: Platforms and validation

- [x] Adapt independent Android identity/storage/manifest and Windows launcher, package validators and dedicated build paths.
- [x] Build Quest and Windows targets with two compiler jobs maximum; verify output ABI, exports, signatures, media-free contents and hashes.
- [x] Record exact commands, exit codes, source hashes, native evidence and untested physical behavior in `docs/validation.md`; report deferred telemetry seam to parent.

Dedicated-chat continuation: native state/pricing/aim, actual stereo replay, native projectile acceptance and both platform builds have software evidence. Original gun extraction, automatic service pages, death/continue/routes and physical checks remain open. See validation.md and the current build-status.json for concrete candidate and evidence identities. Telemetry exposure remains deferred.
