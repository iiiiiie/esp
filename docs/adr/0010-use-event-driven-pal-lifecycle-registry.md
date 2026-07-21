# ADR-0010: Use Event-Driven Pal Lifecycle Updates with Movement Integrity Discovery

## Status

Superseded by ADR-0011

## Date

2026-07-20

## Context

The production `chunked_current` profile still performed one synchronous full reconciliation every five seconds. A fixed scene with 21 loaded Pal actors measured `118-131 ms` per reconciliation: approximately `30 ms` in `FindAllOf` discovery, `88-96 ms` in admission and field access, and only `1-2 ms` in Blueprint bridge synchronization. This produced a reproducible five-second GameThread hitch and visible target-array refresh.

Delayed Lua processing is not a safe smoothing mechanism. Two earlier movement runs crashed after retaining UE4SS UObject wrappers across delayed callbacks. ADR-0005 therefore keeps the hard player gate in Lua, and ADR-0006 forbids delayed admission jobs that retain wrappers.

UE4SS exposes one global BeginPlay post hook plus UFunction hooks for lifecycle transitions. Palworld exposes `PalCharacter::OnDeadCharacter` and `PalUtility::PalCaptureSuccess`. These allow already-initialized actors to be handled individually without repeating `FindAllOf` in steady state.

The first Steam validation exposed an initialization-order constraint. The playable-world bootstrap ran before any monsters were discoverable (`raw=0`), then 79 BeginPlay admissions failed with `individual_parameter_unavailable`. Toggling the Mod off and on later found 46 actors because their individual parameters were ready by then. BeginPlay therefore identifies the actor lifecycle event, but does not guarantee that Pal-specific admission fields are ready in the same callback.

A later distant-streaming run exposed a second lifecycle constraint. Blueprint followed the current Actor transform but continued using name, species, IV, passive, and filter metadata cached when that Actor was first submitted. After new streaming cells loaded, this could pair one Pal's visible guide with another Pal's metadata. The rolling-window code also decremented Lua bookkeeping when an old object path no longer resolved, without removing the corresponding Blueprint entry, which could shift the parallel target/metadata arrays out of alignment.

The first current-nearest implementation introduced a third constraint. It resolved every candidate path and read every Actor location every 250 milliseconds, and repeated the same complete pass during each admission. With only 27 candidates, admission callbacks grew from `86 ms` to `798 ms`; the configured visible limit was 60, so all 27 targets were already eligible and none of those full passes could change nearest-N membership. This produced the reported periodic, unplayable GameThread stutter.

The bounded replacement removed most of that stutter, but a second distant-streaming run still reproduced wrong Pal identity. Both parameter post-hooks registered successfully, yet the run emitted no `EVENT_TARGET_REFRESHED` records. `IsInitialized()` therefore does not prove that an Actor's `IndividualParameter` will remain stable, and client-side lifecycle hooks are an opportunistic fast path rather than a complete identity contract. The same run also proved that a full UObject path can stop resolving and later identify another object instance; a full-name path alone cannot be treated as immutable Actor identity.

The next Steam launch proved that BeginPlay and parameter hooks are also incomplete discovery signals. The runtime admitted 48 event targets, but after `04:03:15` the player travelled through newly streamed Pals for about 40 seconds without another construction, BeginPlay, active-target, or parameter event reaching the Mod. The new Pals therefore never entered the candidate pool. Reintroducing a wall-clock full reconciliation would restore the measured stationary hitch, so the integrity path must be tied to world traversal and must avoid synchronously rebuilding metadata for every loaded Actor.

## Decision

Use event-driven lifecycle updates as the fast path and movement-bounded identity discovery as the completeness path.

- Run one full, synchronous snapshot when entering a playable world, changing a query that requires Lua fields, or explicitly changing runtime profile.
- Set the default active profile and `event_first` profile to a zero millisecond reconciliation interval. Target-display presets do not control scan frequency.
- Reuse the single `RegisterBeginPlayPostHook`. After an actor has completed BeginPlay, process only `PalMonsterCharacter` instances.
- Run the registry-owned human-player rejection gate, ownership checks, death checks, field normalization, and active Lua filters before any event target reaches Blueprint.
- Require `APalCharacter::IsInitialized()` to return true in addition to current individual/save parameter availability before admission.
- Derive the runtime Actor identity from both `GetFullName()` and UE4SS `GetAddress()`. Keep the object path separately for bounded lookup. A path lookup is accepted only when the resolved Actor reproduces the queued or registered composite identity; a different object at the same path is a replacement instance, not the old candidate.
- Attempt admission directly from BeginPlay. When it fails for a bounded set of initialization-only reasons, store only the composite Actor identity, `GetFullName()`-derived object path, lifecycle generation, and attempt count. Resolve that exact path with UE4SS `StaticFindObject` after 250 milliseconds, process one entry per callback, and stop after eight attempts.
- Register post-hooks for `PalCharacter::OnRep_IsPalActiveActor`, `PalCharacter::LocalInitialized`, `PalCharacterParameterComponent::OnRep_IndividualParameter`, `PalCharacterParameterComponent::OnInitialize_AfterSetIndividualParameter`, `PalCharacterParameterComponent::OnInitializedCharacter`, and `PalCharacter::BroadcastOnCompleteInitializeParameter`. Because UE4SS requires a pre callback for `/Script/` UFunction hooks, register a no-op pre callback plus the guarded post callback. Active transitions admit or remove one pooled Actor; initialization hooks may accelerate admission or refresh, but correctness must not depend on any one hook firing client-side.
- Never retain an event callback's UObject wrapper across ticks. Permanent player, ownership, death, and non-distance filter rejections are not retried. Death, capture, EndPlay, profile changes, and world teardown cancel matching pending paths.
- Copy the camera location to Lua number scalars every 250 milliseconds and accumulate travelled distance. After 120 metres of traversal, and no more than once every two seconds, run one identity-only `FindAllOf` pass. Stationary time never triggers this pass.
- During the integrity pass, refresh only the object path and coordinate scalars of known composite instance keys. For an unknown key, enqueue only its composite identity, path, lifecycle generation, and attempt count; perform normal player/ownership/death/initialization/filter admission later through the existing bounded path queue. Never retain an enumerated UObject after the pass and never synchronously rebuild metadata for known Actors.
- Add one target through the existing typed Blueprint bridge. Remove one target on `ReceiveEndPlay`, `OnDeadCharacter`, or `PalCaptureSuccess` through a new typed `RemoveTarget` bridge event.
- Maintain a world-session candidate pool containing only composite instance keys, object paths, Lua-safe level values, copied world-coordinate scalars, and resolve-miss counters. The pool includes every admitted candidate matching non-distance Lua filters, not only the currently displayed targets.
- Every 250 milliseconds, recompute distance for all candidates from the current camera location and copied candidate coordinates, then sort those Lua numbers with a deterministic key tie-break. This scalar pass performs no candidate-path lookup.
- When the eligible candidate count is at or below `N`, perform no periodic candidate `StaticFindObject` calls. When the pool exceeds `N`, refresh Actor positions and converge boundary membership incrementally, with a hard maximum of two candidate path resolutions in one runtime tick. Never carry a resolved Actor wrapper into the next tick.
- Treat the configured target limit as the current-camera nearest `N` candidates using each Actor's latest observed coordinate. Compare the desired top `N` with the displayed key set and call Blueprint remove/set only for targets crossing that boundary. A newly admitted or initialization-refreshed Actor contributes its current coordinate immediately; movement of older Actors above the limit converges through the bounded refresh cursor. Lifecycle removal immediately promotes the next-nearest eligible target.
- Tolerate three transient path-resolution misses. On the fourth consecutive miss, remove that candidate. If it was displayed, clear the complete Blueprint target set and rebuild incrementally from the remaining nearest candidates without calling `FindAllOf` or exceeding the per-tick path budget.
- The Overlay stores the `IndividualParameter` reference and `CharacterID` observed when each target's typed metadata is built. Before drawing, it resolves the Actor's current parameter and compares both the parameter object and CharacterID with the aligned cached values. An unavailable parameter fails closed for that frame. A mismatch draws nothing, removes the old aligned entry, and immediately re-adds the same Actor with its current level, distance, name, species, gender, IVs, passive skills, and filter metadata. This Blueprint-owned repair does not wait for a Lua hook or global Actor scan.
- The Overlay removes the same index from every target, parameter, and metadata array. Full clear remains available for world teardown, runtime-off, profile changes, and query rebuilds.
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
- Actor movement above the visible limit converges incrementally instead of being globally exact in one tick.
- Targets close to the `N`/`N+1` boundary may exchange repeatedly as their distance order changes.
- Requires Steam runtime validation for every lifecycle hook.

### Option 4: Native C++ Spawn Registry

Advantages:
- Provides weak UObject references and the highest performance headroom.
- Can use `UWorld::AddOnActorSpawnedHandler` directly.

Disadvantages:
- Adds a runtime DLL and ABI maintenance.
- Expands the current deployment and security boundary.

## Rationale

Option 3 plus movement integrity discovery removes the measured wall-clock periodic bottleneck without introducing the runtime DLL cost of Option 4. It also avoids the already-reproduced wrapper-lifetime failure of Option 2. Full metadata snapshots remain explicit repair points; traversal performs only an identity/coordinate census and defers new-Actor admission by path.

Composite Actor identities, pooled-Actor activation hooks, Blueprint live-binding validation, and the scalar path pool keep the normal path event-driven without restoring a stationary `FindAllOf` timer. Copying coordinates at Actor-owned events makes normal travel cheap; the 120-metre census repairs lifecycle events that Palworld omits during object-pool streaming. Camera movement reorders every cached candidate using Lua numbers, while Actor movement above the target limit converges under a fixed lookup budget. Temporarily hiding or rebuilding one guide is preferable to displaying a confidently wrong Pal identity.

## Consequences

Positive effects:
- A fixed scene no longer receives a 100+ ms Mod callback every five seconds.
- New loaded Pals require one admission instead of re-admitting every loaded Pal.
- Pals whose individual parameters are not ready at BeginPlay can become visible without a global repair scan.
- Same-path replacement Actors cannot satisfy an older queue or candidate identity.
- Parameter rebinding is detected from the live Blueprint Actor reference even when client-side lifecycle hooks do not fire, and all cached metadata is rebuilt before display continues.
- The target limit selects the current-camera nearest admitted `N` candidates from latest observed Actor positions rather than the most recently initialized candidates.
- Death, capture, and streamed EndPlay remove only the affected target.
- Line arrays no longer clear and rebuild on a timer.

Negative effects:
- Event hooks can be omitted by client-side object-pool streaming, so movement discovery is required for completeness.
- A newly loaded Pal may appear up to the bounded readiness-retry window after BeginPlay.
- Initial world entry and query changes still perform one synchronous full snapshot.
- Crossing each 120-metre accumulated travel boundary performs one synchronous identity-only `FindAllOf`; current evidence measured discovery near `35 ms`, so moving frame-time still requires Steam validation.
- Actor movement can take several bounded ticks to affect membership when more than `N` candidates are eligible.
- A persistently unresolved displayed path temporarily clears every guide before an incremental rebuild from the remaining path pool.
- The Overlay performs a small parameter/component identity check for each otherwise drawable target; this adds steady per-frame Blueprint work in exchange for immediate fail-closed identity correctness.

Needs follow-up:
- Verify no periodic `SCAN_DONE` appears during a 60-second fixed scene.
- Verify `EVENT_TARGET_ADDED` remains below `8 ms` while moving through streaming cells.
- Measure fixed-view and travel runs below and above the configured target limit; confirm below-limit ticks perform zero candidate path lookups and above-limit ticks never exceed two.
- Verify death, capture, EndPlay, return-to-title, and normal exit.
- Verify distant unload/return and new streamed spawns never exchange displayed species identity or filter membership.
- Verify `candidate_player_count=0` and no player target reaches either event bridge call.
- Compare fixed-view and movement captures with the previous stable Pak.
- Escalate to a separate C++ runtime ADR only if event callbacks still exceed the frame budget or miss lifecycle transitions.

## Follow-ups

- [x] Implement zero-interval event-driven active profiles.
- [x] Add BeginPlay single-target admission without delayed UObject retention.
- [x] Add death, capture, and EndPlay single-target removal contracts.
- [x] Add unit and integration tests for no periodic scan and lifecycle deltas.
- [x] Add path-only readiness retries for post-BeginPlay Pal initialization.
- [x] Replace oldest-target rolling replacement with a path-only current-nearest `N` candidate pool.
- [x] Gate admission on completed character initialization and refresh displayed metadata after parameter rebinding.
- [x] Fail closed and rebuild from remaining candidate paths when a displayed path cannot resolve.
- [x] Replace the unbounded 250-millisecond path pass with cached-coordinate ranking and a two-resolution hard budget.
- [x] Replace full-name-only registry keys with full-name plus UE4SS object-address instance identities.
- [x] Add Blueprint-owned live parameter/CharacterID validation and aligned metadata rebuild independent of lifecycle hooks.
- [x] Add pooled-Actor activation/initialization fast paths and movement-bounded identity-only discovery for missed lifecycle events.
- [x] Prove the movement census queues unknown paths without synchronous admission and never repeats while stationary.
- [x] Compile, cook, and inspect the new Overlay and ModActor assets.
- [ ] Validate Steam single-player lifecycle behavior.
- [ ] Run fixed-view and movement PresentMon captures.
- [ ] Accept this ADR after the performance and lifecycle gates pass.
