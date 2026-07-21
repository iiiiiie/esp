# ADR-0012: Use Event Deltas with Atomic Cached Presentation

## Status

Proposed

## Date

2026-07-21

## Context

Steam validation accepted the identity correctness of ADR-0011 but rejected its frame pacing. With a display limit of three, the runtime repeatedly detected a nearest-membership change and promoted it to a complete `FindAllOf`, admission, filtering, ordering, and bridge rebuild. These rebuilds occurred approximately every 2.5 to 3 seconds while moving and cost `292-365 ms` each.

The sort was not the expensive operation. By scan 55, discovery had consumed `2093 ms`, admission had consumed `10049 ms`, ordering had consumed `3 ms`, and Blueprint synchronization had consumed `209 ms`. Re-admitting 30 to 50 loaded Actors to display three targets was therefore the dominant cost.

ADR-0010 had already established a scalar candidate pool containing composite instance keys, object paths, copied coordinates, and display-safe values. Its runtime failure came from synchronous duplicate admissions and single-target `RemoveTarget`/`SetTarget` mutation of parallel Blueprint arrays, not from the scalar registry itself. ADR-0011 fixed array correctness by using complete clear/set synchronization, but coupled that presentation repair to a complete world snapshot.

UE4SS documents `NotifyOnNewObject` as a construction notification, `FindAllOf` as enumeration of every non-default instance, and `StaticFindObject` as an in-memory object lookup. It also warns against retaining transient Actor objects. The Palworld SDK exposes `UPalCharacterImportanceManager::GetAllPalCharacter`, but replacing `FindAllOf` alone cannot remove the measured admission cost.

## Decision

Separate structural candidate updates, scalar nearest-N ordering, and Blueprint presentation.

- Keep one wrapper-safe complete snapshot for playable-world bootstrap, profile entry, and explicit query rebuilds.
- BeginPlay and Pal initialization hooks copy only the composite instance key and object path into the existing bounded readiness queue. They never retain the callback UObject wrapper.
- Process one queued Actor path per callback. Admit or refresh only that Actor, update its scalar candidate record, and mark cached presentation dirty.
- Death, capture, EndPlay, and inactive pooled-Actor hooks remove only the matching scalar candidate and mark cached presentation dirty. They never call Blueprint `RemoveTarget`.
- Every 250 milliseconds, recompute current-camera distances from copied scalar coordinates. When more candidates than the visible limit are eligible, refresh at most two Actor positions by path per tick.
- A nearest-membership change never calls `FindAllOf` and never re-runs Pal field admission.
- Cached presentation resolves only the desired nearest N paths in the current callback. It verifies that every resolved Actor reproduces the stored composite instance key before changing Blueprint state.
- After all desired Actors resolve, submit exactly one `ClearTarget` followed by ordered `SetTarget` calls. A transient resolution failure leaves the previous target arrays intact and retries later. A candidate is removed only after the configured consecutive-miss threshold.
- Coalesce event bursts naturally through one scalar dirty flag consumed by the 250-millisecond runtime tick. A burst can perform several single-Actor admissions, but presentation is submitted at most once per tick.
- After 120 metres of accumulated camera travel, no more than once every two seconds, run an identity-and-coordinate-only `FindAllOf` census. Known Actors update copied paths and coordinates; unknown Actors enter the path queue. The census never synchronously admits every loaded Pal.
- Keep Overlay live parameter and CharacterID validation. A mismatch fails closed during iteration and waits for the next atomic cached presentation; it never mutates parallel arrays in `ForEachTarget`.
- Keep the existing Lua plus LogicMod package. Do not introduce a runtime DLL.

This decision supersedes ADR-0011 and replaces the incremental Blueprint mutation portions of ADR-0010. ADR-0001's package boundary and ADR-0005/ADR-0006's player gate, generation scoping, and UObject lifetime constraints remain in force.

## Options Considered

### Option 1: Increase the Full-Rebuild Interval

Advantages:
- Minimal implementation change.

Disadvantages:
- Preserves a 300 ms long frame.
- Makes nearest-N correctness and streamed-Pal discovery visibly stale.
- Treats timing as the problem even though admission is the measured bottleneck.

### Option 2: Pal Character Importance Manager

Advantages:
- Uses Palworld's own character registry instead of global object enumeration.
- May reduce the approximately 30 to 40 ms discovery component.

Disadvantages:
- Does not remove the 176 to 245 ms all-Actor admission component.
- Lua out-parameter behavior and client registry completeness require separate runtime proof.

### Option 3: Event Deltas with Atomic Cached Presentation

Advantages:
- Preserves the identity-safe clear/set contract.
- Makes distance ordering independent of global discovery and field admission.
- Bounds periodic path work and scales presentation work with N rather than all loaded Actors.
- Retains the current Workshop and Nexus package format.

Disadvantages:
- Newly streamed Pals can appear after the readiness queue and next runtime tick rather than immediately.
- A 120-metre census still has the cost of one `FindAllOf` enumeration.
- Large visible limits require resolving and resubmitting more paths after a structural change.

### Option 4: Native Weak-Reference Runtime Registry

Advantages:
- Provides direct spawn/despawn delegates and native weak references.
- Offers the greatest long-term performance headroom.

Disadvantages:
- Adds a runtime DLL, ABI maintenance, packaging changes, and a larger security boundary.
- Requires a separate decision that supersedes ADR-0001.

## Rationale

Option 3 addresses the measured cost directly while preserving the only synchronization form that passed identity validation. The candidate pool uses only Lua scalars between callbacks, so it also preserves the UObject lifetime safety established after the delayed-wrapper crashes.

Offline integration tests exercise a 22-Actor load burst. Single-Actor admission callbacks complete independently, while the final 27/28-target cached clear/set costs approximately `7-14 ms` in the test runtime and performs no `FindAllOf`. This is not a substitute for Steam frame-time validation, but it removes the code path responsible for the measured `292-365 ms` periodic hitch.

## Consequences

Positive effects:
- Camera-distance reordering performs zero global scans and zero full-field admissions.
- New, refreshed, dead, captured, and inactive Pals update one scalar registry entry.
- Blueprint target and metadata arrays always originate from one atomic ordered submission.
- Duplicate initialization hooks coalesce by composite instance key and one presentation-dirty flag.
- A stale object path cannot admit a replacement Actor under an older instance key.

Negative effects:
- Exact positions of non-visible moving candidates converge under a two-path-per-tick budget.
- Event admission is distributed across short callbacks and can take several frames during a large streaming burst.
- Very large N values make a structural presentation update more expensive than small N values.
- Full profile/query rebuilds remain synchronous and should not be triggered continuously.

Needs follow-up:
- Verify movement no longer emits `ATOMIC_REBUILD_COMPLETED reason=nearest_membership`.
- Measure `CACHED_TARGETS_ATOMIC_SYNCED` duration at limits 3, 60, and 100.
- Verify streamed additions, death, capture, pooled deactivation/reactivation, and same-path replacement retain correct identity.
- Capture PresentMon Mod-on/off travel traces and compare long-frame counts.
- Consider the Pal importance manager only if the 120-metre census remains perceptible.
- Escalate to a native runtime ADR only if bounded Lua path work still fails the frame-time gate.

## Follow-ups

- [x] Add cached nearest-N atomic clear/set synchronization.
- [x] Route BeginPlay and initialization through the path-only readiness queue.
- [x] Remove runtime use of single-target Blueprint removal.
- [x] Replace movement full rebuild with identity/coordinate census.
- [x] Add lifecycle burst, death, current-nearest, replacement-instance, and movement regressions.
- [x] Reuse the unchanged identity-safe Pak, package the Lua candidate, and deploy it reversibly.
- [ ] Validate identity and frame pacing in Steam Palworld.
- [ ] Accept or revise this ADR from runtime evidence.
