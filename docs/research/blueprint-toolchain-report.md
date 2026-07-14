# Blueprint Toolchain Report

## Status

`BOOTSTRAP IN PROGRESS - account-gated Unreal and Wwise prerequisites remain`

## Recorded Environment

Date: 2026-07-15

| Component | Required | Current result |
|---|---|---|
| Palworld | Steam PC build `24088745` at Phase 1 test time | Installed |
| UE4SS | v3.0.1 Beta #0, Git SHA `c2ac2464` | Installed and Phase 1 tested |
| PMK | `localcc/PalworldModdingKit` pinned to `62fad4130238cb0aadf024b87496e7387d5f4bf5` | Cloned externally and verified at the pinned detached HEAD |
| Unreal Engine | 5.1.x | Not installed; Epic Launcher not installed |
| .NET runtime | 6.x x64 | Installed: `Microsoft.NETCore.App 6.0.11` and `Microsoft.WindowsDesktop.App 6.0.11` |
| Visual Studio | VS 2022 C++ build environment | Build Tools 2022 `17.14.20` installed under `D:/Microsoft Visual Studio/2022/BuildTools` |
| MSVC toolset | 14.38 / VS 17.8 | Missing; installed versions are 14.16, 14.40, and 14.44 |
| Windows SDK | Windows SDK for Win64 builds | Installed: 10.0.26100.0 |
| Wwise | 2021.1.11 SDK and Unreal offline integration | Not installed |
| LogicMod loader | UE4SS BPModLoaderMod | Installed and loading existing LogicMods |

## Local Runtime Evidence

- Active LogicMod directory: `E:/Steam/steamapps/common/Palworld/Pal/Content/Paks/LogicMods`.
- Existing packages include Paldar, PalAnalyzer, MapCollectablesMod, integratedStorage, and DekModConfigMenu.
- BPModLoader requires `/Game/Mods/<ModName>/ModActor` by default and the `.pak` filename must match the Mod folder name.
- `PalworldResourceESP.pak` must not use the `_P` patch-pak suffix.

## Source Evidence

- Community documentation: `PalworldModding/Docs` current `master` branch on 2026-07-15.
- PMK repository: `https://github.com/localcc/PalworldModdingKit`.
- PMK head observed: `62fad4130238cb0aadf024b87496e7387d5f4bf5`, dated 2026-07-11.
- External PMK path: `E:/AAA_qian/ji_ji_tui_jin/palworld_mod/tooling/PalworldModdingKit`.
- PMK `Pal.uproject` declares `EngineAssociation: 5.1` and enables Wwise, CommonUI, CommonGame, and other game-facing plugins.
- PMK has no Git submodules and is approximately 7.5 MB in GitHub repository metadata before generated/build files and external Wwise integration.

## Required Human Interaction

- Epic Games account login is required to install Unreal Engine 5.1 through Epic Games Launcher.
- Audiokinetic account login is required to install Wwise 2021.1.11 SDK and obtain offline Unreal integration files.
- The first automated attempt to add MSVC 14.38 made no changes because the installer was not elevated. The corrected elevated attempt was canceled at the UAC boundary. This component must be confirmed through a visible elevated Visual Studio Installer session.

The maintainer does not need to enter Palworld again until the PMK compiles, the LogicMod packages, and the bridge is deployed.

## Next Actions

1. Add MSVC 14.38 to the existing Build Tools installation through an elevated visible installer session.
2. Install Epic Games Launcher, then request maintainer login and UE 5.1 installation to the large-capacity drive.
3. Install Audiokinetic Launcher, then request maintainer login and Wwise 2021.1.11 components.
4. Integrate Wwise into PMK and launch the unmodified project.
5. Only after the clean PMK launch succeeds, create project-owned Blueprint assets.
