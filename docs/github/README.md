<div align="center">

<img src="docs/github/readme-banner.png" alt="Rail Chase 2 VR — Sega promotional poster artwork, standalone Meta Quest and PC VR" width="100%" />

# Rail Chase 2 VR
### Sega Model 2B Arcade Rail Shooter · Standalone Meta Quest & PC VR

![Status: public beta](https://img.shields.io/badge/Release-Public_beta-6f56b6?style=for-the-badge)
![Quest 3 / 3S and Windows OpenXR](https://img.shields.io/badge/Platforms-Quest_3_%2F_3S_%C2%B7_PC_VR_OpenXR-blue?style=for-the-badge)
![Original game required](https://img.shields.io/badge/Your_Game-Rail_Chase_2_Revision_A-orange?style=for-the-badge)

**[Download the Quest APK, Windows OpenXR build, source and checksums](https://github.com/vr4all-standalone/rail-chase-2-vr/releases/tag/v0.1.0-beta)**

**[🌐 VR4ALL — Standalone VR Mod Installer](https://github.com/vr4all-standalone/vr4all-standalone)**  
**[☕ Donate to VR4ALL](https://ko-fi.com/vr4all)**

</div>

---

## The port

Rail Chase 2 VR brings Sega's original ride-on rail shooter into stereo VR with head tracking and tracked guns. It runs locally on Quest 3/3S; a Windows OpenXR build is available in the release. The original game's routes, audio and cabinet logic come from your own game data.

Quest 3S gameplay and audio have been observed in development. Both native player inputs, original targeting artwork, optional custom reticles and the VR4ALL settings menu are implemented. Performance, the latest presentation changes and longer play sessions still need play testing.

## Your game files

Bring your own **Rail Chase 2 Revision A** `rchase2.zip` with all **23 required payloads**. If those files are split, also provide `segabill.zip` for the billboard firmware. Older 21-file sets are incomplete. No CHD is required. No original game files are included in this repository or future software downloads.

## Install

Use [VR4ALL](https://github.com/vr4all-standalone/vr4all-standalone/releases) to choose your complete owned `rchase2.zip`, download the Quest APK and transfer the game file into the app's `files/roms` folder. A split set may also need `segabill.zip`. Open **Rail Chase 2 - VR4ALL** and finish its ROM validation. For Windows OpenXR, extract the PCVR ZIP, put your owned archives in its `roms` folder, and run `run-pcvr.cmd`.

## Development notes

The Quest app name is **Rail Chase 2 - VR4ALL**. Quest 3S gameplay and the latest cart aiming have been observed. Native game speed in the measured scene is about 82–90% of full speed; longer routes, Quest 3 and Windows OpenXR physical play remain unverified. This is an independent fan project, not affiliated with Sega or Meta.

[Original poster and banner attribution](docs/github/ARTWORK.md)
