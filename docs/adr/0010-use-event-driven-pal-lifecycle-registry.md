# ADR-0010: Use Event-Driven Pal Lifecycle Updates

## Status

Proposed

## Date

2026-07-20

## Context

The production `chunked_current` profile still performed one synchronous full reconciliation every five seconds. A fixed scene with 21 loaded Pal actors measured `118-131 ms` per reconciliation: approximately `30 ms` in `FindAllOf` discovery, `88-96 ms` in admission and field access, and only `1-2 ms` in Blueprint bridge synchronization. This produced a reproducible five-second GameThread hitch and visible target-array refresh.

Delayed Lua processing is not a safe smoothing mechanism. Two earlier movement runs crashed after retaining UE4SS UObject wrappers across delayed callbacks. ADR-0005 therefore keeps the hard player gate in Lua, and ADR-0006 forbids delayed admission jobs that retain wrappers.

UE4SS exposes one global BeginPlay post hook plus UFunction hooks for lifecycle transitions. Palworld exposes `PalCharacter::OnDeadCharacter` and `PalUtility::PalCaptureSuccess`. These allow already-initialized actors to be handled individually without repeating `FindAllOf` in steady state.

The first Steam validation exposed an initialization-order constraint. The playable-world bootstrap ran before any monsters were discoverable (`raw=0`), then 79 BeginPlay admissions failed with `individual_parameter_unavailable`. Toggling the Mod off and on later found 46 actors because their individual parameters were ready by then. BeginPlay therefore identifies the actor lifecycle event, but does not guarantee that Pal-specific admission fields are ready in the same callback.

## Decision

Replace steady-state periodic Pal reconciliation with event-driven lifecycle updates.

- Run one full, synchronous snapshot when entering a playable world, changing a query that requires Lua fields, or explicitly changing runtime profile.
- Set the default active profile and `event_first` profile to a zero millisecond reconciliation interval. Target-display presets do not control scan frequency.
- Reuse the single `RegisterBeginPlayPostHook`. After an actor has completed BeginPlay, process only `PalMonsterCharacter` instances.
- Run the registry-owned human-player rejection gate, ownership checks, death checks, field normalization, and active Lua filters before any event target reaches Blueprint.
- Attempt admission directly from BeginPlay. When it fails for a bounded set of initialization-only reasons, store only the actor's `GetFullName()`-derived object path, lifecycle generation, and attempt count. Resolve that exact path with UE4SS `StaticFindObject` after 250 milliseconds, process one entry per callback, and stop after eight attempts.
- Never retain the BeginPlay callback's UObject wrapper in an event queue. Permanent player, ownership, death, filter, and display-budget rejections are not retried. Death, capture, EndPlay, profile changes, and world teardown cancel matching pending paths.
- Add one target through the existing typed Blueprint bridge. Remove one target on `ReceiveEndPlay`, `OnDeadCharacter`, or `PalCaptureSuccess` through a new typed `RemoveTarget` bridge event.
- Treat the configured target limit as a rolling display window. When a newly initialized matching Pal arrives at capacity, remove the oldest registered target before adding the new target; this preserves the exact limit while allowing movement into newly streamed areas to refresh the window.
- The Overlay removes the same index from every target and metadata array. Full clear remains available for world teardown, runtime-off, profile changes, and query rebuilds.
- `NotifyOnNewObject` remains registered exactly once for diagnostics, but construction notifications no longer request a full snapshot or carry an Actor into delayed work.
- Preserve the external PresentMon capture contract and all human-player exclusion diagnostics.

If accepted, this decision supersedes only the periodic steady-state reconciliation portions of ADR-0005 and ADR-0006. Their player boundary, typed field-provider, lifecycle-generation, panel, and capture decisions remain in force.

## Options Considered

### Option 1: Increase the Reconciliation Interval

Advantages:
- Minimal code change.

Disadvantages:
- Hides rather than removes the long frame.
- Makes target discovery and cleanup slower.
- Does not reduce the cost of any individual scan.

### Option 2: Delay Batches of FindAllOf Wrappers

Advantages:
- Can spread admission work across frames.

Disadvantages:
- Previously caused native GameThread access violations during streaming.
- Still performs global object discovery.

### Option 3: BeginPlay and Lifecycle Delta Updates

Advantages:
- Removes global scans from steady state.
- Distributes work across actual Pal lifecycle events.
- Retains the existing Lua player gate and asset-only runtime.

Disadvantages:
- Requires an aligned single-target removal path in Blueprint.
- The rolling window favors recently initialized targets rather than recomputing a global nearest-target order.
- Requires Steam runtime validation for every lifecycle hook.

### Option 4: Native C++ Spawn Registry

Advantages:
- Provides weak UObject references and the highest performance headroom.
- Can use `UWorld::AddOnActorSpawnedHandler` directly.

Disadvantages:
- Adds a runtime DLL and ABI maintenance.
- Expands the current deployment and security boundary.

## Rationale

Option 3 removes the measured periodic bottleneck without introducing the runtime DLL cost of Option 4. It also avoids the already-reproduced wrapper-lifetime failure of Option 2. Full snapshots remain explicit, user-driven repair points rather than a timer-based steady-state mechanism.

## Consequences

Positive effects:
- A fixed scene no longer receives a 100+ ms Mod callback every five seconds.
- New loaded Pals require one admission instead of re-admitting every loaded Pal.
- Pals whose individual parameters are not ready at BeginPlay can become visible without a global repair scan.
- Death, capture, and streamed EndPlay remove only the affected target.
- Line arrays no longer clear and rebuild on a timer.

Negative effects:
- BeginPlay admission and three removal hooks become correctness-critical.
- A newly loaded Pal may appear up to the bounded readiness-retry window after BeginPlay.
- Initial world entry and query changes still perform one synchronous full snapshot.
- A future continuously distance-sorted nearest-target policy would require a separate eligible-target queue and ordering design.
- Until then, the rolling window is recency-based rather than nearest-target based.

Needs follow-up:
- Verify no periodic `SCAN_DONE` appears during a 60-second fixed scene.
- Verify `EVENT_TARGET_ADDED` remains below `8 ms` while moving through streaming cells.
- Verify death, capture, EndPlay, return-to-title, and normal exit.
- Verify `candidate_player_count=0` and no player target reaches either event bridge call.
- Compare fixed-view and movement captures with the previous stable Pak.
- Escalate to a separate C++ runtime ADR only if event callbacks still exceed the frame budget or miss lifecycle transitions.

## Follow-ups

- [x] Implement zero-interval event-driven active profiles.
- [x] Add BeginPlay single-target admission without delayed UObject retention.
- [x] Add death, capture, and EndPlay single-target removal contracts.
- [x] Add unit and integration tests for no periodic scan and lifecycle deltas.
- [x] Add path-only readiness retries for post-BeginPlay Pal initialization.
- [x] Preserve the configured target count with oldest-target rolling replacement.
- [x] Compile, cook, and inspect the new Overlay and ModActor assets.
- [ ] Validate Steam single-player lifecycle behavior.
- [ ] Run fixed-view and movement PresentMon captures.
- [ ] Accept this ADR after the performance and lifecycle gates pass.
