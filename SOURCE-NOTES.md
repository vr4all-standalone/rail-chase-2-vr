# Rail Chase 2 VR development

Original Rail Chase 2 Revision A (`rchase2`), MAME Model 2B, for Quest 3/3S and Windows OpenXR. This is local development. Quest3S gameplay works, and the user accepted the version4 FPS/audio update. Windows OpenXR physical play and publication remain untested. See [validation](docs/validation.md) for precise build and evidence status.

Supply owned `rchase2.zip` and `segabill.zip`, or one complete `rchase2.zip` containing all 23 manifest payloads. Older 21-payload emulator sets lack the Z80 I/O firmware. No ROMs, game graphics or private captures are included. See [platform notes](docs/platform-notes.md).

The separate adapter preserves native reversed eight-bit ADC axes, actual P1/P2 cabinet buttons and operator-configured native initial/join/continue pricing. Native aiming uses each player's actual committed calibration. Shot feedback uses fingerprint-guarded original projectile creation and delivered hand ownership; physical feedback remains untested. No reload action, ammunition counter or original gun model is invented. Original-gun extraction remains an explicit limitation. External telemetry is deferred as requested; the native motion-board behavior remains intact.

Current local additions: `Cart bottom` closes the open native cart with an optional deeper timber interior. `Reticle` selects original In-game artwork, a hollow Custom marker, or None. The marker follows the gun-ray endpoint in stereo; minimum size and shrink distance control its apparent size at distance. Laser is independent. Menu/service pointers remain available. Unknown native artwork and bitmap fallback are preserved where original markers cannot be isolated safely. These additions require their own headset appearance check; see the dated validation results.

Run `tools/test.ps1` for production-policy, media and patch checks. `tools/build-engine.ps1 -MsysRoot D:/msys64 -PythonPath D:/Python314/python.exe -WorkEngine D:/VR4ALL-RailChase2-Build/engine -Platform Windows -Jobs 2` builds the pinned engine in its own compiler checkout. `tools/build-quest.ps1 -CheckOnly -Jobs 2` checks the Android toolchain; omit `-CheckOnly` to build. Neither script installs to a headset or publishes.

Small source files remain here. Compiler outputs, canonical private media, signing identity and captures belong to `D:/VR4ALL-RailChase2-Build`; `build/`, `private/` and `research/` are local junctions and excluded from source packages. [Design](docs/design.md), [plan](docs/implementation-plan.md) and [attribution](CREDITS.md) describe the boundaries and provenance.
