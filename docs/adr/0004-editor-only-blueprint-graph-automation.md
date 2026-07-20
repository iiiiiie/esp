# ADR-0004: Use editor-only C++ automation for LogicMod blueprint graphs

## Status

Accepted

## Date

2026-07-15

## Context

The PMK assets must be generated without requiring the maintainer to operate Unreal Editor manually. UE 5.1 exposes blueprint creation through Python, but its Python wrapper does not expose `EdGraph.Nodes` or node creation/linking APIs. The LogicMod itself must remain a pure client-side asset package and must not depend on a new server component or a runtime native DLL.

## Decision

Add an editor-only `ESPBlueprintAutomation` plugin that constructs and links the project-owned Blueprint graphs and Widget trees through Unreal's C++ editor APIs. The canonical plugin source and asset-generation scripts live under `tools/logicmod` in this repository. A build step copies that fixed source set into the external experimental PMK before generation and cooking.

The external PMK is a build workspace, not a source of record. Its `Binaries`, `Intermediate`, `Saved`, cooked assets, and unrelated local compatibility changes are never copied into this repository. The plugin is enabled only while generating/cooking assets and is not included in the packaged LogicMod runtime.

Cross-asset Widget dependencies must remain valid throughout repeated generation. `WBP_ESPPassiveTooltip` establishes its rich-text initialization event before `WBP_ESPPassiveEntry` is generated, and the entry establishes its public initialization event before `WBP_ESPPanel` is generated. A WidgetTree-only intermediate compile is allowed only on first creation, when the generated control properties do not yet exist; later runs preserve the previous public event until the final replacement graph compiles.

The rich-text repair command is a tooltip-only scoped generator. It may compile and save `WBP_ESPPassiveTooltip`, but it must not prepare, compile, mark dirty, or save `ModActor`, `WBP_ESPPalEntry`, or `DT_ESPRichTextStyle`. Those startup-path assets change only through the complete generator, where their dependency order is rebuilt as one transaction. A 2026-07-19 scoped build that also recompiled those assets produced a reproducible Login-map startup access violation despite clean Blueprint compilation and Cook output.

Passive-skill descriptions use a dedicated `WBP_ESPPassiveTooltip`. Pal's runtime formatter emits rich-text attributes with pipe delimiters such as `id=|...|`, while UE 5.1's stock rich-text parser accepts quoted attributes. The tooltip therefore normalizes pipe delimiters to quotes immediately before assigning the text, then loads the current game's `/Game/Pal/DataTable/Text/RchTextData/DT_PalRichTextStyle` and six `BP_PalRichText*Decorator` classes. The project-owned `DT_ESPRichTextStyle` remains a load-failure fallback because those game assets are absent from the pinned PMK build workspace. This preserves current-game IDs and style rows without committing a versioned game-data copy. An unparented tooltip subtree inside `WBP_ESPPassiveEntry` is not supported because UE 5.1 removes it from the generated Widget properties.

The generated catalog prefers the current game's localized `SkillDesc` row and substitutes all `{EffectValue1..4}` placeholders. Some standard Pal passives intentionally have neither `OverrideDescMsgID` nor `OverrideSummaryTextId`, while a small set of current localized descriptions contains tags that the pinned PMK cannot faithfully reproduce. For confirmed stable internal IDs, the canonical generator therefore embeds a narrowly-scoped Chinese/English fallback description. A fallback overrides the live description only for its exact internal ID; every other passive continues to use current game localization. The fallback list is source-controlled and must be revalidated when the pinned Palworld data fingerprint changes.

Catalog rarity groups use the game row's `Rank` field rather than localized names or `LotteryWeight`. Rank 5 and higher is Rainbow, Rank 4 is Legend, Rank 3 is Gold III, Rank 2 is Gold II, Rank 1 is Normal, and Ranks -1/-2/-3 are the matching negative tiers. Rank 0 and unknown future ranks are not mounted into a visible group. `LotteryWeight` controls acquisition probability and must not be used as a rarity label: doing so previously moved Rank-5 World Tree passives into Legend and split Rank-4 passives between Rainbow and Legend.

## Options Considered

### Option 1: Manual Unreal Editor authoring

Advantages:
- Uses the standard editor UI.

Disadvantages:
- Requires user interaction and is difficult to reproduce or verify.

### Option 2: Python-only graph editing

Advantages:
- No additional C++ source.

Disadvantages:
- UE 5.1 Python does not expose the protected graph node collection or node creation APIs needed for deterministic generation.

### Option 3: Editor-only C++ automation

Advantages:
- Uses the supported native graph APIs and can be run unattended.
- Does not add runtime dependencies to the LogicMod.

Disadvantages:
- Adds a small PMK-only build dependency and requires rebuilding the editor target.

## Rationale

Option 3 meets the user's requirement that the assistant perform the editor work while preserving a pure asset-based runtime mod. The added module is isolated from the packaged chunk and can be removed or disabled after cooking without changing the generated assets.

## Consequences

Positive:
- Blueprint graph generation is reproducible and scriptable.
- Community contributors can inspect and rebuild the generator without receiving the maintainer's PMK workspace.
- No manual editor operation is required.
- Runtime remains client-only and asset-based.
- Standard passives without a description ID still expose readable bilingual effects.

Negative:
- The experimental PMK editor target must be rebuilt once after adding the plugin.
- Generated assets should be regenerated when the graph contract changes.
- Renaming or removing the runtime game style table or decorator paths degrades the tooltip to the project-owned fallback and requires a compatibility update.
- The exact-ID fallback list and Rank grouping rules require review after game data updates.
- The sync step must verify the external PMK destination before replacing the build-workspace copy.

## Follow-ups

- [x] Validate the generated bridge events in a Steam single-player smoke test.
- [x] Keep the editor-only plugin out of the final LogicMod package; the current rich-text/catalog pak contains exactly eighteen asset files and no DLL.
- [x] Regenerate and cook `WBP_ESPPanel`, `WBP_ESPPassiveEntry`, `WBP_ESPPassiveTooltip`, `WBP_ESPPalEntry`, and `DT_ESPRichTextStyle`; retain nine canonical source assets in `LogicMod/Content`.
