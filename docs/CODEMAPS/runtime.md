<!-- Generated: 2026-07-20 | Files scanned: 72 | Token estimate: ~850 -->

# Runtime Codemap

## Entrypoint

`PalworldResourceESP/Scripts/main.lua` (4086 lines) composes the runtime in this order:

```text
initialize_user_settings
-> resolve_required_classes
-> register_entity_adapters
-> register Blueprint bridge + discovery
-> register monster notification + lifecycle/map hooks
-> register draw hook + Shift+Y keybind
-> start 250 ms runtime tick
-> schedule world bootstrap
```

## Pure Modules

| File | Public role |
| --- | --- |
| `core/adapter_registry.lua` | Ordered adapters; mandatory player gate before dispatch |
| `core/entity_snapshot.lua` | Session/generation records and `known/bridge/unavailable` cells |
| `core/filter_engine.lua` | Fail-closed matching, deterministic ordering and display budget |
| `core/runtime_profiles.lua` | Off/snapshot/current/event modes; all active steady-state intervals are zero |
| `core/user_settings.lua` | Strict `v1..v12` parsing, normalization, append and path resolution |
| `adapters/pal.lua` | Wild-Pal classification and safe field normalization |
| `config.lua` | Runtime limits, defaults, display flags and timing constants |

## Lifecycle Flow

```text
Playable-world bootstrap -> one synchronous full snapshot
BeginPlay(Pal) -> player/owner/dead gate -> direct admission
  not ready -> queue object path only -> StaticFindObject retry (250 ms, max 8)
  accepted -> add one target; evict oldest if display window is full
Death/Capture/EndPlay -> cancel retry -> remove one aligned target index
Map pre-hook/runtime off -> clear bridge, queues, candidates and lifecycle generation
```

No delayed job retains the BeginPlay UObject wrapper. `NotifyOnNewObject` is diagnostic only. Resolved active profiles set `reconcile_interval_ms = 0`; full rescans are explicit repair points for world entry, profile changes, and query changes that require Lua fields.

## Runtime Controls

- `Shift+Y`: enqueue panel toggle; the idle runtime tick performs it.
- Display limit: UI `1..100`; default `64`.
- Distance: fixed lower bound `0m`, maximum `0..330m`.
- Readiness queue: max `512`, one path processed per callback.
- Metrics: emitted every 30 seconds; privacy audit must keep `candidate_player_count=0`.

## Bridge Calls

`ResetSession`, `SetTarget`, `RemoveTarget`, `ClearTarget`, `TogglePanel`, and `SetDisplayStyle` are Lua-to-Blueprint only. Blueprint never initiates runtime discovery.
