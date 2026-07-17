# ADR-0007: Sequence Feature Delivery Before Costly Performance Optimization

## Status

Proposed

## Date

2026-07-17

## Context

The first runtime experiments showed noticeable hitches while Pal actors were loading, but the available captures do not yet isolate discovery, admission, field probing, bridge synchronization, Blueprint painting, and the base game's streaming work. The number of visible guide lines is therefore not proven to be the dominant cost.

The current implementation already records stage timings and supports external PresentMon capture. The project also needs the user-facing Pal filters and additional entity adapters. Introducing a C++ hot path, memory readers, or broad cache changes before the feature baseline would make behavior harder to compare and increase lifecycle risk.

## Decision

Deliver the next functional baseline before pursuing broad performance rewrites. Performance work remains an explicit backlog and measurement requirement, but it does not block feature development unless a change introduces a crash, violates the human-player exclusion invariant, or produces a clearly reproducible severe hitch.

Keep the current Lua + LogicMod architecture and capture instrumentation while implementing features. Do not add a runtime DLL, memory reader, or C++ hot path solely because of subjective stutter reports. Revisit those options only after a reproducible benchmark identifies Lua, Blueprint, or bridge work as the bottleneck.

The optimization backlog is ordered as follows:

1. Coalesce event-queue admissions so one burst performs one filter/order/output refresh and one bridge synchronization.
2. Skip bridge synchronization when the ordered displayed actor set has not changed; consider a single Lua-to-Blueprint target-array call only after the existing scalar/event safety boundary is proven.
3. Cache static admission facts per world session and revalidate only dynamic facts such as death, ownership, and lifecycle state.
4. Make field probes lazy: read a field only when an active filter or visible style requires it, and retain safe scalar snapshots.
5. Cache the local player/controller lookup and run the hard player audit at lifecycle boundaries plus a bounded integrity interval.
6. Measure Blueprint/Slate `OnPaint` separately before changing line, label, box, or weak-point rendering.
7. Consider a C++ pure-data hot path only if the preceding changes and measurements show Lua/bridge overhead remains the limiting factor.

## Options Considered

### Option 1: Optimize the visible line count immediately

Advantages:
- Small implementation surface.
- Easy to expose as a user setting.

Disadvantages:
- Does not address full scans, per-actor admission, field probes, or repeated bridge calls.
- Can reduce functionality without proving that drawing is the bottleneck.

### Option 2: Freeze optimization and add features without measurement

Advantages:
- Fastest feature delivery.

Disadvantages:
- A new adapter or filter could silently increase long-frame cost.
- Existing performance evidence would become stale and difficult to compare.

### Option 3: Preserve measurement, deliver features, then optimize measured hotspots

Advantages:
- Keeps the user-visible roadmap moving.
- Makes competing implementations comparable with the same effect and target scene.
- Avoids premature C++ or memory-reading commitments.

Disadvantages:
- The current hitch risk remains until the feature baseline is complete.
- Some instrumentation and benchmark maintenance must continue during feature work.

## Rationale

Option 3 matches the current evidence and keeps future choices reversible. Unreal's profiling guidance separates CPU, GPU, GameThread, UI, and processing spikes; UE4SS also distinguishes one-shot object discovery from construction notifications. The repository already has enough instrumentation to preserve that evidence while implementing filters and adapters.

## Consequences

Positive effects:
- Feature delivery is not blocked by an unproven line-count hypothesis.
- The player boundary, lifecycle checks, and benchmark contract remain in force.
- Future optimizations can be compared under the same behavior and display result.

Negative effects:
- Some users may continue to observe hitches during the feature phase.
- The current full-reconcile and per-target bridge paths remain technical debt.

## Follow-ups

- [ ] Implement the first Pal filter/data adapter baseline.
- [ ] Keep stage timing and PresentMon capture available for each new adapter.
- [ ] Add event-burst coalescing and bridge-set deduplication after the feature baseline.
- [ ] Run fixed-view and movement benchmarks with identical target behavior before accepting an optimization.
- [ ] Revisit a C++ hot path only after a measured Lua/Blueprint bottleneck is reproducible.
