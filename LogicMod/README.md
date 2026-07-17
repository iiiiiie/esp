# PalworldResourceESP LogicMod Source

This directory stores only project-owned LogicMod assets and documentation. The full Palworld Modding Kit, Unreal Engine, Wwise integration, generated caches, and packaged Windows project remain outside this repository.

## Expected Asset Layout

```text
Content/
  Mods/
    PalworldResourceESP/
      ModActor.uasset
      BP_ESPBridge.uasset
      WBP_ESPOverlay.uasset
      WBP_ESPPanel.uasset
      PalworldResourceESP.uasset
```

The assets will be created in Unreal Engine after the external PMK toolchain compiles successfully. `PalworldResourceESP.uasset` is the Primary Asset Label for a unique non-zero chunk.

The repository is the source of record for the editor-only generator. Sync it into an existing external PMK with an explicit path; the command creates a dated rollback copy before replacing any PMK file:

```powershell
./tools/logicmod/Sync-LogicModTooling.ps1 -PmkRoot <PalworldModdingKit-no-wwise>
```

## Bridge Contract

- Bridge discovery: Lua observes the passive `ModActor_C` through `RegisterBeginPlayPostHook`
- Lua to Blueprint: `PalworldResourceESP_ResetSession`
- Lua to Blueprint: `PalworldResourceESP_SetTarget`
- Lua to Blueprint: `PalworldResourceESP_ClearTarget`
- Lua to Blueprint: `PalworldResourceESP_TogglePanel`

`SetTarget` appends one already accepted wild Pal plus its normalized snapshot level and rounded distance in meters to three same-order overlay arrays. Every synchronization clears the arrays first, so metadata cannot drift between targets. `ClearTarget` empties all three arrays without recreating the overlay, and `ResetSession` removes the overlay before a new world session. The overlay uses one `OnPaint` pass to project every accepted target up to `MAX_DISPLAY_TARGETS`, draw `Lv.<level>  <distance>m` below it, and optionally draw its top guide.

`PostBeginPlay` must remain passive. Runtime testing showed that a packaged Blueprint call to a UE4SS custom event with `self` crashes during startup, while Lua discovery followed by Lua-to-Blueprint calls survives gameplay, return to title, and normal exit.

Blueprint receives only `PalMonsterCharacter` instances already accepted by the Lua acquisition boundary. It must not scan actors, enumerate players, or accept a player object from another path.

Gender enum access remains entirely inside Blueprint. The overlay normalizes `EPalGenderType` to `ESP_GenderDiagnosticCode` (`0=unknown`, `1=male`, `2=female`), and `ModActor` exposes only that integer as `ESP_BridgeGenderDiagnosticCode`. Lua maps the integer to diagnostic text; it must not read the enum or the compatibility `FString` property.

Dead targets are filtered at both lifecycle speeds: Lua rejects `PalCharacterParameterComponent:IsDead()` during snapshot rebuild, and the Widget calls `PalUtility::IsDead` before drawing each frame. Capture/ownership transitions are removed by the next classification snapshot.

## Panel Contract

`Shift+Y` is registered by Lua and calls `PalworldResourceESP_TogglePanel` on the passive `ModActor`. The panel is Chinese by default and can switch its labels to English. It owns no discovery or player-classification logic.

The key callback defers the Blueprint call by 50 ms. This keeps panel removal and input-mode restoration outside UE4SS's active key-dispatch stack; an immediate second toggle produced a native access violation on the first Steam panel run.

Opening the panel switches the controller to `UIOnlyEx`, focuses the panel, shows the cursor, and flushes pending input. Closing removes the panel, hides the cursor, and restores `GameOnly` with another input flush.

The panel writes only scalar properties: `ESP_RuntimeEnabled`, `ESP_ProfileId`, `ESP_PresetId`, `ESP_LevelMin`, `ESP_LevelMax`, `ESP_DistanceMin`, `ESP_DistanceMax`, `ESP_DisplayTargetLimit`, `ESP_ShowTopGuideLine`, `ESP_CaptureRequested`, and the monotonically increasing `ESP_ControlRevision`. Level and distance use integer SpinBox range inputs; zero means no bound. The display target limit uses an integer SpinBox from 1 through 512. Reopening the panel initializes all inputs from the passive `ModActor`, while Lua continues to poll the scalar revision every 250 ms. The top-guide style switch updates the existing Blueprint overlay through the same filtered target resynchronization path. Blueprint never initiates a Lua callback, because that event direction caused a reproducible startup crash in the earlier bridge spike.

The runtime-off mode clears all accepted targets and stops discovery while leaving the panel, key bind, and lifecycle cleanup available. The registry player gate remains before every adapter and cannot be changed through panel state.

## Packaging

- Package the assigned non-zero chunk from the pinned PMK.
- Rename the produced chunk to `PalworldResourceESP.pak`.
- Install it under `Pal/Content/Paks/LogicMods`.
- Never add the `_P` suffix; that suffix is reserved for patch paks.
- The panel checkpoint contains exactly ten mounted asset files: five `.uasset` and five `.uexp` files, with no runtime DLL.

See `.claude/PRPs/plans/blueprint-bridge-spike.plan.md` and ADR-0002 for the pinned build environment and validation requirements.
