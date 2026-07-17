# ADR-0006: Use a Panel Control Plane and External Performance Capture

## Status

Proposed

## Date

2026-07-17

## Context

The Entity Core smoke tests proved multi-target guides, capture/death cleanup, map teardown, and the hard player boundary. They also measured reconciliation callbacks above the frame budget and showed that subjective remote-play reports are not sufficient to compare algorithms.

The maintainer needs to switch Mod behavior and mark benchmark intervals without restarting the game or repeating a streaming route. The runtime remains a pure client-side UE4SS Lua plus LogicMod package, while PresentMon remains an external ETW tool. Earlier startup tests also showed that a Blueprint-initiated UE4SS custom event can crash before the first frame.

## Decision

Add a dedicated `WBP_ESPPanel` controlled by `Shift+Y` and keep it separate from the per-frame overlay.

- Lua owns the key bind and invokes Blueprint panel methods.
- The key callback waits 50 ms before invoking Blueprint so Widget removal does not occur inside UE4SS key dispatch.
- While open, the panel owns input through `UIOnlyEx`; closing restores `GameOnly`. Both transitions flush pending input.
- The panel writes only scalar control properties and a monotonically increasing revision on the passive `ModActor`.
- User-facing level, distance, and display-limit controls use integer SpinBox inputs. Level and distance use zero for an inactive bound; the display limit is clamped to 1-512.
- Recreated panel widgets initialize their numeric controls from the passive `ModActor`; programmatic initialization still crosses only the Blueprint-to-Blueprint boundary.
- Lua polls those properties every 250 ms and applies changes on the GameThread.
- No panel option can bypass the registry-owned human-player rejection gate.
- Runtime profiles are `off`, `snapshot_once`, `chunked_current`, and `event_first`.
- During the experiment, `chunked_current` reconciles every 5 seconds in batches of two, while `event_first` performs a 30-second integrity reconciliation and admits notifications one at a time.
- Profile transitions invalidate prior reconcile jobs and pending notification work before applying the new profile.
- Capture buttons emit timestamped `PERF_SESSION_START`, `PERF_MODE_CHANGED`, and `PERF_SESSION_STOP` log markers.
- A repository PowerShell companion controls PresentMon and writes benchmark data outside the repository. The packaged Mod never launches a process or adds a runtime DLL.
- Fixed-view segmented captures measure steady-state behavior. A separate movement capture remains required for entity-streaming behavior.

The advanced diagnostics controls are present in the public package but collapsed by default. The resulting winning discovery strategy is selected by functional correctness, then the lowest rate of frames over 50 ms, then P99 frame time.

## Options Considered

### Option 1: Continue restart-based A/B captures

Advantages:
- No panel work is required.

Disadvantages:
- World streaming, cache state, route timing, and remote input make comparisons noisy.
- The maintainer must repeatedly leave and restart the game.

### Option 2: Let Blueprint launch or embed PresentMon

Advantages:
- One visible control surface.

Disadvantages:
- Violates the asset-only runtime boundary.
- Adds process-management and distribution risk to the Mod.

### Option 3: Panel markers with an external capture companion

Advantages:
- Keeps runtime pure-client and asset-only.
- Produces exact segment boundaries while the player remains in one world session.
- Reuses the proven UE4SS log and PresentMon measurement paths.

Disadvantages:
- Capture buttons require the companion tool to be running for frame data to exist.
- Panel-to-Lua scalar polling adds a small fixed control-plane cost that must be measured.

## Rationale

Option 3 isolates benchmark modes without expanding runtime authority. Scalar polling avoids the failed Blueprint-to-Lua startup event direction, and the external companion preserves PresentMon as a development tool instead of a player dependency.

## Consequences

Positive effects:
- The maintainer can run a complete segmented comparison from inside the game.
- Runtime-off, render-only, current chunking, and event-first behavior can be compared in one process.
- Benchmark logs remain free of player identity and coordinates.

Negative effects:
- The panel and polling loop remain loaded when ESP discovery is off.
- A separate fully-disabled launch is still needed to quantify that fixed control-plane overhead.
- Event-first admission must retain the existing map-teardown and wrapper-lifetime safeguards.

## Follow-ups

- [x] Generate, cook, and package `WBP_ESPPanel` through ADR-0004 automation.
- [x] Implement the PresentMon watcher, marker parser, segmented analyzer, and synthetic contract tests.
- [x] Revalidate deferred open/close and gameplay input restoration in Steam single-player.
- [ ] Validate Shift+Y button clicks and UI/game input restoration after the `UIOnlyEx` fix.
- [ ] Run forward and reverse fixed-view profile sequences with PresentMon.
- [ ] Run one movement capture after a steady-state strategy passes.
- [ ] Accept this ADR only after panel lifecycle, capture segmentation, and normal exit pass.
