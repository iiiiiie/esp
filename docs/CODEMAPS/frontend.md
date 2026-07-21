<!-- Generated: 2026-07-20 | Files scanned: 72 | Token estimate: ~700 -->

# Frontend Codemap

## Asset Tree

| Asset | Role |
| --- | --- |
| `ModActor.uasset` | Passive control plane and typed Lua-callable events |
| `BP_ESPBridge.uasset` | Bridge helper contract |
| `WBP_ESPOverlay.uasset` | Per-frame projection, filtering and top-guide/label drawing |
| `WBP_ESPPanel.uasset` | Full-viewport settings UI and input ownership |
| `WBP_ESPPassiveEntry.uasset` | Passive include/exclude row and mouse behavior |
| `WBP_ESPPassiveTooltip.uasset` | Rich-text effect renderer |
| `WBP_ESPPalEntry.uasset` | Runtime Pal catalog row and hover details |
| `DT_ESPRichTextStyle.uasset` | Project fallback rich-text styles |
| `PalworldResourceESP.uasset` | Primary Asset Label for the LogicMod chunk |

## UI Hierarchy

```text
WBP_ESPPanel
  Display tab -> top guide, name, level, distance, IV, passives
  Filters tab
    General -> reset, level/distance, IV, elements, gender, Lucky, Boss,
               collection completion, passive categories/search
    Pal species -> search, element/work queries, sort, selected summary

WBP_ESPOverlay
  aligned target + parameter + metadata arrays
  -> live parameter/CharacterID identity gate
  -> mismatch: fail closed for this frame; never mutate during iteration
  -> pre-draw filters -> projection -> guides/labels
```

## State Flow

- Panel writes scalar properties plus revisions on passive `ModActor`.
- Lua polls revisions every 250 ms, applies runtime/query changes and persists stable state.
- Passive and species selections remain Blueprint arrays; delimiter-safe ID mirrors cross into Lua only for persistence/filter contracts.
- Slider drag is preview-only; release/commit changes one revision.
- Chinese is default; one language state switches all supported labels to English.

## Input Boundary

Opening uses `UIOnlyEx`, focuses the panel, shows the cursor, flushes input and sets the Mod-owned Pal controller disable flag. Key events return `Handled` after focused children. An unconsumed Escape key-down records a widget-local pending flag; the matching key-up clears it and invokes the canonical panel toggle, so game input is not restored during the original key-down. Closing clears only the Mod-owned controller flag and restores `GameOnly`, while Escape remains native game input when the panel is closed.

## Generation

Canonical generator: `tools/logicmod/ESPBlueprintAutomation`. Full builds call `BuildPalworldResourceESPAssets`; scoped tooltip repair calls `BuildPalworldResourceESPRichTextAssets`. The plugin is editor-only and excluded from the Pak.
