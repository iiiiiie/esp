# ADR-0005: Use Generation-Scoped Entity Snapshots

## Status

Proposed

## Date

2026-07-16

## Context

The runtime and Blueprint bridge spikes proved that UE4SS Lua can discover loaded wild Pals, reject player representations before candidate insertion, and hand accepted actors to one Blueprint Widget. They also exposed constraints that the Entity Core must preserve:

- UE4SS may return different Lua wrapper identities for the same UObject across later `FindAllOf` calls.
- Retained wrappers must not be dereferenced during map teardown.
- Some values are safe Lua scalars, some are available only through typed Blueprint nodes, and some have no proven accessor.
- Guessing enum, array, struct, or userdata conversions can crash the GameThread rather than raise a catchable Lua error.
- Rich filters and future resource adapters need one contract for unknown values, admission, ordering, and display budgets.
- Human players must remain impossible to admit even when new entity categories are added.

A persistent registry keyed by Lua wrapper identity would conflict with the observed runtime behavior. A Blueprint-owned registry would move discovery past the player-exclusion boundary. A C++ runtime registry would add cost before Lua and Blueprint performance limits are known.

## Decision

Use a Lua-owned, generation-scoped entity snapshot pipeline.

- Lua remains the only discovery and admission layer.
- The hard player-representation gate runs before every category adapter and cannot be disabled by configuration.
- Each reconciliation creates a new snapshot generation. Actor wrapper references are valid only inside the current generation and current world session.
- Map pre-load clears the snapshot and bridge references without dereferencing retained actors.
- Category-specific classification and normalization live behind registered adapters. Adding an adapter may add one registration entry, but must not alter the player gate, filter engine, or renderer core.
- A normalized record has an entity kind, current actor reference, session/generation/ordinal metadata, source, and named field cells.
- Field cells use explicit states:
  - `known`: a canonical Lua-safe scalar or scalar list is available.
  - `bridge`: the value requires a typed Blueprint adapter and no Lua value is fabricated or retained.
  - `unavailable`: no proven path exists for the current game/toolchain fingerprint.
- When a filter is inactive, its field state has no effect. When a filter is active, `bridge` and `unavailable` do not match until an explicit provider implements that field.
- Filtering runs before ordering and the display budget. Ordering is deterministic within one generation: known distance ascending, then discovery ordinal.
- The Blueprint bridge receives only actors from the filtered, budgeted output. Blueprint may enrich or filter already admitted wild entities in later phases, but it must never discover actors or widen admission.
- No snapshot, filter, or diagnostic may contain player identity, player coordinates, or player parameters.

The first Entity Core implementation proves the contract with Lua-safe fields such as level and reconciliation-time distance. Gender is the first validated Blueprint field provider: Blueprint maps `EPalGenderType` to `0 = unknown`, `1 = male`, or `2 = female` for each already-admitted target and applies the selected filter before projection. Lucky is a separate Blueprint provider backed by `UPalIndividualCharacterParameter::IsRarePal()`: `-1 = unknown`, `0 = normal`, and `1 = Lucky`. Alpha/Boss is another independent Blueprint provider: it reads `GetCharacterID()` from the admitted target, resolves `UPalDatabaseCharacterParameter` through `UPalUtility::GetDatabaseCharacterParameter()`, and calls `GetIsBoss()` to produce `-1 = unknown`, `0 = normal`, or `1 = Boss`. Lucky never implies Boss. Both three-state selectors reject unknown values in either restricted mode. Elements use a fourth Blueprint provider: it calls `HasElementType()` for the nine supported `EPalElementType` values and stores one per-target bit mask (`Normal=1`, `Fire=2`, `Water=4`, `Leaf=8`, `Electricity=16`, `Ice=32`, `Earth=64`, `Dark=128`, `Dragon=256`). No selected bit means all elements; multiple bits use match-any semantics; an unknown target mask is `-1` and fails closed when filtering is active. IV display uses a fifth typed Blueprint provider: it calls `GetSaveParameter()`, breaks `FPalIndividualCharacterSaveParameter`, and reads `Talent_HP`, `Talent_Shot`, and `Talent_Defense` as `0..100` integers. Provider failure stores `-1` for all affected per-target entries; enhancement ranks are never substituted. Lua reads only scalar selectors, the aggregate `0..511` element filter mask, and the IV visibility toggle; it never receives the enums, save struct, character ID, database object, provider results, or a field-derived Actor reference. Species, passive skills, IV threshold filtering, and capture count remain explicit `bridge` or `unavailable` values until their dedicated adapters are validated.

## Options Considered

### Option 1: Persistent UObject Registry Keyed by Lua Wrappers

Advantages:

- Incremental updates appear simple.
- Fewer full snapshot allocations.

Disadvantages:

- Wrapper identity is not stable across scans on the tested UE4SS build.
- Stale wrapper cleanup previously crashed during world teardown.
- Requires an unproven stable native identifier.

### Option 2: Generation-Scoped Lua Snapshots with Adapters

Advantages:

- Matches the lifecycle behavior already proven in runtime tests.
- Keeps the player boundary before all adapters and rendering.
- Gives filters explicit unknown-value semantics.
- Supports synthetic unit tests without loading Unreal.

Disadvantages:

- Rebuilds normalized records every reconciliation.
- Requires deliberate Blueprint adapters for fields unsafe in Lua.
- Notification updates must be reconciled with generation replacement.

### Option 3: Blueprint-Owned Registry

Advantages:

- Typed access to Pal enums and arrays.
- Convenient integration with the future settings panel.

Disadvantages:

- Weakens the existing discovery and player-exclusion ownership boundary.
- Makes non-Widget lifecycle and category adapters harder to test.
- Cannot replace UE4SS discovery for all planned resources.

### Option 4: C++ Runtime Registry

Advantages:

- Strong typing and potential performance headroom.
- Direct control over stable keys and memory layout.

Disadvantages:

- Premature without a measured Lua/Blueprint bottleneck.
- Adds ABI and game-update maintenance.
- Expands ADR-0001 and would require a separate runtime migration decision.

## Rationale

Option 2 is the smallest design consistent with the evidence. It formalizes the already stable rebuild-before-render behavior, prevents unsafe values from becoming accidental filter matches, and makes future category work additive without moving the player boundary. It accepts periodic record rebuilding because the current five-second scan interval and observed actor counts are modest, while preserving a later C++ escalation path if profiling justifies it.

## Consequences

Positive effects:

- Player exclusion remains centralized and testable.
- Missing data has one fail-closed meaning across filters.
- Snapshot, filter, ordering, and budget logic can be unit tested outside Unreal.
- Future resource adapters do not need to change renderer ownership.
- Map teardown does not walk stale actor references.

Negative effects:

- Rich Pal filters cannot be declared supported until their field providers pass runtime tests.
- Snapshot generations allocate new Lua tables on reconciliation.
- Lua and Blueprint may each own part of field normalization for types that cannot cross the bridge safely.

Follow-up concerns:

- Do not treat generation ordinal as a persistent entity identity.
- Do not log opaque Unreal values or convert them speculatively.
- Do not describe the distance filter as an entity-loading radius.
- Keep scan admission capacity separate from the smaller display budget.
- Record a new ADR before moving the canonical registry into C++ or external memory.

## Follow-ups

- [x] Implement the Entity Core PRP and unit-test the adapter/filter contracts.
- [x] Implement the gender Blueprint field provider without marshalling `EPalGenderType` through Lua.
- [x] Validate all/male/female filtering, panel-state restoration, and lifecycle cleanup in Steam single-player.
- [x] Implement the Lucky Blueprint field provider without exposing the result or UObject to Lua.
- [ ] Validate all/only Lucky/exclude Lucky behavior, panel-state restoration, and lifecycle cleanup in Steam single-player.
- [x] Implement the independent Alpha/Boss Blueprint provider without exposing CharacterID or the database object to Lua.
- [ ] Validate all/only Boss/exclude Boss behavior against a fixed-map Alpha/Boss and ordinary Pal in Steam single-player.
- [x] Implement the multi-select element Blueprint provider without marshalling `EPalElementType` through Lua.
- [ ] Validate no-selection, one-element, and multi-element match-any behavior with ordinary Steam single-player samples.
- [x] Implement display-only HP/attack/defense IV providers without marshalling the save struct through Lua.
- [ ] Validate displayed `Talent_HP`, `Talent_Shot`, and `Talent_Defense` values with ordinary Steam single-player samples before adding minimum-threshold filters.
- [ ] Validate snapshot replacement, capture/death cleanup, map teardown, and normal exit in Steam single-player.
- [ ] Measure reconciliation, filtering, ordering, and bridge synchronization separately.
- [ ] Accept this ADR only after the Entity Core smoke matrix passes.
- [ ] Add explicit Blueprint field-provider contracts for the remaining Pal Filter MVP fields.
