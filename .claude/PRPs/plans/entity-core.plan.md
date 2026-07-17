# Entity Core Plan

## Status

`IMPLEMENTED - objective performance benchmark pending`

## Summary

Phase 2 of the product roadmap turns the proven Pal-only runtime spike into a small extensible entity pipeline. It introduces generation-scoped normalized snapshots, an adapter registry, explicit field availability, deterministic filtering/ordering, and separate admission/display budgets without widening the player boundary or adding final UI features.

The phase must preserve the validated Blueprint bridge and visible guides. It does not attempt unsafe Pal fields merely to make the snapshot look complete.

## Baseline Evidence

- Steam build `24181527` and UE4SS `v3.0.1 Beta #0` / SHA `c2ac2464`.
- Lua player audit reports `candidate_player_count=0`.
- Current reconciliation rebuilds the candidate table every 5000 ms and clears it before map teardown.
- Current final-gate samples observed 10-33 raw monsters, 7-30 accepted wild Pals, and 70-233 ms reconciliation time.
- One Widget renders all submitted actors and rejects dead actors per frame.
- Capture, death, return to Title, and normal exit passed.
- Lua wrapper identity is not stable across later scans.
- Level is a proven Lua-safe scalar. Species is FName userdata, gender is Blueprint-only, and complex fields remain unproven.
- The diagnostic `MAX_DISPLAY_TARGETS = 512` is a capacity ceiling, not an acceptable default visual budget.

## Decisions Requiring Approval

Maintainer approval was recorded on 2026-07-16. The following values are the implementation baseline; runtime acceptance remains subject to EC-01 through EC-12.

The following values are proposed for the MVP contract:

| Setting | Proposed value | Rationale |
|---|---:|---|
| Reconciliation interval | 5000 ms | Already stable; current 70-233 ms samples do not justify a faster full scan |
| Admission capacity | 512 loaded entities | Existing fail-safe ceiling; separate from rendering |
| Default distance when the UI enables distance filtering | 2000 m | Useful local search without implying remote loading |
| Allowed distance range | 10-50000 m | Product requirement; clamps configuration only |
| Default display budget | 64 entities | Preserves multi-target utility while limiting per-frame clutter and projection work |
| Maximum configurable display budget | 512 entities | Keeps the proven diagnostic ceiling available to advanced users |

Distance filtering remains inactive in this phase so the current visible behavior is preserved. Phase 4 will activate and persist the default through the settings panel.

## Success Criteria

1. Every admitted entity passes the non-configurable player gate before adapter classification.
2. The current world session owns exactly one replaceable snapshot generation.
3. Snapshot replacement and map pre-load clear references without iterating stale actors.
4. A record follows the ADR-0005 field-state contract and retains the current `fields` data that the spike currently computes and discards.
5. Active filters never match `bridge` or `unavailable` fields.
6. Filter output is ordered deterministically and capped independently from admission capacity.
7. The Pal adapter reproduces the existing wild/owned/dead/unknown behavior without new native accessors.
8. A synthetic second adapter can be registered in tests without changing player-gate, filter, or renderer modules.
9. Runtime metrics expose raw, admitted, matched, displayed, budget-rejected, and per-stage timing counts.
10. `candidate_player_count=0`, capture/death cleanup, map teardown, visible guides, and normal exit remain passing.
11. No player identity, coordinates, save mutation, server RPC, external memory access, or forced loading is introduced.

## Non-Goals

- `Shift+Y` panel, localization resources, or configuration persistence.
- Species/name, element, gender, Lucky/Alpha, passive-skill, IV, or capture-count filters.
- Names, levels, distances, boxes, colors, or dedicated off-screen indicator UI.
- Chest, egg, statue, ore, or special-tree runtime adapters.
- Faster scan frequency, per-frame Lua scanning, or a 50000 m loading claim.
- Multiplayer verification, Game Pass, C++ runtime migration, offsets, signatures, or external memory reads.
- Rebuilding LogicMod assets unless the existing actor-only bridge contract unexpectedly changes.

## Hard Invariants

The pipeline order is fixed:

```text
UE4SS discovery/notification
  -> object validity
  -> hard player-representation rejection
  -> category adapter admission
  -> generation-scoped normalized record
  -> filter evaluation
  -> deterministic ordering
  -> display budget
  -> existing Blueprint actor bridge
```

Rules:

- Adapters cannot bypass or override the player gate.
- Unknown types and unreadable ownership fail closed.
- No filter can re-admit a rejected object.
- No renderer can enumerate actors.
- Actor references are never serialized, logged, or retained across map sessions.
- Notification additions may update the current generation, but the next reconciliation replaces the complete generation.

## Snapshot Contract

The logical record shape is:

```lua
{
    kind = "pal",
    actor = actor,
    session_id = 3,
    generation_id = 12,
    ordinal = 7,
    source = "reconcile",
    accepted_at = 123.45,
    fields = {
        level = { state = "known", value = 18, source = "parameter.method:GetLevel" },
        distance_m = { state = "known", value = 245.2, source = "camera_to_actor" },
        species = { state = "bridge", reason = "fname_userdata" },
        gender = { state = "bridge", reason = "blueprint_enum_adapter" },
        passive_skills = { state = "unavailable", reason = "adapter_not_proven" },
    },
}
```

Constraints:

- `known` values are booleans, finite numbers, strings produced without Unreal userdata conversion, or lists of those scalars.
- `bridge` never retains an opaque Lua value.
- `unavailable` includes a stable machine-readable reason.
- Runtime actor objects remain outside pure unit-test fixtures.
- A snapshot is an ordered array plus a current-generation lookup used only for deduplication.

## Adapter Contract

Each adapter descriptor provides:

```lua
{
    id = "pal",
    find_all = function(runtime) ... end,
    notification_class = "/Script/Pal.PalMonsterCharacter",
    classify = function(runtime, actor) ... end,
    normalize = function(runtime, admitted) ... end,
}
```

The core owns registration, capacity, snapshot IDs, filtering, ordering, bridge submission, and metrics. The adapter owns category-specific class checks, ownership/lifecycle classification, and proven field extraction. The Pal adapter may call only the already validated accessors in this phase.

## Filter Semantics

The core implements pure-data predicates needed by later phases:

- exact scalar or set membership;
- numeric minimum/maximum ranges;
- boolean equality;
- list `any`, `all`, and `exclude` modes;
- kind inclusion;
- distance range;
- composition with logical AND across active fields.

Within a field, the selected mode defines OR/AND/exclusion behavior. Across different active fields, all conditions must match. An active condition on a non-`known` field returns `false` with reason `field_not_known`; no default value is invented.

This phase activates only synthetic unit-test filters and optional diagnostic level/distance filters. Product Pal filters remain disabled until Phase 3 providers are validated.

## Ordering and Budgets

- Admission stops at `MAX_ADMITTED_ENTITIES` and logs one budget reason per generation.
- Filtering runs over admitted records without actor dereference.
- Known distance sorts ascending; unknown distance sorts after known distance.
- Discovery ordinal is the stable tie-breaker inside one generation.
- Display output stops at `MAX_DISPLAY_TARGETS`.
- Metrics distinguish admission-capacity rejection from display-budget truncation.
- The Widget receives only the final ordered actor list.

## Planned Repository Layout

```text
PalworldResourceESP/
  Scripts/
    main.lua
    config.lua
    core/
      adapter_registry.lua
      entity_snapshot.lua
      filter_engine.lua
    adapters/
      pal.lua
tests/
  lua/
    entity_snapshot_spec.lua
    filter_engine_spec.lua
    adapter_registry_spec.lua
  run-lua-tests.mjs
package.json
package-lock.json
docs/
  adr/0005-use-generation-scoped-entity-snapshots.md
  testing/entity-core-matrix.md
```

`package.json` contains development-only `fengari` and `luaparse` dependencies so the current Windows environment can parse and execute pure Lua tests. No Node dependency is shipped under `PalworldResourceESP` or loaded by the game.

## Tasks

### Task 1: Approve the Contract and Baseline

- Review and accept or revise ADR-0005.
- Confirm the proposed interval, distance, admission, and display defaults.
- Record Steam/UE4SS/PMK/LogicMod/Lua hashes before implementation.
- Preserve the current active Lua and pak in a dated rollback snapshot.

### Task 2: Add a Reproducible Pure-Lua Test Harness

- Add pinned `luaparse` and `fengari` development dependencies.
- Parse every shipped Lua file.
- Execute core tests without UE4SS globals.
- Fail on missing assertions, syntax errors, or accidental game-runtime imports from pure core modules.

### Task 3: Implement Field Cells and Snapshot Generations

- Implement constructors/validators for `known`, `bridge`, and `unavailable` cells.
- Implement ordered generation creation, current-generation dedupe, and replacement.
- Ensure replacement drops the previous tables without actor validity calls.
- Add session and generation counters to diagnostics.

### Task 4: Implement the Adapter Registry and Pal Adapter

- Put the player-representation gate in the pipeline before adapter dispatch.
- Move Pal-specific wild/owned/dead/unknown classification behind the adapter contract without changing accessor order.
- Retain normalized fields on each record.
- Mark unsafe fields with explicit states instead of probing new paths.
- Prove extension with a test-only fake adapter.

### Task 5: Implement Filters, Ordering, and Budgets

- Implement the pure-data predicate modes and fail-closed field-state behavior.
- Annotate reconciliation-time distance through the existing safe location path when available.
- Sort matched records by known distance and ordinal.
- Apply the display budget after filtering.
- Keep all product filters disabled by default in this phase.

### Task 6: Integrate the Existing Blueprint Bridge

- Submit only the ordered, budgeted actor list.
- Preserve session reset, empty-snapshot clear, passive `ModActor` discovery, and gender diagnostics.
- Do not rebuild Blueprint assets unless the actor-only event signature changes.
- Keep per-frame validity/death projection in the Widget.

### Task 7: Expand Metrics and Static Safety Checks

- Emit raw, admitted, matched, displayed, admission-rejected, display-truncated, notification, and timing metrics.
- Keep `PLAYER_AUDIT ... candidate_player_count=0` as a release-blocking invariant.
- Scan source and logs for player identity/location fields and prohibited mutation capabilities.
- Document field-state reasons without logging opaque values.

### Task 8: Run the Entity Core Smoke Matrix

- Deploy Lua through the existing Junction; do not relaunch the game after source edits without a full restart.
- Enter a Steam single-player save near multiple wild Pals.
- Confirm counts satisfy `raw >= admitted >= matched >= displayed` and `displayed <= 64`.
- Confirm visible guides remain independent and top-anchored.
- Capture one tracked Pal and kill another; both must disappear and stay excluded.
- Return to Title, wait, and exit normally.
- Record timings, hashes, no-pending diagnostics, and crash-directory timestamps.

### Task 9: Close the Phase

- Update the PRD, ADR-0005, runtime report, and entity-core matrix with observed evidence.
- Change ADR-0005 to `Accepted` only if the runtime gate passes.
- Commit and push on `codex/entity-core`, open a Draft PR during implementation, and mark it Ready only after the matrix passes.

## Validation Commands

```powershell
npm ci
npm test
git diff --check
rg -n "candidate_player_count|ENTITY_SNAPSHOT|FILTER_RESULT|DISPLAY_BUDGET" PalworldResourceESP docs tests
rg -ni "player name|player uid|platform id|steam id|location=" PalworldResourceESP LogicMod docs tests
rg -ni "ReadProcessMemory|WriteProcessMemory|SetActorLocation|Teleport|SpawnActor|ServerTravel" PalworldResourceESP LogicMod tests
```

Runtime validation remains mandatory because unit tests cannot prove UObject lifetime, UE4SS wrapper behavior, actor accessors, bridge calls, or Blueprint rendering.

## Runtime Matrix

| ID | Scenario | Required evidence |
|---|---|---|
| EC-01 | Boot and adapter registration | Pal adapter registered once; no core import error |
| EC-02 | Player boundary | `candidate_player_count=0`; no player record or bridge actor |
| EC-03 | Generation replacement | Generation increments; old snapshot dropped without per-actor teardown access |
| EC-04 | Field states | Level/distance are known when available; unsafe fields are bridge/unavailable |
| EC-05 | Filter semantics | Synthetic exact/range/any/all/exclude and unknown-state tests pass |
| EC-06 | Ordering and budget | Known distance ascending; ordinal tie-break; displayed never exceeds 64 |
| EC-07 | Notification/reconcile interaction | No duplicate current-generation record; next reconcile replaces state |
| EC-08 | Capture and death | Both targets disappear and cannot re-enter while owned/dead |
| EC-09 | Return to Title | Snapshot and bridge clear before old-world teardown |
| EC-10 | Performance | Per-stage timing logged; no faster full-scan interval introduced |
| EC-11 | Adapter extension | Test-only second adapter requires no player-gate/filter/renderer changes |
| EC-12 | Normal exit | No new crash report; source and active deployment hashes match |

## Rollback

- Snapshot the active Lua files and any changed pak before deployment.
- New incorrect files are renamed `__MISTAKE_YYYYMMDD_<name>` rather than deleted.
- Superseded generated or deployment artifacts are renamed `__DEPRECATED_YYYYMMDD_<name>`.
- Restore the previous Lua snapshot and restart Palworld for runtime rollback.
- Use ordinary Git revert commits after push; never reset shared history or force push.

## Phase Gate

Entity Core passes only when pure-data tests, EC-01 through EC-12, the player-exclusion invariant, map teardown, visible guides, and normal exit pass on Steam single-player. Multiplayer remains community-pending. Rich Pal filters and the `Shift+Y` UI cannot begin until the snapshot/adapter/filter contract is accepted.
