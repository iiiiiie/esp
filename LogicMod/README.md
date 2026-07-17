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
- Lua to Blueprint: `PalworldResourceESP_SetDisplayStyle`

`SetTarget` appends one already accepted wild Pal plus its normalized snapshot level and rounded snapshot distance. The overlay keeps target, level, distance, Blueprint-normalized gender, Blueprint-normalized Lucky state, and Blueprint nickname in six same-order arrays. Every synchronization clears all arrays first, so metadata cannot drift between targets. `ClearTarget` empties all six arrays without recreating the overlay, and `ResetSession` removes the overlay before a new world session. The overlay uses one `OnPaint` pass to filter and project every accepted target up to `MAX_DISPLAY_TARGETS`. Level uses the accepted Entity Core value; visible distance is recalculated from the live player and target positions during painting. Name, level, distance, and the top guide can be shown independently. UE 5.1's Widget `DrawText` has no outline input, so each visible label uses four 1px black offset passes followed by the white foreground pass.

`PostBeginPlay` must remain passive. Runtime testing showed that a packaged Blueprint call to a UE4SS custom event with `self` crashes during startup, while Lua discovery followed by Lua-to-Blueprint calls survives gameplay, return to title, and normal exit.

Blueprint receives only `PalMonsterCharacter` instances already accepted by the Lua acquisition boundary. It must not scan actors, enumerate players, or accept a player object from another path.

Gender enum access remains entirely inside Blueprint. The overlay normalizes `EPalGenderType` to `ESP_GenderDiagnosticCode` (`0=unknown`, `1=male`, `2=female`), and `ModActor` exposes only that integer as `ESP_BridgeGenderDiagnosticCode`. Lua maps the integer to diagnostic text; it must not read the enum or the compatibility `FString` property.

Lucky classification also remains entirely inside Blueprint. After Lua has admitted a wild Pal, the overlay calls `UPalIndividualCharacterParameter::IsRarePal()` and records `-1=unknown`, `0=normal`, or `1=Lucky`. Restricted Lucky modes reject unknown values before projection. Lucky is not an alias for Alpha/Boss; Alpha/Boss requires a separate validated classifier.

Dead targets are filtered at both lifecycle speeds: Lua rejects `PalCharacterParameterComponent:IsDead()` during snapshot rebuild, and the Widget calls `PalUtility::IsDead` before drawing each frame. Capture/ownership transitions are removed by the next classification snapshot.

## Panel Contract

`Shift+Y` is registered by Lua and calls `PalworldResourceESP_TogglePanel` on the passive `ModActor`. The panel is Chinese by default and can switch its labels to English. It owns no discovery or player-classification logic.

The key callback defers the Blueprint call by 50 ms. This keeps panel removal and input-mode restoration outside UE4SS's active key-dispatch stack; an immediate second toggle produced a native access violation on the first Steam panel run.

Opening the panel switches the controller to `UIOnlyEx`, focuses the panel, shows the cursor, and flushes pending input. Closing removes the panel, hides the cursor, and restores `GameOnly` with another input flush.

The panel writes only scalar properties: `ESP_RuntimeEnabled`, `ESP_ProfileId`, `ESP_PresetId`, `ESP_LanguageId`, `ESP_LevelMin`, `ESP_LevelMax`, `ESP_DistanceMax`, `ESP_DisplayTargetLimit`, `ESP_ShowTopGuideLine`, `ESP_ShowName`, `ESP_ShowLevel`, `ESP_ShowDistance`, `ESP_GenderFilterId`, `ESP_LuckyFilterId`, `ESP_CaptureRequested`, and the monotonically increasing `ESP_ControlRevision`. Numeric settings use a Slider plus an exact integer SpinBox. Dragging updates only the visible number; mouse/controller release or SpinBox value commit applies one scalar revision. Level zero means no bound, distance is fixed to a 0-330m range, and visible targets are limited to 1-100. Reopening the panel initializes numeric, toggle, gender, Lucky, and language state from the passive `ModActor`, while Lua continues to poll the scalar revision every 250 ms. Display style changes use an actor-free bridge event. Query changes rebuild a fresh snapshot instead of dereferencing Actor wrappers retained by an older scan. Blueprint never initiates a Lua callback, because that event direction caused a reproducible startup crash in the earlier bridge spike.

Functional panel settings are persisted as versioned, complete, whitelisted snapshots in `Scripts/user-settings.log`. The reader accepts strict `v1` and `v2`; new writes use `v2`, and a `v1` record defaults Lucky to all. Writes are coalesced for 750 ms; capture state, player data, entity data, and coordinates are never stored. See ADR-0008.

`FindAllOf` Actor wrappers are normalized in the same GameThread callback that discovers them. They are never retained in delayed Lua admission jobs. The previous two-Actor/16-ms chunking experiment produced repeatable GameThread access violations while moving through streamed entities and is retained only in deprecated source comments for rollback history.

The runtime-off mode clears all accepted targets and stops discovery while leaving the panel, key bind, and lifecycle cleanup available. The registry player gate remains before every adapter and cannot be changed through panel state.

## Packaging

- Package the assigned non-zero chunk from the pinned PMK.
- Rename the produced chunk to `PalworldResourceESP.pak`.
- Install it under `Pal/Content/Paks/LogicMods`.
- Never add the `_P` suffix; that suffix is reserved for patch paks.
- The panel checkpoint contains exactly ten mounted asset files: five `.uasset` and five `.uexp` files, with no runtime DLL.

See `.claude/PRPs/plans/blueprint-bridge-spike.plan.md` and ADR-0002 for the pinned build environment and validation requirements.
