# Blueprint Toolchain Report

## Status

`PHASE 2 COMPLETE - Wwise-free PMK build, assets, package, and Steam smoke matrix verified`

## Recorded Environment

Date: 2026-07-15

| Component | Required | Current result |
|---|---|---|
| Palworld | Steam PC build `24088745` at Phase 1 test time | Phase 2 gate passed on build `24181527` |
| UE4SS | v3.0.1 Beta #0, Git SHA `c2ac2464` | Installed and Phase 1 tested |
| PMK | `localcc/PalworldModdingKit` pinned to `62fad4130238cb0aadf024b87496e7387d5f4bf5` | Cloned externally and verified at the pinned detached HEAD |
| Epic Games Launcher | Launcher capable of installing UE 5.1 | Installed through `winget`; UE account login completed |
| Unreal Engine | 5.1.x | Installed: 5.1.1 changelist `23901901` under `D:/Epic Games/UE_5.1` |
| .NET runtime | 6.x x64 | Installed: `Microsoft.NETCore.App 6.0.11` and `Microsoft.WindowsDesktop.App 6.0.11` |
| Visual Studio | VS 2022 C++ build environment | Build Tools 2022 `17.14.36` installed under `D:/Microsoft Visual Studio/2022/BuildTools` |
| MSVC toolset | 14.38 / VS 17.8 | Installed and verified: `14.38.33130`; newer toolsets remain installed |
| Windows SDK | Windows SDK for Win64 builds | Installed: 10.0.26100.0 |
| Wwise | Upstream PMK normally expects 2021.1.11 | Not installed; ADR-0003 accepts a local no-op `AkAudio` compatibility module for this non-audio ESP |
| LogicMod loader | UE4SS BPModLoaderMod | Installed and loading existing LogicMods |

## Local Runtime Evidence

- Active LogicMod directory: `<Palworld>/Pal/Content/Paks/LogicMods`.
- Existing packages include Paldar, PalAnalyzer, MapCollectablesMod, integratedStorage, and DekModConfigMenu.
- BPModLoader requires `/Game/Mods/<ModName>/ModActor` by default and the `.pak` filename must match the Mod folder name.
- `PalworldResourceESP.pak` must not use the `_P` patch-pak suffix.

## Source Evidence

- Community documentation: `PalworldModding/Docs` current `master` branch on 2026-07-15.
- PMK repository: `https://github.com/localcc/PalworldModdingKit`.
- PMK head observed: `62fad4130238cb0aadf024b87496e7387d5f4bf5`, dated 2026-07-11.
- External PMK path: an untracked sibling `tooling/PalworldModdingKit` checkout.
- PMK `Pal.uproject` declares `EngineAssociation: 5.1` and enables Wwise, CommonUI, CommonGame, and other game-facing plugins.
- PMK has no Git submodules and is approximately 7.5 MB in GitHub repository metadata before generated/build files and external Wwise integration.
- No-Wwise PMK copy: an untracked sibling `tooling/PalworldModdingKit-no-wwise` checkout.
- The copy disables the Wwise plugin and loads `Plugins/AkAudioStub`, a no-op module that exposes only the Pal declarations required by UHT/UBT.
- `PalEditor Win64 Development` passed UHT and linked `Pal.dll`, `PalModLoader.dll`, and `AkAudio` using the default MSVC 14.39 toolchain. A headless editor run mounted `AkAudioStub` without a module-initialization error.

## Required Human Interaction

- Epic Games account login and Unreal Engine 5.1 installation were completed by the maintainer.
- Audiokinetic account login is not required for the current Wwise-free ESP build path.
- The first automated attempts to add MSVC 14.38 made no changes at the UAC boundary. The maintainer then installed the component through the visible Visual Studio Installer, and both the component ID and `cl.exe` path were verified.

The maintainer completed the required Steam single-player runs. Multiplayer remains community-pending.

## Outcome

1. `ModActor`, `BP_ESPBridge`, and `WBP_ESPOverlay` were generated through the accepted editor-only automation path.
2. The project-owned assets were copied back to `LogicMod/Content/Mods/PalworldResourceESP` with dated rollback snapshots.
3. The original Phase 2 package contained eight correctly mounted files. The 2026-07-17 panel checkpoint adds `WBP_ESPPanel`, cooks five source assets into exactly ten mounted files, and still contains no runtime DLL.
4. The Phase 2 matrix passed, including multi-target guides, Blueprint-local gender normalization, capture/death cleanup, `candidate_player_count=0`, return to Title, and normal exit.
5. The remaining toolchain caveat is that UE 5.1 UBT selected MSVC 14.39 even though the pinned 14.38 family is installed.
