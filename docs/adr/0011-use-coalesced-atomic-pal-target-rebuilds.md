# ADR-0011: Use Coalesced Atomic Pal Target Rebuilds

## Status

Superseded by ADR-0012

## Date

2026-07-21

## Context

Steam validation rejected the incremental lifecycle design in ADR-0010. Pal parameter initialization commonly emitted two usable post-hooks for the same Actor. Each hook synchronously repeated Lua admission and Blueprint metadata work on the GameThread: observed callbacks cost approximately `68-91 ms` and `103-119 ms` per Pal, with a measured maximum of `141 ms`. Streaming several Pals therefore produced frequent medium-to-severe hitches while moving.

The same validation exposed a correctness failure in the Blueprint bridge. Changing the display limit invoked the full bridge synchronization path and restored all 16 correct guides. Killing one Pal then changed Lua's eligible count only from 16 to 15, but most visible guides disappeared. The lifecycle path had called single-target `RemoveTarget` and incremental `SetTarget` against several parallel Blueprint arrays. The Overlay also removed and re-added an entry from inside `ForEachTarget` when live identity validation failed. Either mutation can invalidate iteration or array alignment, so Lua's target count cannot prove that the visible Blueprint state is coherent.

UE4SS Lua provides BeginPlay, construction, and UFunction hooks, but none is a complete Palworld object-pool reactivation registry. A native `UWorld::AddOnActorSpawnedHandler` weak-reference registry would require a runtime DLL and would expand the deployment, ABI, and security boundary established by ADR-0001.

## Decision

Keep the UE4SS Lua plus LogicMod architecture and replace incremental lifecycle synchronization with coalesced atomic rebuilds.

- Pal BeginPlay, parameter initialization, active-state, death, capture, and tracked EndPlay hooks perform no Pal admission, metadata refresh, path resolution, or Blueprint target mutation. They only request a rebuild by updating Lua scalar state.
- Coalesce hook bursts behind a 500 millisecond quiet window, cap continuous coalescing at 1500 milliseconds, and enforce a 2500 millisecond minimum interval between completed lifecycle rebuilds.
- Preserve the movement completeness trigger, but after 120 metres it requests the same rebuild instead of running an identity census and queueing individual Actor paths.
- Cached current-nearest membership changes also request the same rebuild. They do not call `RemoveTarget` or `SetTarget` incrementally.
- A rebuild performs one wrapper-safe synchronous `FindAllOf`, admission, filtering, current-distance ordering, and display budgeting pass. After the snapshot is complete, synchronize Blueprint with exactly one `ClearTarget` followed by ordered `SetTarget` calls for the current nearest `N` records.
- Death, capture, EndPlay, pooled activation, and parameter rebinding never call the single-target Blueprint `RemoveTarget` path.
- The Overlay continues to validate the live `IndividualParameter` object and `CharacterID` before drawing. An identity mismatch fails closed for that frame; it never mutates any target or metadata array from inside `ForEachTarget`. Lifecycle or movement invalidation performs the later atomic repair.
- Never retain an event callback's UObject wrapper in a delayed callback. Rebuild scheduling stores only scalar flags, timestamps, reason text, and revision numbers.

This decision supersedes ADR-0010. ADR-0001's no-runtime-DLL boundary and ADR-0005/ADR-0006's player gate, generation scoping, and wrapper-lifetime constraints remain in force.

## Options Considered

### Option 1: Keep Incremental Lifecycle Deltas and Repair RemoveTarget

Advantages:
- Lowest apparent amount of work per ideal lifecycle event.
- Can update one target immediately.

Disadvantages:
- Real Palworld hooks arrive in duplicate bursts and already measured `68-119 ms` each.
- Parallel Blueprint arrays have already lost visible consistency after one death.
- Still depends on incomplete object-pool lifecycle signals.

### Option 2: Coalesced Atomic Rebuilds

Advantages:
- Uses the only path that restored the complete, correctly identified target set during validation.
- Makes all parallel Blueprint arrays originate from one ordered snapshot.
- Converts many per-Pal stalls into at most one coalesced rebuild.
- Retains the current Workshop/Nexus package format and security boundary.

Disadvantages:
- A rebuild remains a synchronous GameThread operation; the observed 21-object scan cost was about `146 ms`.
- New or removed targets may remain hidden or stale until the debounce expires.
- Continuous lifecycle activity can postpone a rebuild only to the 1500 millisecond hard deadline.

### Option 3: Native UWorld Weak-Reference Registry

Advantages:
- Offers complete spawn tracking and native weak references.
- Provides the largest performance headroom.

Disadvantages:
- Requires a runtime DLL, Palworld ABI maintenance, new packaging, and security review.
- Conflicts with the current asset-only runtime boundary unless ADR-0001 is superseded.

## Rationale

Option 2 is the smallest reversible design that matches the runtime evidence. Full synchronization is the only path demonstrated to restore correct visible state, while event callbacks are reliable enough as invalidation hints but not as a complete identity registry. Coalescing removes duplicate per-Pal work without introducing a DLL. The remaining occasional full-scan hitch is accepted provisionally because it replaces much more frequent stalls and preserves array correctness; Steam validation must determine whether its debounce and minimum interval need tuning.

## Consequences

Positive effects:
- One lifecycle burst causes one discovery/admission pass and one complete Blueprint rebuild.
- One death cannot shift or partially remove parallel metadata arrays.
- Overlay identity validation cannot corrupt its own active iteration.
- Movement discovery and parameter hooks no longer queue per-Actor path callbacks.
- The display limit is evaluated from one current-distance-ordered snapshot.

Negative effects:
- Target additions and removals are not immediate; the quiet window is 500 milliseconds and the minimum rebuild spacing can delay a later generation up to 2500 milliseconds.
- A full rebuild can still produce one noticeable frame hitch.
- Current-nearest membership is exact at rebuild boundaries rather than continuously re-resolving every Actor each tick.

Needs follow-up:
- Validate a 60/61 target limit without toggling the limit after save entry.
- Kill and capture targets and confirm all surviving guides remain visible and correctly identified.
- Travel beyond 200 metres, stop, and confirm newly loaded Pals appear in one complete rebuild.
- Measure stationary and movement frame times and inspect `ATOMIC_REBUILD_COMPLETED` frequency.
- Consider a separate native runtime ADR only if the coalesced full rebuild remains unacceptable.

## Follow-ups

- [x] Add lifecycle-burst, death, nearest-N, and movement atomic-rebuild regressions.
- [x] Remove synchronous admission and refresh work from lifecycle hook callbacks.
- [x] Stop lifecycle and nearest-N paths from calling single-target Blueprint removal.
- [x] Make Overlay identity mismatch fail closed without mutating target arrays.
- [x] Regenerate, compile, cook, and inspect the Overlay asset.
- [ ] Validate the candidate in Steam Palworld without changing the display limit.
- [ ] Accept or revise this ADR from runtime evidence.
