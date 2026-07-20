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
      WBP_ESPPassiveEntry.uasset
      WBP_ESPPassiveTooltip.uasset
      WBP_ESPPalEntry.uasset
      DT_ESPRichTextStyle.uasset
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

`SetTarget` appends one already accepted wild Pal plus its normalized snapshot level and rounded snapshot distance. The overlay keeps target, level, distance, Blueprint-normalized gender, Lucky state, Boss state, element mask, HP IV, attack IV, defense IV, Blueprint nickname, localized passive-skill text, and delimiter-safe passive IDs in thirteen same-order arrays. Every synchronization clears all arrays first, so metadata cannot drift between targets. `ClearTarget` empties all thirteen arrays without recreating the overlay, and `ResetSession` removes the overlay before a new world session. The overlay uses one `OnPaint` pass to filter and project every accepted target up to `MAX_DISPLAY_TARGETS`. Level uses the accepted Entity Core value; visible distance is recalculated from the live player and target positions during painting. Name, level, distance, IVs, passive skills, and the top guide can be shown independently. Level, distance, and IV field labels are Chinese by default and switch as one language with the panel. UE 5.1's Widget `DrawText` has no outline input, so each visible label uses four 1px black offset passes followed by the white foreground pass.

`PostBeginPlay` must remain passive. Runtime testing showed that a packaged Blueprint call to a UE4SS custom event with `self` crashes during startup, while Lua discovery followed by Lua-to-Blueprint calls survives gameplay, return to title, and normal exit.

Blueprint receives only `PalMonsterCharacter` instances already accepted by the Lua acquisition boundary. It must not scan actors, enumerate players, or accept a player object from another path.

Gender enum access remains entirely inside Blueprint. The overlay normalizes `EPalGenderType` to `ESP_GenderDiagnosticCode` (`0=unknown`, `1=male`, `2=female`), and `ModActor` exposes only that integer as `ESP_BridgeGenderDiagnosticCode`. Lua maps the integer to diagnostic text; it must not read the enum or the compatibility `FString` property.

Lucky and Alpha/Boss classification remain separate and entirely inside Blueprint. After Lua has admitted a wild Pal, the overlay calls `UPalIndividualCharacterParameter::IsRarePal()` for Lucky. For Boss it reads `GetCharacterID()`, resolves the character database through `UPalUtility::GetDatabaseCharacterParameter()`, and calls `GetIsBoss()`. Each provider records `-1=unknown`, `0=normal`, or `1=matching`; restricted modes reject unknown values before projection. Lucky never implies Boss.

Element classification also remains entirely inside Blueprint. The overlay calls `HasElementType()` for Normal, Fire, Water, Leaf, Electricity, Ice, Earth, Dark, and Dragon, then stores one bit mask per already admitted target (`1..256`). No selected element means all; multiple selections use match-any semantics. An unavailable provider stores `-1`, which fails closed whenever an element filter is active. Lua receives and persists only the nine panel booleans and combines them into the scalar `0..511` filter mask.

IV access remains entirely inside Blueprint. For each already admitted target, the overlay calls `GetSaveParameter()`, breaks the typed `FPalIndividualCharacterSaveParameter`, and converts `Talent_HP`, `Talent_Shot`, and `Talent_Defense` from bytes to integers. An unavailable provider stores `-1` rather than fabricating zero. The optional label is `个体值 生命 x / 攻击 y / 防御 z` in Chinese and `IV HP x / ATK y / DEF z` in English; it is hidden unless all three values are known. Enhancement ranks such as `Rank_HP`, `Rank_Attack`, and `Rank_Defence` are not IVs and are not used. HP, attack, and defense each have an independent 0-100 minimum; zero disables that dimension, and every enabled dimension must match.

Passive-skill access remains entirely inside Blueprint. The overlay reads `GetPassiveSkillList()` only after Lua admission, resolves each ID through `PalUIUtility::GetPassiveSkillName()`, and joins the current-language names for optional display. The panel obtains the selectable catalog and rank metadata from the game's passive-skill manager; hovering an entry shows the localized `SkillDesc`. Selected IDs remain a Blueprint `TArray<FName>` capped at four entries. No selection shows all targets, while multiple selections use AND and require every selected ID. Lua receives only a revision integer and never receives the source or selected name arrays or any field-derived Actor reference.

The Pal species catalog also remains entirely inside Blueprint. Its page is nested under Filters and enumerates the running game's `/Game/Pal/DataTable/Character/DT_PalMonsterParameter` rows to derive the current Paldex number, localized name, elements, work suitability ranks, soft icon reference, base and movement statistics, food amount, partner skill text/effect, and defeat drops. It does not ship a versioned Pal snapshot. Entries default to Paldex-number ascending and can be reloaded and sorted by Paldex number, sprint speed, or any of the 12 player-visible work suitabilities with explicit ascending/descending direction; filtering, rank display, and sorting use one shared field definition, and internal Oil Extraction data is not exposed. Partner effects use the game's own baseline-rank formatter, `GetFormatedFirstActivatedInfoTextFixedRank`, which is also called by the original Paldex and Pal-box UI; the Mod does not guess localization keys or treat `OverridePartnerSkillDescTextID` as a complete source. The formatter's pipe-delimited rich-text attributes are normalized to standard UE 5.1 quoted attributes immediately before the current game's styles and decorators render them. Distinct CharacterID variants are retained and shown as an ID suffix, so same-name entries with different drops remain diagnosable. Localized-name or number search plus element and work queries narrow the catalog with AND semantics. Selected cards use a high-contrast state and a localized selected-name summary. Selected species use OR semantics against already admitted world targets; no selected species means all. Lua persists only validated species IDs and never receives DataTable rows, icons, or actor references. See ADR-0009.

Dead targets are filtered at both lifecycle speeds: Lua rejects `PalCharacterParameterComponent:IsDead()` during snapshot rebuild, and the Widget calls `PalUtility::IsDead` before drawing each frame. Capture/ownership transitions are removed by the next classification snapshot.

## Panel Contract

`Shift+Y` is registered by Lua and calls `PalworldResourceESP_TogglePanel` on the passive `ModActor`. The panel is Chinese by default and can switch its labels to English. It owns no discovery or player-classification logic.

The key callback records one pending request and returns. The existing 250 ms GameThread runtime tick consumes it only when reconciliation is idle, performs no discovery in the same tick, and skips requests from an older lifecycle. This keeps panel mutation outside UE4SS key dispatch and prevents a competing delayed GameThread callback during synchronous scanning.

Opening the panel switches the controller to `UIOnlyEx`, focuses the panel, shows the cursor, flushes pending input, and sets the Pal SDK's Mod-owned `PalworldResourceESP_Panel` input-disable flag so A-Z text entry cannot trigger game shortcuts. Unconsumed panel key-down/up events return `Handled`; focused text fields still receive editing input first. Closing clears only that flag before removing the panel, hides the cursor, and restores `GameOnly` with another input flush.

The panel writes scalar properties for normal settings plus Blueprint-owned passive include/exclude and species-selection arrays. The scalar contract includes `ESP_RuntimeEnabled`, profile/preset/language, level endpoints, maximum distance, display limit, visibility toggles, three IV minima, gender/Lucky/Boss, nine element booleans, capture state, filter revisions, and `ESP_ControlRevision`. Numeric settings use a Slider plus an exact integer SpinBox. Dragging updates only the visible number; mouse/controller release or SpinBox value commit applies one scalar revision. Level zero means no bound, distance is fixed to a 0-330m range, each IV minimum is 0-100, and visible targets are limited to 1-100. The panel fills the viewport with a 16px margin and uses two primary tabs: Display and Filters. Filters contains nested General and Pal species pages, with the reset-default action at the top of the filter hierarchy. Reopening initializes controls from the passive `ModActor`, while Lua continues to poll scalar revisions every 250 ms. Binary display controls use an outlined gray/green checkbox so unchecked rows remain visible. Display style changes use an actor-free bridge event. Query changes rebuild a fresh snapshot instead of dereferencing Actor wrappers retained by an older scan. Blueprint never initiates a Lua callback, because that event direction caused a reproducible startup crash in the earlier bridge spike.

Functional settings are persisted as versioned, complete, whitelisted snapshots in `Scripts/user-settings.log`. The reader accepts strict `v1` through `v12`; new writes use `v12`. Legacy records migrate forward without inventing selections. Passive include/exclude IDs, category expansion state, selected species IDs, and the last primary/nested panel page persist across panel recreation, save transitions, and process restarts. Transient catalog search, element/work queries, and capture state are not restored. The runtime first resolves the settings path from the script source and falls back to locating sibling `config.lua` through `package.searchpath`, which is required on the tested UE4SS runtime. Writes are coalesced for 750 ms; player data, entity data, coordinates, catalog rows, and hover details are never stored. See ADR-0008 and ADR-0009.

`FindAllOf` Actor wrappers are normalized in the same GameThread callback that discovers them. They are never retained in delayed Lua admission jobs. The previous two-Actor/16-ms chunking experiment produced repeatable GameThread access violations while moving through streamed entities and is retained only in deprecated source comments for rollback history.

The runtime-off mode clears all accepted targets and stops discovery while leaving the panel, key bind, and lifecycle cleanup available. The registry player gate remains before every adapter and cannot be changed through panel state.

## Packaging

- Package the assigned non-zero chunk from the pinned PMK.
- Rename the produced chunk to `PalworldResourceESP.pak`.
- Install it under `Pal/Content/Paks/LogicMods`.
- Never add the `_P` suffix; that suffix is reserved for patch paks.
- The runtime Pal-catalog checkpoint contains exactly eighteen mounted asset files: nine `.uasset` and nine `.uexp` files, with no runtime DLL.

See `.claude/PRPs/plans/blueprint-bridge-spike.plan.md` and ADR-0002 for the pinned build environment and validation requirements.
