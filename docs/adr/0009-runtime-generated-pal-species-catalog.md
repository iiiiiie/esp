# ADR-0009: Runtime-Generated Pal Species Catalog

## Status

Proposed

## Date

2026-07-19

## Context

The ESP needs a species-oriented filter that can search the full Pal catalog by localized name, element, and work suitability, then use selected species as world-target filters. Each catalog entry also needs the current Paldex number, icon, work ranks, base statistics, movement values, partner-skill text, food amount, and defeat drops.

A source-controlled snapshot of current Pal names or values would become stale whenever Palworld adds or changes a Pal. Save-file parsing would not provide the authoritative static catalog or current localization. The existing runtime boundary keeps complex game enums, structs, object references, and arrays in Blueprint while Lua handles only safe scalar control data and strict persistence mirrors.

## Decision

Generate the species catalog from the running game's current data on the client.

- The Pal catalog is a nested page under the main Filters tab. On first use per panel/language instance, Blueprint enumerates the current `/Game/Pal/DataTable/Character/DT_PalMonsterParameter` table and caches normalized catalog entries for that panel instance.
- Catalog admission requires a real Paldex species row. Boss, tower-boss, raid-boss, non-Pal, invalid-index, and duplicate CharacterID rows are excluded while distinct suffix variants sharing a base Paldex index remain visible. Their loaded world actors remain eligible for the independent Boss filter.
- Localized names, element names, item names, partner-skill names and descriptions, icon soft references, base statistics, movement values, food amount, work ranks, and defeat drops come from current game APIs and data rows. Partner effects call the same runtime formatter used by the game's Paldex and Pal-box UI, `PalUIUtility::GetFormatedFirstActivatedInfoTextFixedRank(CharacterID, 1)`, so ordinary descriptions, referenced passive effects, rank parameters, localization, and explicit character overrides remain game-owned. The formatter's `id=|...|` and `style=|...|` attributes are normalized to UE 5.1's quoted attribute syntax immediately before rendering. The shared tooltip widget then uses the current game's `/Game/Pal/DataTable/Text/RchTextData/DT_PalRichTextStyle` and six `BP_PalRichText*Decorator` classes. The project-owned partial style table is retained solely as a load-failure fallback. A Shipping-safe full catalog audit covered 300 unique CharacterIDs and found only `activeSkillName`, `characterName`, `img`, `itemName`, `mapObjectName`, `Status_Keyword`, `Status_Up`, and `uiCommon`; its continuous logger is disabled after establishing this contract. This preserves current character-name, element-icon, localized UI-name, and status-value markup without committing copies of game UI assets. Character IDs are not guessed as `PalLongDescription` or `PartnerSkillAppendText` keys, and `OverridePartnerSkillDescTextID` is not treated as the complete description source. No Pal-name, icon, stat, work-rank, partner-skill, drop, rich-text style, or decorator snapshot is committed as runtime data.
- Icon widgets use soft texture references and asynchronous UMG loading. Catalog construction must not synchronously load every icon.
- Catalog rows default to current Paldex number ascending. The user can reload the current table and sort by Paldex number, sprint speed, or any of the 12 player-visible work suitabilities with explicit ascending/descending direction. Filtering, work-rank display, and work sorting share the same current-row field definitions: Kindling, Watering, Planting, Electricity, Handiwork, Gathering, Lumbering, Mining, Medicine, Cooling, Transporting, and Ranching. The internal `OilExtraction` field is intentionally not exposed. Search accepts the current localized name or displayed Paldex number. Distinct CharacterID variants are retained and exposed as an ID suffix in the card title so same-name rows with different drops remain distinguishable. Selected elements and work suitabilities use match-all semantics to narrow the catalog; no query selection directly widens world admission.
- Clicking catalog entries toggles a Blueprint-owned `TArray<FName>` of selected species. Selected cards use a high-contrast fill and border, and the page lists the current localized selected names independently of the active query. No selected species means all species; multiple selected species use match-any semantics for already admitted wild targets.
- Chinese is the default and is not mixed with English field labels. Catalog cards, result summaries, base/movement/food labels, work ranks, partner-skill labels and effects, and drop labels switch to English only when the panel language is English.
- Blueprint mirrors selected species as a delimiter-safe ASCII ID string. Lua validates and persists only that mirror, then uses a scalar/string revision to request actor-free Overlay synchronization. Lua never receives the species array, DataTable rows, icons, enums, drops, partner-skill data, or player data.
- Temporary compatibility audits may append the already-formatted species tooltip text and CharacterID to a ModActor `FString`; Lua drains newly appended complete lines into `UE4SS.log`. This diagnostic bridge is used because Blueprint `PrintString` is compiled to a no-op in Shipping builds. It contains no player identity or coordinates, is not persisted, and must be disabled after the current tag inventory is captured.
- The existing Lua human-player exclusion remains before all Blueprint enrichment and species filtering. The catalog cannot configure or bypass that boundary.

## Options Considered

### Option 1: Commit a generated catalog snapshot

Advantages:
- Fast and deterministic panel construction.
- Straightforward unit testing.

Disadvantages:
- Requires a data refresh for every Palworld update.
- Can show stale names, icons, work ranks, drops, and statistics.
- Conflicts with the requirement that future Pal additions appear without a Mod data release.

### Option 2: Parse save files or exported game data

Advantages:
- Does not require a Blueprint DataTable graph.

Disadvantages:
- Save files are not the authoritative source for the complete static catalog.
- Exported data is still a versioned snapshot.
- Adds format and maintenance risk outside the current runtime architecture.

### Option 3: Read the current game database through Blueprint

Advantages:
- Uses the running build's current rows, localization, and soft icon references.
- Preserves the existing pure client-side Lua plus LogicMod architecture.
- Keeps unsafe complex values out of Lua and behind the player gate.

Disadvantages:
- First catalog construction has a measurable one-time GameThread/UI cost.
- DataTable and localization contracts must be regression-tested after game updates.
- A large result set still creates many UMG entry widgets.

## Rationale

Option 3 is the only option that satisfies update compatibility without adding an exported-data maintenance workflow. It extends the already proven Blueprint field-provider boundary instead of introducing a runtime DLL or save parser. Caching normalized entries per panel instance and loading icons asynchronously bounds the most obvious repeated costs while keeping the first implementation reversible.

The partner-effect formatter is preferred over manually reconstructing `FPalPartnerSkillParameterDataRow::TextReferencePassiveSkills`. SDK dumps show that both the original Paldex captured view and Pal-box partner-skill detail call this formatter, while the raw parameter rows expose effect components rather than the final localized presentation contract. Rank 1 represents the baseline effect for a species-only catalog entry that has no individual Pal rank.

## Consequences

Positive effects:
- New Pal species and updated current-game values appear automatically when the stable table/API contracts remain available.
- Users can combine name, element, and multiple work-suitability requirements before choosing world filters.
- Selected species persist without exposing complex game data to Lua.

Negative effects:
- Opening the catalog for the first time may hitch on slower systems.
- Partner-skill descriptions and drop rows can be absent for legitimate species and must render as unavailable rather than fabricated.
- A renamed or removed game rich-text style/decorator path requires a compatibility update; the style table falls back to the project-owned partial table, while missing decorators degrade only their affected markup. Neither failure changes catalog filtering data.
- Table schema or asset-path changes require a compatibility update even though row content changes do not.

Follow-up considerations:
- Measure first-open catalog construction separately from normal ESP reconciliation.
- Replace eager UMG entry creation with pooled or virtualized entries only if measured cost justifies the added architecture.
- Keep catalog query semantics explicit: element/work filters are AND; selected species are OR.

## Follow-ups

- [x] Generate the catalog and entry widget from canonical editor automation.
- [x] Add strict selected-species persistence and legacy migration tests.
- [ ] Verify names, icons, work ranks, elements, details, and filtering on Steam single-player.
- [ ] Measure first-open time and repeated query latency.
- [ ] Accept this ADR only after runtime lifecycle and player-boundary regression passes.
