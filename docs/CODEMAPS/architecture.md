<!-- Generated: 2026-07-20 | Files scanned: 72 | Token estimate: ~620 -->

# Architecture Codemap

## System Shape

Single-repository, pure-client Palworld Mod. Runtime is UE4SS Lua plus a cooked LogicMod; no server component or runtime DLL is shipped.

```text
Palworld lifecycle/objects
  -> UE4SS hooks and object notifications
  -> Lua player gate -> Pal adapter -> normalized Entity Core
  -> Lua filters/budget -> typed ModActor events
  -> Blueprint Overlay/Panel -> UMG rendering and game-data providers

Panel scalar state -> Lua poll (250 ms) -> filters/profile/settings
Settings -> append-only Scripts/user-settings.log (v12)
```

## Ownership Boundaries

| Owner | Responsibility | Must not do |
| --- | --- | --- |
| Lua runtime | Lifecycle, human-player rejection, wild-Pal admission, snapshots, scalar filters, persistence | Render or retain delayed UObject wrappers |
| Blueprint/UMG | Panel, projection, labels, game-localized catalogs, typed Pal-only providers | Enumerate actors or admit players |
| Editor plugin | Generate project-owned Blueprint graphs/assets in external PMK | Ship in runtime Pak |
| External tools | Cook/Pak, PresentMon capture, release assembly | Enter runtime package |

## Invariants

- Human-player rejection runs before every adapter, filter, budget, and bridge call.
- Stable runtime discovery is event-driven; active profiles use `reconcile_interval_ms = 0`.
- Full snapshots occur only on world/profile/query transitions.
- Runtime catalog data comes from the current game, not a committed Pal snapshot.
- Cooked Pak contains 9 `.uasset` + 9 `.uexp` files and no DLL.

## Key Paths

- `PalworldResourceESP/Scripts/main.lua`: runtime composition root.
- `PalworldResourceESP/Scripts/core/`: pure snapshot/filter/profile/settings modules.
- `PalworldResourceESP/Scripts/adapters/pal.lua`: Pal adapter.
- `LogicMod/Content/Mods/PalworldResourceESP/`: 9 canonical source assets.
- `tools/logicmod/`: editor-only generator and PMK sync tooling.
- `tests/`: Lua, runtime-stub, source-contract, and performance-marker tests.
- `docs/adr/`: binding architecture decisions; ADR-0010 replaces periodic steady-state scanning.
