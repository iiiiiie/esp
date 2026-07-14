# Plan: Runtime Spike

## Summary

Phase 1 will create a deliberately small UE4SS Lua diagnostic mod that proves the critical runtime assumptions before the project commits to the full entity core or production Widget renderer. The spike must discover currently loaded wild Pals, reject all human player representations before they enter candidate state, probe the required Pal data fields, draw one minimal top-anchored guide, and record single-player lifecycle/performance evidence. Multiplayer remains a best-effort compatibility target with a community-ready test matrix, not a maintainer-tested completion gate.

The spike is experimental code, not the production architecture. It follows ADR-0001: Lua owns discovery and normalization, while the final UI and renderer remain a LogicMod/Blueprint Widget concern; the temporary Lua/Canvas guide exists only to validate projection and rendering without blocking on the currently missing Unreal Editor/Palworld Modding Kit toolchain.

## User Story

As a Palworld collector, I want the mod to prove that it can safely find and point to loaded wild Pals without ever collecting human players, so that later filtering and rendering work is built on verified runtime data instead of assumptions.

## Problem -> Solution

The repository currently contains a product definition and architecture decision but no executable mod and no verified class/field paths for the current game build -> Build a fail-closed diagnostic Lua mod, execute a controlled single-player experiment matrix, publish a reusable community multiplayer matrix, and produce a runtime report that either unlocks Phase 2 or identifies the exact blocked assumptions.

## Metadata

- **Complexity**: Large
- **Source PRD**: `.claude/PRPs/prds/palworld-resource-pal-esp.prd.md`
- **PRD Phase**: Phase 1 - Runtime Spike
- **Estimated Files**: 5 repository files
- **Estimated Tasks**: 9
- **Current Steam Build ID**: `24088745` (must be re-read at test time)
- **Current active UE4SS baseline**: v3.0.1 Beta, Git SHA `c2ac246`, Workshop package `experimental-palworld-4` (must be re-read from the newest `UE4SS.log`)
- **Active UE4SS root**: `E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS`
- **Working internal mod identifier**: `PalworldResourceESP`; public project name remains an open product decision

---

## UX Design

### Before

```text
Game world
  Pal is loaded but the player must find and inspect it manually
  No runtime evidence that required fields are readable
  No proof that remote players are excluded before rendering
```

### After

```text
                         top-center anchor
                                |
                                | diagnostic guide
                                v
                         [one proven wild Pal]

UE4SS log/report:
  discovery source + classification reason + available field paths
  aggregate rejected-player counts only
  scan/callback/render timing and replication observations
```

### Interaction Changes

| Touchpoint | Before | After | Notes |
|---|---|---|---|
| Game launch | No project code loads | One `[PalworldResourceESP] loaded` line per process | Duplicate initialization is a failure |
| Loaded wild Pal | No project behavior | Closest proven candidate can receive one top-anchored diagnostic guide | Spike-only renderer; not the final UX |
| Human player | Not handled | Rejected before candidate insertion, field probing, location caching, or rendering | Never configurable |
| Map/session transition | No lifecycle handling | Cache is invalidated and rebuilt without stale UObject access | Must survive travel, death, and respawn |
| Diagnostics | No evidence | Structured log lines and a completed runtime report | Do not log player names, IDs, or coordinates |

---

## Architecture Decision Alignment

This phase follows `docs/adr/0001-use-ue4ss-hybrid-client-mod-architecture.md`; no new ADR is required.

- The Lua spike validates discovery, classification, field access, lifecycle, and the Lua side of projection.
- The Lua/Canvas guide is explicitly diagnostic and does not replace the accepted direction of a LogicMod/Blueprint Widget for the product UI.
- Direct offsets, signatures, external memory reading, network modification, forced entity loading, or C++ are not introduced.
- If runtime evidence later justifies any of those approaches, create a separate ADR before implementation.

---

## Mandatory Reading

Files that MUST be read before implementing:

| Priority | File | Lines | Why |
|---|---|---|---|
| P0 (critical) | `.claude/PRPs/prds/palworld-resource-pal-esp.prd.md` | 182-257, 309-343 | Defines architecture, Phase 1 scope, player exclusion, and research baseline |
| P0 (critical) | `docs/adr/0001-use-ue4ss-hybrid-client-mod-architecture.md` | all | Defines the architecture and non-negotiable acquisition-layer player exclusion |
| P0 (critical) | `../automatic_pickup/AutomaticPickup/Scripts/main.lua` | 20-66, 232-287, 330-420, 1291-1328 | Working UE4SS patterns for logging, UObject safety, `IsA`, individual parameters, hooks, and notifications |
| P1 (important) | `../automatic_pickup/AutomaticPickup/Scripts/config.lua` | all | Existing local configuration style and uppercase constant naming |
| P1 (important) | `readme.txt` | all | Original feature intent and naming vocabulary |
| P1 (important) | `E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS/UE4SS.log` | startup section and newest project lines | Runtime version, active mod root, loader errors, and actual hook behavior |
| P1 (important) | `E:/Steam/steamapps/appmanifest_1623730.acf` | build metadata | Records the exact Steam build under test |
| P2 (reference) | `E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS/Mods/BPModLoaderMod/Scripts/main.lua` | initialization and `ExecuteInGameThread` sections | Current installed UE4SS/LogicMod initialization examples |
| P2 (reference) | `E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS/Mods/shared/UEHelpers/UEHelpers.lua` | object lookup helpers | Installed examples of `FindFirstOf` and `FindAllOf` |

Do not edit the `automatic_pickup` repository or either installed UE4SS directory while implementing repository code. Deployment into the active UE4SS root is a separate, reversible validation step.

## External Documentation

| Topic | Source | Key Takeaway |
|---|---|---|
| UE4SS Lua functions | https://github.com/PalworldModding/Docs/blob/master/docs/developers/ue4ss-modding/lua-mods/ue4ss-functions.mdx | Use `/Script/` hooks for reliable initialization; hook parameters may be `RemoteUnrealParam` and require `:get()`; guard notifications against duplicate registration |
| Lua with Blueprint | https://github.com/PalworldModding/Docs/blob/master/docs/developers/ue4ss-modding/lua-mods/blueprints-with-lua.mdx | Cache and validate the ModActor, bind custom events once, and use unique custom-event names because one registration can overwrite another |
| LogicMod introduction | https://github.com/PalworldModding/Docs/blob/master/docs/developers/ue4ss-modding/logic-mods/introduction.md | End-user UI should use Widgets rather than the UE4SS GUI console; LogicMods must not use the `_P` patch-pak suffix |
| Steam Workshop packaging | https://github.com/PalworldModding/Docs/blob/master/docs/developers/mod-publishing/workshop/packaging.md | Lua + LogicMods can be packaged together and declare UE4SS; current Lua install root is under `Palworld/Mods/NativeMods/UE4SS` |

### External Research Findings

```text
KEY_INSIGHT: `ServerAcknowledgePossession` works in local and multiplayer client flows and is a better initialization signal than relying only on `ClientRestart`.
APPLIES_TO: Runtime bootstrap and post-travel reinitialization.
GOTCHA: Initialization hooks can fire more than once; all hook and notification registration must be idempotent.
```

```text
KEY_INSIGHT: `NotifyOnNewObject` only observes future constructions and non-`/Script/` classes may not exist when the Lua file first loads.
APPLIES_TO: Incremental Pal discovery.
GOTCHA: Pair one initial/low-frequency `FindAllOf` bootstrap with one guarded notification registration; never register a new notification on every possession callback.
```

```text
KEY_INSIGHT: Hook parameters can be `RemoteUnrealParam` wrappers.
APPLIES_TO: Every callback that receives a UObject or field value.
GOTCHA: Use the local `unwrap` pattern and `pcall`; direct dereference can fail or crash after object invalidation.
```

```text
KEY_INSIGHT: The active Workshop UE4SS root differs from the older `Pal/Binaries/Win64/ue4ss` layout used by the read-only reference copy.
APPLIES_TO: Development deployment and log inspection.
GOTCHA: Deploy only to the root reported by the newest `UE4SS.log`; code placed in the legacy directory may never load.
```

---

## Discovery Reference

| Category | File:Lines | Pattern | Key Snippet/Conclusion |
|---|---|---|---|
| Naming | `../automatic_pickup/AutomaticPickup/Scripts/config.lua:1-29` | `config.UPPER_SNAKE_CASE`, local snake_case functions | Keep spike constants in `config.lua`; keep helpers local |
| Entry point | `../automatic_pickup/AutomaticPickup/Scripts/main.lua:1-3,1324-1328` | `require("config")`, register once, final loaded log | `main.lua` is the UE4SS entry point |
| Error handling | `../automatic_pickup/AutomaticPickup/Scripts/main.lua:36-66` | Unwrap and validate every UObject access inside `pcall` | Fail closed on any failed read |
| Logging | `../automatic_pickup/AutomaticPickup/Scripts/main.lua:20-34` | Prefix every message with the mod name; debug logs are gated | Add stable event codes for report extraction |
| Type checks | `../automatic_pickup/AutomaticPickup/Scripts/main.lua:232-287` | Resolve `/Script/` class once and call `actor:IsA(class)` inside `pcall` | Player rejection runs before monster acceptance |
| Data access | `../automatic_pickup/AutomaticPickup/Scripts/main.lua:330-420` | Try method, property, component, and individual-handle paths | Record the first successful path; never fabricate missing values |
| Lifecycle | `.claude/PRPs/prds/palworld-resource-pal-esp.prd.md:191-193,228-230` | Event-driven cache plus low-frequency reconciliation | No per-frame full scan |
| Configuration | `../automatic_pickup/AutomaticPickup/Scripts/config.lua:3-27` | Small explicit flags and measured timing values | No public setting can enable player collection |
| Tests | Repository currently has no test harness | Runtime experiment matrix is the Phase 1 test structure | Do not invent a framework before the runtime API is proven |
| Dependencies | `E:/Steam/.../UE4SS/UE4SS.log:2-22` | UE4SS v3.0.1 Beta loads mods from `Mods/NativeMods/UE4SS/Mods` | Game and loader versions are part of every result |

### Runtime Data Flow

```text
UE4SS startup / possession / map travel
  -> guarded bootstrap
  -> initial FindAllOf("PalMonsterCharacter") reconciliation
  -> NotifyOnNewObject for future monster actors
  -> UObject validity check
  -> HARD REJECT player class / player controller / player state / player-owned chain
  -> require proven APalMonsterCharacter
  -> require proven wild/unowned classification
  -> probe individual parameter and core fields
  -> add normalized diagnostic candidate
  -> choose at most one nearest valid candidate
  -> diagnostic top-anchor projection/draw
  -> aggregate metrics and field-path evidence
```

### State Owned by the Spike

- Resolved class handles for player, monster, player controller, and player state.
- An idempotent initialization/notification registration flag.
- A bounded candidate table keyed by stable object identity or full name plus validity checks.
- Aggregate rejection counts by reason; never remote player identity or position.
- Field-path availability and first successful accessor per semantic field.
- Timing counters for bootstrap scan, reconciliation, projection, and draw.
- Current selected diagnostic target, cleared on invalidation or travel.

### Contracts

- `candidate` means a currently valid object proven to be a non-player, wild `APalMonsterCharacter`.
- A failed type, ownership, or field read returns `nil`/`false` and cannot become a candidate through a fallback guess.
- Human player objects are rejected before location, name, ID, or individual-parameter reads.
- The renderer accepts only objects already admitted to the candidate table; it never performs independent discovery.
- The spike reads client-visible state only and never calls server mutation RPCs.

---

## Patterns to Mirror

### NAMING_CONVENTION

```lua
-- SOURCE: ../automatic_pickup/AutomaticPickup/Scripts/main.lua:1-4
local config = require("config")

local MOD_NAME = "AutomaticPickup"
local BOUND_RECORD_TTL_SECONDS = 30.0
```

Use local `snake_case` functions and tables, `UPPER_SNAKE_CASE` constants, and one `MOD_NAME` prefix. The new code substitutes `PalworldResourceESP` and spike-specific constants.

### ERROR_HANDLING

```lua
-- SOURCE: ../automatic_pickup/AutomaticPickup/Scripts/main.lua:36-66
local function unwrap(value)
    if value == nil then
        return nil
    end

    if type(value) == "userdata" and value.get ~= nil then
        local ok, result = pcall(function()
            return value:get()
        end)
        if ok then
            return result
        end
    end

    return value
end

local function is_valid(object)
    if object == nil then
        return false
    end

    if type(object) == "userdata" and object.IsValid ~= nil then
        local ok, result = pcall(function()
            return object:IsValid()
        end)
        return ok and result
    end

    return true
end
```

Every UObject method/property access in the spike must use this safety shape. A caught error is logged at debug level with a stable event code, then the object/field is treated as unavailable.

### LOGGING_PATTERN

```lua
-- SOURCE: ../automatic_pickup/AutomaticPickup/Scripts/main.lua:20-34
local function log(message)
    print(string.format("[%s] %s", MOD_NAME, message))
end

local function debug_log(message)
    if config.DEBUG then
        log(message)
    end
end
```

Extend messages with stable codes such as `BOOT_OK`, `CLASS_REJECT_PLAYER`, `PAL_ACCEPT`, `FIELD_PATH`, `METRIC`, and `DRAW_READY`. Player rejection logs contain only class/reason and aggregate counts.

### TYPE_AND_DATA_ACCESS

```lua
-- SOURCE: ../automatic_pickup/AutomaticPickup/Scripts/main.lua:262-286
local ok, result = pcall(function()
    return actor:IsA(monsterClass)
end)
if not ok then
    return false
end

return result == true
```

```lua
-- SOURCE: ../automatic_pickup/AutomaticPickup/Scripts/main.lua:354-389
if object.GetIndividualParameter ~= nil then
    local ok, result = pcall(function()
        return object:GetIndividualParameter()
    end)
    if ok and is_valid(result) then
        return result
    end
end
```

Resolve `/Script/Pal.PalPlayerCharacter` and `/Script/Pal.PalMonsterCharacter` once through `StaticFindObject`. Run player checks first, then monster checks, then ownership/wildness checks. Field probes follow method -> direct property -> character parameter component -> individual handle and record the successful path.

### HOOK_AND_NOTIFICATION_REGISTRATION

```lua
-- SOURCE: ../automatic_pickup/AutomaticPickup/Scripts/main.lua:1291-1321
local function register_hook_safely(path, preCallback, postCallback)
    local ok, preId, postId = pcall(function()
        return RegisterHook(path, preCallback, postCallback)
    end)

    if not ok then
        log(string.format("failed to register hook %s: %s", path, tostring(preId)))
        return nil, nil
    end

    return preId, postId
end
```

Wrap `RegisterHook` and `NotifyOnNewObject`, store one registration flag, and never bind them again after repeated possession/travel callbacks.

### REPOSITORY_PATTERN

N/A - this runtime spike has no repository or persistence layer. Use bounded local Lua tables only; Phase 2 will define the production entity registry.

### SERVICE_PATTERN

N/A - do not create a speculative service abstraction. Keep bootstrap, classification, probing, metrics, and diagnostic rendering as clearly separated local sections in `main.lua`.

### TEST_STRUCTURE

No automated test framework exists in this repository, and no standalone Lua interpreter is currently installed. Phase 1 uses a versioned runtime experiment matrix plus log assertions; do not introduce a test framework until pure logic boundaries exist in Phase 2.

---

## Files to Change During Implementation

| File | Action | Justification |
|---|---|---|
| `PalworldResourceESP/enabled.txt` | CREATE | UE4SS mod enable marker for direct development deployment |
| `PalworldResourceESP/Scripts/config.lua` | CREATE | Explicit diagnostic flags, scan intervals, display cap of one, and logging controls |
| `PalworldResourceESP/Scripts/main.lua` | CREATE | Spike entry point, safe bootstrap, discovery, hard exclusion, field probes, metrics, and diagnostic guide |
| `docs/testing/runtime-spike-matrix.md` | CREATE | Required single-player scenarios plus optional community host/dedicated-server scenarios with expected evidence |
| `docs/research/runtime-spike-report.md` | CREATE | Version fingerprint, successful field paths, replication boundaries, performance baseline, failures, and Phase 2 recommendation |

No production Widget asset, C++ module, package manifest, public README rewrite, or Workshop upload is part of this phase.

## NOT Building

- Player ESP, player diagnostics containing identity/location, or a setting that can enable either.
- The production entity registry, filter engine, presets, or configuration persistence.
- The final `Shift+E` panel, localization resources, or LogicMod Widget.
- More than one diagnostic target at a time.
- Full boxes, outlines, health, skeletons, weak points, off-screen arrows, or map markers.
- Resource adapters for chests, eggs, statues, ores, trees, or drops.
- Forced loading or access to server data not replicated to the client.
- Direct offsets, signatures, external memory reads, network hooks, server RPC mutation, or anti-cheat/server-rule bypass.
- C++ optimization or Game Pass support.
- Public release packaging or GitHub release automation.

---

## Step-by-Step Tasks

### Task 1: Capture the runtime compatibility fingerprint

- **ACTION**: Record the exact game, UE4SS, install-root, and installed-mod baseline before writing runtime assumptions.
- **IMPLEMENT**: Read Steam `buildid`, newest `UE4SS.log` version/SHA/root, Workshop UE4SS package metadata, and whether known overlay mods are enabled. Add these fields to the test matrix template and require them for every run.
- **MIRROR**: Use the active-root evidence in `UE4SS.log:2-22`; treat the legacy Win64 copy as read-only reference only.
- **IMPORTS**: None.
- **GOTCHA**: Workshop content can update before the installed copy is activated by a game launch; the newest runtime log is authoritative for the tested binary.
- **VALIDATE**: `runtime-spike-matrix.md` contains non-empty build ID, UE4SS version, Git SHA, and active mod root fields.

### Task 2: Scaffold the minimal UE4SS Lua mod

- **ACTION**: Create the three runtime files with a single entry point and conservative configuration.
- **IMPLEMENT**: `enabled.txt` is present; `config.lua` exposes only `ENABLED`, `DEBUG`, bootstrap/reconcile intervals, one-target display limit, and metric cadence; `main.lua` requires config and logs one load event.
- **MIRROR**: `../automatic_pickup/AutomaticPickup/Scripts/config.lua:1-29` and `main.lua:1-34`.
- **IMPORTS**: `local config = require("config")` only.
- **GOTCHA**: Do not add player visibility, scan-distance promises, resource toggles, or future UI options to the spike config.
- **VALIDATE**: Deploy to a preflight-checked `.../UE4SS/Mods/PalworldResourceESP` folder; one game process emits exactly one `BOOT_FILE_LOADED` line and no Lua load error.

### Task 3: Implement idempotent lifecycle bootstrap and safe UObject helpers

- **ACTION**: Establish reliable initialization without duplicate hooks or stale state.
- **IMPLEMENT**: Add `unwrap`, `is_valid`, safe method/property helpers, prefixed logs, stable event codes, an initialization flag, cache reset, and guarded possession/map-lifecycle hooks that remain usable in server clients. All engine-facing work that requires it runs on the game thread.
- **MIRROR**: Error and hook wrappers from `../automatic_pickup/AutomaticPickup/Scripts/main.lua:36-66,1291-1321`.
- **IMPORTS**: UE4SS globals only; no third-party Lua modules.
- **GOTCHA**: Possession and travel callbacks can repeat. Register notifications once per process; reset entity state without registering another callback.
- **VALIDATE**: Launch, load a save, return to title, reload, die, and respawn. Logs show repeated `SESSION_RESET` events but only one hook/notification registration event and no invalid-UObject exception.

### Task 4: Discover monster actors incrementally

- **ACTION**: Prove both existing-object bootstrap and future-object discovery.
- **IMPLEMENT**: Resolve required `/Script/Pal` classes; run one timed `FindAllOf("PalMonsterCharacter")` bootstrap after possession; register one `NotifyOnNewObject` for future monster instances; perform a low-frequency bounded reconciliation only for diagnostics and stale-object cleanup.
- **MIRROR**: Class caching and `IsA` handling from `../automatic_pickup/AutomaticPickup/Scripts/main.lua:232-287`.
- **IMPORTS**: UE4SS `StaticFindObject`, `FindAllOf`, `NotifyOnNewObject`, `ExecuteWithDelay` or `LoopAsync` as available.
- **GOTCHA**: `NotifyOnNewObject` does not cover objects created before registration, while `FindAllOf` is too expensive for each frame. Record nil/empty results rather than treating them as a crash.
- **VALIDATE**: In a save with loaded Pals, bootstrap reports at least one raw monster object; moving into a new spawn area produces notification/reconciliation evidence without another per-frame full scan.

### Task 5: Enforce acquisition-layer player and ownership exclusion

- **ACTION**: Implement the phase's core safety invariant before any candidate fields or positions are read.
- **IMPLEMENT**: Apply checks in this exact order: valid UObject -> reject `APalPlayerCharacter` -> reject player controller/player state representations -> require `APalMonsterCharacter` -> inspect owner/controller/trainer/individual relationships -> accept only when the actor is proven wild/unowned. Unknown, ambiguous, NPC-owned, base-owned, party, summoned, mounted, or player-associated Pals remain outside the diagnostic candidate table until positively classified.
- **MIRROR**: `IsA` and owner-candidate access patterns from `../automatic_pickup/AutomaticPickup/Scripts/main.lua:262-287,423-455`.
- **IMPORTS**: Resolved player/monster/controller/state classes and local safe access helpers.
- **GOTCHA**: Never read or log a human player's name, platform ID, player UID, location, distance, individual parameters, or label. Log only aggregate rejection counts and non-identifying class/reason codes.
- **VALIDATE**: Single-player local character, death/respawn, riding, party Pal, and base Pal scenarios produce zero human candidate admissions. Host, remote-client, and dedicated-server rows remain clearly marked `community-pending`; any future server result must still require `candidate_player_count = 0`.

### Task 6: Probe and normalize core Pal fields

- **ACTION**: Determine which client-visible access paths work on the current build.
- **IMPLEMENT**: Starting from the accepted monster actor, probe character parameter component, individual parameter, and individual ID using the existing ordered access pattern. For each semantic field, record `available`, `unavailable`, or `ambiguous`, the successful method/property path, and value type for species/internal Pal ID, display-name source, level, gender, passive skills, HP/attack/defense IVs, Lucky/rare, Alpha/Boss, elements, and local capture-count/completion source. Keep the same diagnostics available for later community multiplayer runs.
- **MIRROR**: `get_character_parameter_component`, `try_get_individual_parameter`, and `get_individual_id` from `../automatic_pickup/AutomaticPickup/Scripts/main.lua:330-420`.
- **IMPORTS**: No PalSchema runtime dependency. PalSchema/type dumps may be read as documentation only.
- **GOTCHA**: Do not coerce missing values to zero/false, assume capture completion is always five, or merge Lucky and Alpha into one flag. Missing and ambiguous data must remain explicit.
- **VALIDATE**: The report contains one row per required semantic field with exact access path and single-player evidence, or a documented blocker with the attempted paths and error type. Multiplayer evidence is optional and community-supplied.

### Task 7: Draw one temporary top-anchored diagnostic guide

- **ACTION**: Prove the world-to-screen and draw path without introducing the production Widget toolchain.
- **IMPLEMENT**: Select at most one nearest valid candidate; during a safe HUD/Canvas draw callback, project its world location and draw a thin line from a top-center anchor to the projected target. Add a minimal diagnostic label only if a stable font/text path is already available. Renderer input is the candidate table only.
- **MIRROR**: Use the same safe hook wrapper, UObject validity checks, and one-time registration pattern; no new rendering abstraction.
- **IMPORTS**: UE4SS hook API plus current engine HUD/Canvas projection and line-draw functions discovered by the fixed probe order `HUD Canvas -> player-controller projection -> mark blocked`.
- **GOTCHA**: Do not use the screen center as the anchor. Do not retain Canvas/HUD objects across frames. If neither projection path is valid, record the exact blocker and make the LogicMod toolchain a Phase 1 completion dependency rather than switching architecture silently.
- **VALIDATE**: With one accepted wild Pal, the guide originates near the screen top, terminates at the Pal's projected position, disappears when the UObject becomes invalid/unloaded, and never appears for a human player.

### Task 8: Execute the single-player matrix and publish community server cases

- **ACTION**: Validate discovery, exclusion, data availability, and lifecycle in single-player, then make server cases reproducible for community contributors.
- **IMPLEMENT**: Run controlled single-player cases covering local spawn, approaching/leaving a spawn area, multiple Pals, mount/dismount, party/base Pals, Pal death/despawn, player death/respawn, fast travel, dungeon/interior transition, return to title, and reconnect. Disable other overlays for one clean control run, then repeat one compatibility run with normal mods enabled. Document host/co-op and dedicated-server client rows as optional `community-pending` cases rather than executing them locally.
- **MIRROR**: Use the PRD success signal and risks at `.claude/PRPs/prds/palworld-resource-pal-esp.prd.md:221-234,253-257`.
- **IMPORTS**: None.
- **GOTCHA**: Multiplayer replication range is an observation, not a promise. Do not attempt to force loading or infer unsent attributes.
- **VALIDATE**: Every executed single-player row contains environment fingerprint, expected result, actual result, relevant log code, pass/fail, and notes. Community server rows contain prerequisites, exact steps, expected log evidence, and an untested status.

### Task 9: Establish baselines and publish the Phase 1 gate report

- **ACTION**: Convert runtime evidence into a clear go/no-go decision for Phase 2.
- **IMPLEMENT**: Record bootstrap scan duration/count, notification count, reconciliation cost, candidate count, projection/draw timing, invalidation events, discovery latency samples, and a 15-minute single-player stability run. Summarize field availability, renderer feasibility, known compatibility issues, and recommended initial refresh/display budgets without treating the small sample as final production limits. Mark server replication boundaries and stability as unknown until community evidence exists.
- **MIRROR**: PRD metric intent at `.claude/PRPs/prds/palworld-resource-pal-esp.prd.md:54-62` and ADR follow-ups at `docs/adr/0001-use-ue4ss-hybrid-client-mod-architecture.md:143-151`.
- **IMPORTS**: Lua `os.clock`/`os.time` for local timing and PowerShell/`rg` for log extraction.
- **GOTCHA**: Report measured values with build/version context. Do not claim 50,000 m visibility; the client replication/loading boundary remains authoritative.
- **VALIDATE**: `runtime-spike-report.md` ends with one of: `GO - Phase 2`, `CONDITIONAL GO` with explicit follow-ups, or `BLOCKED` with reproducible evidence. It also states whether a LogicMod development environment must be installed before Phase 4/5.

---

## Testing Strategy

### Automated/Log-Level Tests

There is no current unit-test harness. Phase 1 uses deterministic log assertions that can later become fixtures.

| Test | Input | Expected Output | Edge Case? |
|---|---|---|---|
| Idempotent bootstrap | Repeated possession/travel callbacks | One registration set, multiple safe session resets | Yes |
| Invalid UObject | Despawned/dead/travel-stale Pal | Candidate removed; no property call after invalidation | Yes |
| Player hard reject | Local or remote `APalPlayerCharacter` | Aggregate rejection increments; no candidate/location/identity log | Critical |
| Unknown actor | UObject cannot be proven monster/non-player | Rejected with `UNKNOWN_TYPE` | Yes |
| Owned Pal | Party/base/mounted/summoned Pal | Not admitted as a wild candidate | Critical |
| Existing Pal | Pal exists before notification registration | Found by bootstrap scan | No |
| Newly created Pal | Pal spawns after registration | Found by notification/reconciliation | No |
| Missing field | Accessor/property absent or throws | `unavailable` with attempted path; no default value | Yes |
| Renderer invalidation | Selected target unloads mid-frame | Guide disappears safely | Yes |
| Duplicate overlay environment | Existing UI mods enabled | No player admission; conflicts documented | Yes |

### Runtime Test Environments

| Environment | Required | Purpose |
|---|---|---|
| Steam single-player | Yes | Baseline discovery, fields, guide, travel, stability |
| Steam host/co-op | Community pending | Host/client ownership and remote-player exclusion |
| Steam dedicated-server client | Community pending | Client replication boundary and remote-player exclusion |
| Clean overlay control | Yes | Separate project behavior from Paldar/Map Collectables/Pal Analyzer |
| Normal mod set compatibility | Yes | Detect hook/Widget conflicts in the maintainer's real environment |

### Edge Cases Checklist

- [ ] No loaded monsters at bootstrap
- [ ] Nil result from `FindAllOf`
- [ ] Notification fires before session bootstrap completes
- [ ] Duplicate possession and travel callbacks
- [ ] UObject invalidates between classification and field read
- [ ] UObject invalidates between selection and draw
- [ ] Local player; host and remote dedicated-server players remain community-pending
- [ ] Player death, respawn, reconnect, and return to title
- [ ] Mounted player and mounted Pal
- [ ] Party, base, summoned, caged, NPC-owned, and wild Pal distinctions
- [ ] Alpha/Boss and Lucky are independently represented
- [ ] Community case: missing or delayed multiplayer individual parameters
- [ ] More raw actors than the diagnostic cap
- [ ] Other installed overlay mods enabled
- [ ] Game/UE4SS version changed since the plan was written

---

## Validation Commands

Run commands from `E:/AAA_qian/ji_ji_tui_jin/palworld_mod/esp` unless noted.

### Repository Structure

```powershell
Get-ChildItem -Recurse PalworldResourceESP,docs/testing,docs/research
```

EXPECT: The five planned files exist at the documented paths; no unplanned runtime modules or packaged binaries are present.

### Static Content Checks

```powershell
rg -n "PalworldResourceESP|BOOT_|CLASS_REJECT_PLAYER|PAL_ACCEPT|FIELD_PATH|METRIC|DRAW_READY" PalworldResourceESP docs
rg -n "Request.*Server|ServerInternal|Set.*Rate|Teleport|AutoAim|PlayerESP|SHOW_PLAYERS" PalworldResourceESP
git diff --check
```

EXPECT: Required diagnostic codes exist; the prohibited mutation/Player-ESP search returns no implementation matches; `git diff --check` reports no whitespace errors. References inside documentation are allowed and must be reviewed manually.

### Active Runtime Root Preflight

```powershell
rg -n "UE4SS -|Git SHA|Loading mods from:|root directory:" "E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS/UE4SS.log"
rg -n '"buildid"' "E:/Steam/steamapps/appmanifest_1623730.acf"
Test-Path "E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS/Mods/PalworldResourceESP"
```

EXPECT: Version/root/build evidence is captured before deployment. If the target exists unexpectedly, stop and inspect it; do not overwrite it.

### Reversible Development Deployment

Before copying, follow the repository defensive preflight protocol. Copy the repository mod folder into the active UE4SS `Mods` root only when the destination does not exist. On rollback, rename the deployed folder to `__DEPRECATED_YYYYMMDD_PalworldResourceESP`; never delete it.

### Runtime Log Validation

```powershell
rg -n "\[PalworldResourceESP\].*(BOOT_|CLASS_|PAL_|FIELD_|METRIC|DRAW_|ERROR)" "E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS/UE4SS.log"
rg -n "\[PalworldResourceESP\].*(player name|player uid|platform id|location)" "E:/Steam/steamapps/common/Palworld/Mods/NativeMods/UE4SS/UE4SS.log"
```

EXPECT: The first command provides evidence for the matrix. The second command returns no identifying player diagnostics.

### Manual Validation

- [ ] Launch with the exact build/version fingerprint recorded.
- [ ] Confirm one load and one registration sequence.
- [ ] Confirm one known wild Pal is accepted and guided from the screen top.
- [ ] Confirm local and remote human players never enter candidates or receive guides.
- [ ] Confirm party/base/mounted Pals are not misclassified as wild.
- [ ] Confirm each required Pal field has a path or explicit unavailable/ambiguous result.
- [ ] Confirm fast travel, death/respawn, return to title, and reconnect do not crash.
- [ ] Confirm no full `FindAllOf` occurs per frame.
- [ ] Complete the 15-minute single-player stability sample; leave server samples explicitly community-pending.
- [ ] Complete the report and Phase 2 gate decision.

---

## Acceptance Criteria

- [ ] All nine tasks are complete and limited to the five planned repository files.
- [ ] The mod loads from the active Workshop UE4SS root on the recorded Steam build.
- [ ] Existing and newly created loaded wild Pals can be discovered without per-frame full scans.
- [ ] Human player candidate admission is exactly zero in every executed single-player scenario; server behavior is not guaranteed until community validation.
- [ ] Human player identity, location, and individual data are never stored or logged.
- [ ] Unknown/ambiguous actors and player-owned/associated Pals fail closed.
- [ ] One proven wild Pal can receive a minimal top-anchored diagnostic guide, or the exact renderer blocker and required LogicMod toolchain are documented as the only remaining Phase 1 blocker.
- [ ] Each required semantic Pal field has a verified access path or a reproducible unavailable/ambiguous result.
- [ ] Multiplayer replication/loading boundaries are measured and documented without forced loading.
- [ ] Bootstrap, reconciliation, projection/draw, discovery latency, and stability baselines are recorded with version context.
- [ ] No server mutation RPC, game-data modification, direct memory reading, or player ESP capability is introduced.
- [ ] `runtime-spike-report.md` provides a clear Phase 2 gate decision.

## Completion Checklist

- [ ] Code follows discovered local naming, logging, `pcall`, and UObject-validity patterns.
- [ ] Hook and notification registration is idempotent.
- [ ] Player exclusion happens before candidate insertion and before sensitive field/location reads.
- [ ] Logs use stable event codes and contain no player identifiers.
- [ ] Config contains only Phase 1 diagnostic controls and no player switch.
- [ ] Test matrix records build and UE4SS versions for every run.
- [ ] No hardcoded guess replaces missing runtime data.
- [ ] No per-frame `FindAllOf` or unbounded object table exists.
- [ ] Diagnostic renderer is clearly marked spike-only.
- [ ] Documentation distinguishes fact, measured observation, ambiguity, and recommendation.
- [ ] No unrelated files, installed mods, or reference project files are modified.
- [ ] No additional codebase search or product decision is needed during implementation.

## Risks

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Player/owned-Pal classification path differs across multiplayer modes | High | Critical | Player-first type checks, ownership-chain probes, unknown fail-closed, publish community server matrix and do not claim verified compatibility |
| Current game update changes class/function paths | High | High | Fingerprint every run, resolve `/Script/` classes at startup, log exact failed path |
| `NotifyOnNewObject` misses existing actors or registers repeatedly | Medium | High | One bootstrap scan plus one guarded notification registration |
| UObject invalidates during asynchronous work | High | High | Validate before every access, never retain HUD/Canvas, clear on travel/despawn |
| Required fields are server-delayed or unavailable | High | Medium | Report per-environment availability; no guessed values; defer unsupported filters |
| Lua diagnostic rendering path is unavailable | Medium | Medium | Fixed Canvas/controller projection probes; document LogicMod toolchain blocker without changing ADR |
| Other overlay mods create misleading evidence | Medium | Medium | Clean control run plus normal compatibility run |
| Initial scan or draw causes frame hitching | Medium | High | Time every phase, display one target, no per-frame scan, bounded reconciliation |
| Active UE4SS installation changes after Workshop update | Medium | High | Trust newest runtime log, not stale package metadata or legacy directories |

## Notes

- The current machine has no standalone `lua`, `luac`, `stylua`, CMake, MSBuild, or Unreal Editor in PATH. Phase 1 does not need to install the full Widget toolchain unless the diagnostic render path is blocked.
- Network research can use the local proxy at `127.0.0.1:7897` when direct access is unreliable.
- Any environment/tool installation during implementation is a separate state change and must follow the defensive preflight and rollback protocol.
- The existing `docs/adr/0001-use-ue4ss-hybrid-client-mod-architecture.md` remains `Proposed`; this plan follows it and does not silently accept, supersede, or modify it.
- The reference `automatic_pickup` repository has user changes and is strictly read-only.
