# Attribution

MAME 0.289, commit f34f02505e32c1993c6a782b6814232cbfc74e36, supplies original hardware emulation. Its full license is retained as LICENSE and individual upstream licenses apply. Model 2 driver/video credits include R. Belmont, Olivier Galibert, ElSemi, Angelo Salese and Matthew Daniels.

Selected Model 2 capture, renderer, OpenXR, controller, VR4ALL purple presentation, settings and platform helpers are adapted from the local licensed VR4ALL Gunblade NY/VC2 foundation. Exact imported files and source hashes are recorded in docs/common-provenance.json. Selected generic UART patches are adapted independently and checked against Rail Chase2 native execution; no complete sibling engine tree or compiler products are copied. Original per-file notices remain. New adapter code is GPL-2.0-or-later. VR4ALL common components credit mikermak and the contributors retained in their files.

Third-party notices are in licenses/ and authored gun/controller asset credits in assets/models/CREDITS.txt. Arcade and Zapper are authored alternatives and are never described as an extracted Rail Chase 2 weapon.

The independent audio output worker uses SoundTouch2.4.1 by Olli Parviainen and contributors, LGPL-2.1-or-later. The selected unmodified streaming library source, full upstream license and pinned hashes are in vendor/soundtouch; its license is also in licenses/SoundTouch.txt. Native MAME synthesis and WAV recording remain upstream of the output worker. The UART event batching policy and native-reticle endpoint rendering reuse selected licensed VR4ALL GunbladeNY helpers with Rail Chase2-specific validation.

Rail Chase 2 commercial game data, firmware, artwork and trademarks belong to their owners and are not included. Development reads user-owned verified media; no commercial media is downloaded or shipped.
