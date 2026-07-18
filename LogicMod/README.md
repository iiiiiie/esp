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

`SetTarget` appends one already accepted wild Pal plus its normalized snapshot level and rounded snapshot distance. The overlay keeps target, level, distance, Blueprint-normalized gender, Lucky state, Boss state, element mask, HP IV, attack IV, defense IV, and Blueprint nickname in eleven same-order arrays. Every synchronization clears all arrays first, so metadata cannot drift between targets. `ClearTarget` empties all eleven arrays without recreating the overlay, and `ResetSession` removes the overlay before a new world session. The overlay uses one `OnPaint` pass to filter and project every accepted target up to `MAX_DISPLAY_TARGETS`. Level uses the accepted Entity Core value; visible distance is recalculated from the live player and target positions during painting. Name, level, distance, IVs, and the top guide can be shown independently. UE 5.1's Widget `DrawText` has no outline input, so each visible label uses four 1px black offset passes followed by the white foreground pass.

`PostBeginPlay` must remain passive. Runtime testing showed that a packaged Blueprint call to a UE4SS custom event with `self` crashes during startup, while Lua discovery followed by Lua-to-Blueprint calls survives gameplay, return to title, and normal exit.

Blueprint receives only `PalMonsterCharacter` instances already accepted by the Lua acquisition boundary. It must not scan actors, enumerate players, or accept a player object from another path.

Gender enum access remains entirely inside Blueprint. The overlay normalizes `EPalGenderType` to `ESP_GenderDiagnosticCode` (`0=unknown`, `1=male`, `2=female`), and `ModActor` exposes only that integer as `ESP_BridgeGenderDiagnosticCode`. Lua maps the integer to diagnostic text; it must not read the enum or the compatibility `FString` property.

Lucky and Alpha/Boss classification remain separate and entirely inside Blueprint. After Lua has admitted a wild Pal, the overlay calls `UPalIndividualCharacterParameter::IsRarePal()` for Lucky. For Boss it reads `GetCharacterID()`, resolves the character database through `UPalUtility::GetDatabaseCharacterParameter()`, and calls `GetIsBoss()`. Each provider records `-1=unknown`, `0=normal`, or `1=matching`; restricted modes reject unknown values before projection. Lucky never implies Boss.

Element classification also remains entirely inside Blueprint. The overlay calls `HasElementType()` for Normal, Fire, Water, Leaf, Electricity, Ice, Earth, Dark, and Dragon, then stores one bit mask per already admitted target (`1..256`). No selected element means all; multiple selections use match-any semantics. An unavailable provider stores `-1`, which fails closed whenever an element filter is active. Lua receives and persists only the nine panel booleans and combines them into the scalar `0..511` filter mask.

IV access remains entirely inside Blueprint. For each already admitted target, the overlay calls `GetSaveParameter()`, breaks the typed `FPalIndividualCharacterSaveParameter`, and converts `Talent_HP`, `Talent_Shot`, and `Talent_Defense` from bytes to integers. An unavailable provider stores `-1` rather than fabricating zero. The optional label is `IV HP x / ATK y / DEF z`; it is hidden unless all three values are known. Enhancement ranks such as `Rank_HP`, `Rank_Attack`, and `Rank_Defence` are not IVs and are not used. Minimum-threshold filtering remains deferred until ordinary-Pal runtime validation proves these displayed values.

Dead targets are filtered at both lifecycle speeds: Lua rejects `PalCharacterParameterComponent:IsDead()` during snapshot rebuild, and the Widget calls `PalUtility::IsDead` before drawing each frame. Capture/ownership transitions are removed by the next classification snapshot.

## Panel Contract

`Shift+Y` is registered by Lua and calls `PalworldResourceESP_TogglePanel` on the passive `ModActor`. The panel is Chinese by default and can switch its labels to English. It owns no discovery or player-classification logic.

The key callback defers the Blueprint call by 50 ms. This keeps panel removal and input-mode restoration outside UE4SS's active key-dispatch stack; an immediate second toggle produced a native access violation on the first Steam panel run.

Opening the panel switches the controller to `UIOnlyEx`, focuses the panel, shows the cursor, and flushes pending input. Closing removes the panel, hides the cursor, and restores `GameOnly` with another input flush.

The panel writes only scalar properties: `ESP_RuntimeEnabled`, `ESP_ProfileId`, `ESP_PresetId`, `ESP_LanguageId`, `ESP_LevelMin`, `ESP_LevelMax`, `ESP_DistanceMax`, `ESP_DisplayTargetLimit`, `ESP_ShowTopGuideLine`, `ESP_ShowName`, `ESP_ShowLevel`, `ESP_ShowDistance`, `ESP_ShowIV`, `ESP_GenderFilterId`, `ESP_LuckyFilterId`, `ESP_BossFilterId`, the nine `ESP_Element*` booleans, `ESP_CaptureRequested`, and the monotonically increasing `ESP_ControlRevision`. Numeric settings use a Slider plus an exact integer SpinBox. Dragging updates only the visible number; mouse/controller release or SpinBox value commit applies one scalar revision. Level zero means no bound, distance is fixed to a 0-330m range, and visible targets are limited to 1-100. Reopening the panel initializes numeric, toggle, gender, Lucky, Boss, element, IV-visibility, and language state from the passive `ModActor`, while Lua continues to poll the scalar revision every 250 ms. Display style changes use an actor-free bridge event. Query changes rebuild a fresh snapshot instead of dereferencing Actor wrappers retained by an older scan. Blueprint never initiates a Lua callback, because that event direction caused a reproducible startup crash in the earlier bridge spike.

Functional panel settings are persisted as versioned, complete, whitelisted snapshots in `Scripts/user-settings.log`. The reader accepts strict `v1` through `v5`; new writes use `v5`. A `v1` record defaults Lucky and Boss to all, `v2` defaults Boss to all, `v1` through `v3` default every element toggle to unselected, and `v1` through `v4` default IV display to disabled. The runtime first resolves the settings path from the script source and falls back to locating sibling `config.lua` through `package.searchpath`, which is required on the tested UE4SS runtime. Writes are coalesced for 750 ms; capture state, player data, entity data, and coordinates are never stored. See ADR-0008.

`FindAllOf` Actor wrappers are normalized in the same GameThread callback that discovers them. They are never retained in delayed Lua admission jobs. The previous two-Actor/16-ms chunking experiment produced repeatable GameThread access violations while moving through streamed entities and is retained only in deprecated source comments for rollback history.

The runtime-off mode clears all accepted targets and stops discovery while leaving the panel, key bind, and lifecycle cleanup available. The registry player gate remains before every adapter and cannot be changed through panel state.

## Packaging

- Package the assigned non-zero chunk from the pinned PMK.
- Rename the produced chunk to `PalworldResourceESP.pak`.
- Install it under `Pal/Content/Paks/LogicMods`.
- Never add the `_P` suffix; that suffix is reserved for patch paks.
- The panel checkpoint contains exactly ten mounted asset files: five `.uasset` and five `.uexp` files, with no runtime DLL.

See `.claude/PRPs/plans/blueprint-bridge-spike.plan.md` and ADR-0002 for the pinned build environment and validation requirements.
