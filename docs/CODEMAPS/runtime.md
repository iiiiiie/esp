<!-- Generated: 2026-07-21 | Files scanned: 72 | Token estimate: ~900 -->

# Runtime Codemap

## Entrypoint

`PalworldResourceESP/Scripts/main.lua` composes the runtime in this order:

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
BeginPlay/ActiveActor/LocalInitialized/parameter hooks -> enqueue composite key + object path
  delayed callbacks resolve one path and admit/refresh one Actor
Death/Capture/tracked EndPlay/inactive Actor -> remove one scalar candidate key
  no hook retains a UObject wrapper or calls Blueprint RemoveTarget
Runtime tick (250 ms) -> rank copied coordinate scalars for current-camera nearest N
  refresh at most two candidate paths when position convergence is needed
  membership/metadata change -> resolve only desired N paths
  after all resolve -> one ClearTarget + ordered SetTarget calls
  transient resolution failure -> preserve old arrays and retry
Accumulated camera travel >= 120m, minimum interval 2s -> identity/coordinate census
  one FindAllOf reads composite keys, paths and coordinates only
  known Actor -> refresh scalar record; unknown Actor -> enqueue path admission
  stationary camera + no lifecycle signal -> no FindAllOf, regardless of elapsed time
Overlay OnPaint -> compare cached parameter + CharacterID with current Actor binding
  unavailable -> hide this target for the frame
  mismatch -> hide this target; never mutate parallel arrays during ForEachTarget
Map pre-hook/runtime off -> clear bridge, queues, candidates and lifecycle generation
```

`NotifyOnNewObject` is diagnostic only. Resolved active profiles set `reconcile_interval_ms = 0`; stationary steady state does not call `FindAllOf`. World entry, profile changes, and explicit query changes retain the complete snapshot path. Lifecycle changes use one-Actor path admission, nearest membership uses scalar ordering, and every visible-set change retains the atomic Blueprint clear/set contract.

## Runtime Controls

- `Shift+Y`: enqueue panel toggle; the idle runtime tick performs it.
- Display limit: UI `1..100`; default `64`.
- Lifecycle admission: path-only queue, one Actor per callback; presentation coalesces on the `250 ms` runtime tick.
- Nearest position convergence: at most two candidate path resolutions per tick before an atomic nearest-N submission.
- Distance: fixed lower bound `0m`, maximum `0..330m`.
- Movement completeness: accumulated `120m`, minimum trigger interval `2s`.
- Metrics: emitted every 30 seconds; privacy audit must keep `candidate_player_count=0`.

## Bridge Calls

`ResetSession`, `SetTarget`, `ClearTarget`, `TogglePanel`, and `SetDisplayStyle` are Lua-to-Blueprint only. `RemoveTarget` remains a compatibility event but is not called by lifecycle synchronization. Blueprint never initiates runtime discovery.
