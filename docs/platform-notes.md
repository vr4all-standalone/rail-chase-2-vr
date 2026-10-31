# Platform notes

Independent Android application ID `com.vrlightgunhub.rchase2`, native library `librchase2_xr.so`, Windows executable `rchase2.exe` and host `rchase2_xr.dll`. Native engine/host ABI `0x52433201`. Quest 3/3S uses Android ARM64, API 29 minimum and NDK 29.0.14206865; PC uses Windows AMD64 and the selected OpenXR runtime.

Use original Rail Chase 2 Revision A `rchase2.zip` with all 23 manifest payloads, including `epr-17895.ic8` I/O firmware. No CHD or Model 2 CPU-board table bundle is required. The separately declared billboard device additionally requires epr-18022.ic2, supplied in segabill.zip or the complete archive. Older Model 2 emulator archives with only 21 payloads are incomplete for pinned MAME. Setup validates each payload by size, CRC32 and SHA1.

Preferred Quest path: `/sdcard/Android/data/com.vrlightgunhub.rchase2/files/roms/rchase2.zip`. Setup creates the app-owned folder. Windows uses package `roms/rchase2.zip`. Settings/NVRAM/configuration are private writable state, retained independently of game data. Software-only packages omit ROMs, original textures/captures, saved state, signing keys and private diagnostics.

Native readiness/players/pricing, committed calibration and original projectile acceptance are fingerprint guarded and backed by native observations. Feedback requires confirmed native events and delivered hand ownership; physical feedback is untested. Original-gun extraction remains unsupported. Compilation and desktop simulation alone do not establish physical VR playability.
