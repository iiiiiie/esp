# Runtime Spike Research Report

## Status

`CONDITIONAL GO - stable Lua acquisition core proven; Blueprint bridge required for rendering and complex fields`

Server compatibility is intentionally unverified by the maintainer. Community multiplayer evidence may be added later, but it is not a Phase 1 completion gate.

## Objective

Prove that the Steam PC client can use UE4SS Lua to discover currently loaded wild Pals, reject human players and player-associated Pals before candidate insertion, read useful individual fields, and draw one top-anchored diagnostic guide without modifying game or server state.

## Compatibility Fingerprint

| Field | Plan-Time Value | Runtime-Test Value |
|---|---|---|
| Steam build ID | `24088745` | `24088745` |
| Game executable timestamp | 2026-07-11 | Pending |
| UE4SS version | v3.0.1 Beta | v3.0.1 Beta #0 |
| UE4SS Git SHA | `c2ac2464` | `c2ac2464` observed 2026-07-15 |
| UE4SS Workshop package | `experimental-palworld-5` | `experimental-palworld-5` |
| Active UE4SS root | `E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss` | Confirmed by current-process log |
| Project commit | Pending | Uncommitted `codex/runtime-spike` working tree |

The active root must be taken from the newest current-process `UE4SS.log`. On 2026-07-15, an alternate `Mods/NativeMods/UE4SS` installation existed with a stale July 11 log, while the running game loaded from `Pal/Binaries/Win64/ue4ss`. The first test therefore did not load this Mod and produced no guide.

## Implementation Facts

- Entry point: `PalworldResourceESP/Scripts/main.lua`
- Configuration: `PalworldResourceESP/Scripts/config.lua`
- Discovery: one bootstrap/low-frequency `FindAllOf("PalMonsterCharacter")` reconciliation plus one guarded `NotifyOnNewObject`
- Scheduling: `LoopInGameThreadWithDelay` when available, with `LoopAsync` as compatibility fallback
- Player boundary: player/player-controller/player-state type checks occur before monster acceptance and before candidate insertion
- Wildness boundary: accepted monsters require a zero `SaveParameter.OwnerPlayerUId` and no `Trainer`/`NPCSpawnedOtomoTrainer`
- Unknown or unreadable ownership state fails closed
- Rendering: at most one candidate, projected through the local player controller and drawn from a top-center Canvas anchor
- Mutation: no server RPC, setter, teleport, spawn, capture, probability, save, or network modification path exists

## Runtime Event Contract

| Event | Meaning |
|---|---|
| `BOOT_FILE_LOADED` | Lua entry point loaded once for the process |
| `BOOT_OK` | Delayed session bootstrap completed |
| `SESSION_RESET` | Candidate state reset after a lifecycle signal |
| `CLASS_REJECT_PLAYER` | A player representation was rejected before candidate insertion |
| `CLASS_REJECT_OWNED` | A player/trainer-associated Pal was rejected |
| `CLASS_REJECT_UNKNOWN` | Type or ownership could not be proven; object failed closed |
| `PLAYER_AUDIT` | Aggregate player-boundary result; must report `candidate_player_count=0` |
| `PAL_ACCEPT` | A monster passed all wild/non-player checks |
| `FIELD_PATH` | First successful accessor or explicit unavailable result for a semantic field |
| `SCAN_DONE` | Bootstrap/reconciliation object counts and duration |
| `SCAN_SKIPPED` | Scan was suppressed because no local gameplay player exists or a map transition is active |
| `CANDIDATES_CLEARED` | Candidate snapshot was dropped without dereferencing retained actors |
| `WORLD_TRANSITION` | UE4SS LoadMap pre/post lifecycle boundary |
| `METRIC` | Aggregate discovery, rejection, invalidation, notification, scan, and draw metrics |
| `DRAW_READY` | Top-center diagnostic line successfully rendered |
| `ERROR_PLAYER_BOUNDARY` | Critical safety failure; Phase 1 cannot pass |

## Field Probe Matrix

Runtime values must be copied from `FIELD_PATH` log lines. Do not infer unavailable fields.

| Semantic Field | Probe Order | Single-Player Result | Path/Type | Notes |
|---|---|---|---|---|
| Species | `UPalIndividualCharacterParameter.GetCharacterID()` | Available | `parameter.method:GetCharacterID` returning FName userdata | Eighth run completed without direct ScriptStruct access |
| Individual ID | Confirmed parameter method only | Available, structured value | `parameter.method:GetPalId` | Returned a table-like value; diagnostic identity only for Pals |
| Level | `UPalIndividualCharacterParameter.GetLevel()` | Available | `parameter.method:GetLevel` | Eighth run value was `3` |
| Gender | Blueprint/LogicMod bridge required | Runtime unsafe in Lua | Direct property and `parameter.method:GetGenderType` both rejected | Direct enum access caused two crashes; UFUNCTION enum return caused the tenth-run crash |
| Passive skills | Blueprint/LogicMod bridge required | Runtime unsafe to probe further in Lua | Pending bridge | Array marshalling remains unproven; no further crash-prone Lua experiment |
| HP IV | Blueprint/LogicMod bridge over confirmed `Talent_HP` data | Deferred from Lua | Pending bridge | No rank-method substitution |
| Attack IV | Blueprint/LogicMod bridge over confirmed `Talent_Shot` data | Deferred from Lua | Pending bridge | |
| Melee IV | No adapter until the game-build field is confirmed | Unavailable by design | Pending bridge/schema confirmation | Supplemental research field deferred after the native probe crash |
| Defense IV | Blueprint/LogicMod bridge over confirmed `Talent_Defense` data | Deferred from Lua | Pending bridge | |
| Lucky/rare | `IsRarePal`, related properties | Pending | Pending | Kept separate from Alpha/Boss |
| Alpha/Boss | Actor/parameter Alpha/Boss methods and properties | Pending | Pending | Missing is allowed and must be explicit |
| Elements | Actor/parameter element methods and properties | Pending | Pending | May require a later species database adapter |
| Capture count | No individual accessor assumed | Unavailable by design | N/A | Requires a separate local player-save collection adapter |

## Player and Ownership Boundary

### Required Invariants

- `APalPlayerCharacter`, player controller, and player state representations never enter `state.candidates`.
- No player name, platform ID, player UID, or player coordinate is logged.
- A monster with a non-zero `OwnerPlayerUId` is not a wild candidate.
- A monster with `Trainer` or `NPCSpawnedOtomoTrainer` is not a wild candidate.
- Missing owner/trainer evidence is rejected as unknown rather than accepted.
- Reconciliation reclassifies existing candidates so a captured Pal cannot remain marked as wild.

### Single-Player Evidence

The second run (2026-07-15 00:43:38-00:44:59) loaded the Mod successfully and audited one local player with `candidate_player_count=0`. The client discovered 13-26 raw `PalMonsterCharacter` objects and received 25 construction notifications. No candidate was admitted because the first implementation attempted to read `OwnerPlayerUId` directly from `UPalIndividualCharacterParameter`; current type evidence places that field in `FPalIndividualCharacterSaveParameter`.

The third run (2026-07-15 00:54:17-00:56:07) used the nested `SaveParameter` path, again audited one local player with `candidate_player_count=0`, discovered 10-27 raw monsters, and received 33 construction notifications. Candidate classification then raised `ERROR_CANDIDATE` because `unwrap` read `value.get` outside `pcall`; UE4SS mapped that member access to a nonexistent `Get` property on `FPalIndividualCharacterSaveParameter`. Getter and `IsValid` discovery are now guarded. `luaparse`, a Fengari userdata that throws on missing-property access, UTF-8, whitespace, and prohibited-capability checks all pass. Runtime confirmation requires a fourth run.

The fourth run (2026-07-15 01:22:20-01:23:04) entered the save and retained `candidate_player_count=0`. It successfully logged species (`save.property:CharacterID`), individual ID (`parameter.method:GetPalId`), and level (`save.property:Level`) for a wild Pal, proving the nested save path works. Immediately afterward, the GameThread crashed with `EXCEPTION_ACCESS_VIOLATION reading address 0x1`; the crash stack is dominated by UE4SS frames and the last project event is the level field. This demonstrates that Lua `pcall` cannot contain every native failure caused by speculative Unreal member access or complex userdata conversion. Recovery now uses an exact field allowlist, defers unconfirmed Lucky/Alpha/element paths, avoids calling `ToString` on userdata, and logs `FIELD_PROBE_BEGIN` before each remaining probe. A fifth runtime run is required.

The fifth run used `DRAW_ENABLED=false` to remove the unproven Canvas path, but still exercised the reduced field allowlist. No green guide was expected during that run. Rendering remains disabled until candidate acceptance and stability are confirmed.

The fifth run (2026-07-15 01:36:55-01:37:33) reproduced the same UE4SS access-violation hash. `FIELD_PROBE_BEGIN` showed the exact sequence: species, individual ID, and level completed; the process terminated immediately after `FIELD_PROBE_BEGIN field=gender`. Direct `FPalIndividualCharacterSaveParameter.Gender` access is therefore runtime-unsafe on the tested UE4SS build even though the field exists in SDK headers. Public Modding Kit headers expose `UPalIndividualCharacterParameter.GetGenderType()` and `UPalIndividualCharacterSaveParameterUtility.GetSaveParameterValue_Gender()` as possible adapters, but neither will be exercised until the acquisition path is stable. The sixth run disables all display field probes and retains only the actor in candidate state.

The sixth run (2026-07-15 01:47:38-01:49:13) successfully entered the save, kept `candidate_player_count=0`, emitted `PAL_ACCEPT`, found 11-19 raw monsters, rejected player-owned and trainer-associated Pals, and recorded 16 construction notifications. The acquisition core therefore passed for the first time. A separate cache bug was exposed: UE4SS returned different Lua wrapper identities for the same actors on later `FindAllOf` calls, so the candidate table grew to 58 while only 17 raw monsters were present. On return to title, the periodic cleanup iterated these stale wrappers; after one `PAL_REMOVE reason=invalidated`, the GameThread crashed with the same UE4SS crash hash. Recovery now registers `RegisterLoadMapPreHook`/`RegisterLoadMapPostHook`, clears snapshots without actor dereference, skips all scans when no local player exists, gates construction notifications by gameplay state, and rebuilds the candidate snapshot on each low-frequency reconcile.

The seventh run (2026-07-15 01:58:08-02:00:05) passed the lifecycle recovery. Reconciliation found 8-12 raw monsters and retained 3-6 current wild candidates rather than accumulating wrapper identities. `candidate_player_count=0` remained intact, owned and trainer-associated Pals were rejected, and 16 construction notifications were observed. Returning to title emitted `CANDIDATES_CLEARED reason=load_map_pre previous=4`, followed by `WORLD_TRANSITION` pre/post and `SCAN_SKIPPED` with zero candidates. The title wait and normal process exit produced no new crash report.

The eighth run (2026-07-15 02:05:33-02:06:50) validated the first safe field profile. `GetCharacterID`, `GetPalId`, and `GetLevel` succeeded through `UPalIndividualCharacterParameter`; level was `3`, species returned FName userdata without unsafe text conversion, and the individual ID returned a structured table. Reconciliation retained 5-8 wild candidates from 10-16 raw monsters, `candidate_player_count=0` remained intact, return-to-title cleanup passed, and no crash report was added. The next run enables only a guarded HUD callback probe and still performs no Canvas or projection access.

The ninth run (2026-07-15 02:14:50-02:16:01) kept the safe field and lifecycle paths stable but recorded `draw_callbacks=0` and no `DRAW_CALLBACK_READY` while accepted candidates were present. The `/Script/Engine.HUD:ReceiveDrawHUD` Hook registered successfully but is not invoked by the current Palworld HUD implementation. Per ADR-0001, the Lua Canvas experiment stops at this reproducible blocker; production and further diagnostic rendering require the planned LogicMod/Blueprint Widget path rather than speculative native hooks.

The tenth run (2026-07-15 02:18:54-02:19:32) isolated `UPalIndividualCharacterParameter.GetGenderType()`. Species, individual ID, and level completed, then the process terminated immediately after `FIELD_PROBE_BEGIN field=gender` with the same UE4SS access-violation hash as the direct-property crashes. The enum return value itself cannot be safely marshalled through the tested Lua bridge. Gender is restored to unavailable in the stable Lua profile. Passive-skill arrays and SaveParameterUtility calls are also deferred to the Blueprint bridge rather than risking further native crash experiments on the maintainer's game process.

Post-crash recovery restored the previously stable profile: only `GetCharacterID()`, `GetPalId()`, and `GetLevel()` remain enabled; gender and all complex enum/array/struct probes have empty source lists. Repository and active Junction deployment hashes match. This recovery was applied after the tenth game process had already terminated, so the reported crash does not represent the restored deployment profile.

The eleventh run (2026-07-15 02:27:47-02:28:51) validated that recovery profile. It entered the save, reported `candidate_player_count=0`, read species, individual ID, and level, and reported every disabled complex field as unavailable without invoking an accessor. Reconciliation found 9-15 raw monsters and retained 4-7 current wild candidates while rejecting player-owned and trainer-associated Pals. Returning to title cleared seven candidates before map teardown, later scans remained suppressed without a local player, and the game exited normally with no new crash report. This is a short recovery smoke test, not the pending 15-minute stability sample.

### Server Evidence

`UNTESTED - community pending`

The code retains server-compatible lifecycle hooks, but the maintainer does not guarantee server availability or replication behavior. Any future server claim requires attached sanitized logs and `candidate_player_count=0`.

## Discovery and Lifecycle Results

| Check | Result | Evidence |
|---|---|---|
| Existing loaded Pals found at bootstrap | Pass via reconciliation | Sixth run emitted `PAL_ACCEPT` and found 11-19 raw monsters after world readiness |
| Newly constructed Pals observed | Pass | 16 `NotifyOnNewObject` callbacks recorded in the sixth run |
| No per-frame full scan | Implemented | Reconcile interval in config; runtime timing pending |
| Candidate invalidation/despawn cleanup | Pass for snapshot lifecycle | Seventh run rebuilt current candidates each scan and cleared the snapshot before title-map load without actor dereference |
| Capture/ownership transition cleanup | Pending | |
| Fast travel/interior transition | Pending | |
| Death/respawn | Pending | |
| Return to title/reload | Pass | Seventh and eleventh runs returned to title, waited, and exited without a crash |

## Diagnostic Renderer Results

| Check | Result | Evidence |
|---|---|---|
| HUD draw hook registration | Hook registered, callback unavailable | Ninth run recorded zero callbacks during gameplay |
| World-to-screen projection | Blocked in Lua path | HUD callback never executes; defer to LogicMod/Blueprint Widget |
| Top-center anchor | Implemented, runtime pending | `TOP_ANCHOR_Y`, no center anchor option |
| One-target cap | Implemented | `MAX_DISPLAY_TARGETS = 1` |
| Invalid target disappears safely | Pending | Fourth run crashed before candidate insertion and rendering |
| Player guide impossible through renderer input | Pass for local audit | Renderer reads candidate table only; local player candidate count remained zero |

If `ReceiveDrawHUD` or the Canvas struct bridge is unavailable on the current build, record `DRAW_HOOK_BLOCKED`/`DRAW_FAILED` and treat LogicMod/Blueprint tooling as the remaining rendering prerequisite. Do not silently change the architecture.

## Performance Baseline

| Metric | Result | Context |
|---|---|---|
| Bootstrap raw actor count | 0 | Bootstrap fired before Pal actors were available |
| Bootstrap scan time | 32 ms in world bootstrap sample | Cumulative value increased from earlier title scans |
| Reconcile interval | 5000 ms configured | Diagnostic value, not production default |
| Reconcile scan time | 59-98 ms in latest stable run | Eleventh run raw actor count 9-15; earlier runs observed 56-140 ms |
| Notification count | 17 in latest stable run | Eleventh run; earlier second run observed 25 |
| Candidate count | 4-7 current wild candidates with 9-15 raw monsters | Eleventh run confirmed the rebuilt snapshot remains bounded by the current raw set |
| Draw calls/time | 0 callbacks and 0 calls | Ninth run confirmed the registered `ReceiveDrawHUD` Hook is not invoked |
| Discovery latency samples | Pending | |
| 15-minute single-player stability | Pending | |

## Deviations From Plan

- Multiplayer runtime tests were changed from required to community-pending at the maintainer's request. Server compatibility remains an unverified target.
- The first deployment followed Workshop documentation and a stale log under `Mods/NativeMods/UE4SS`; current-process evidence showed experimental-palworld-5 loading from `Pal/Binaries/Win64/ue4ss`, so deployment and documentation were corrected.
- The first ownership probe treated `OwnerPlayerUId`, `IsPlayer`, IVs, and passive skills as direct individual-parameter fields. Runtime behavior and SDK headers confirmed they belong to the nested `SaveParameter` struct; classification and field probes were corrected without weakening fail-closed behavior.
- The first nested-struct implementation assumed that probing `userdata.get` was harmless. UE4SS ScriptStruct access treats unknown members as Unreal property lookups and throws, so getter and validity-method discovery now occur inside `pcall`.
- The fourth run proved that `pcall` is not a native crash boundary for all UE4SS member access. Phase 1 field discovery now uses only confirmed exact paths; unconfirmed Lucky/Alpha/element access is reported unavailable rather than guessed at runtime.
- The sixth run showed that UObject Lua wrapper identity is not stable across `FindAllOf` calls. Candidate state must be a current-scan snapshot or use a proven stable native identifier; raw wrapper keys cannot be accumulated across scans.
- The spike uses current UE4SS delayed actions when available instead of preferring deprecated `LoopAsync`; a compatibility fallback remains.
- Capture count is documented as requiring a separate player-save adapter rather than probing sensitive player objects during the acquisition spike.

## Known Limitations

- Server compatibility and replication range are unknown.
- The diagnostic Canvas renderer is not the production Widget renderer.
- Alpha/Boss and element accessors may require a later database/spawn adapter.
- No claim is made that a configured scan distance can load objects the client has not loaded or received.
- Runtime syntax and static checks cannot prove game-version field compatibility; the single-player matrix is required.
- Some invalid or mismatched UE4SS property/method conversions can terminate the GameThread rather than raising a catchable Lua error; new accessors must be introduced one confirmed path at a time.
- SDK presence does not imply that UE4SS Lua can safely marshal a property type. Enum and array fields require a proven UFUNCTION/Blueprint adapter or must remain unavailable.
- A successfully registered UE4SS Hook does not prove the game executes that UFUNCTION. Palworld's current HUD does not call `ReceiveDrawHUD`; renderer work must use the ADR-selected Widget path.
- A BlueprintPure UFUNCTION declaration does not guarantee that its return type is safe through UE4SS Lua. `EPalGenderType` crashes in both property and function-return marshalling on the tested build.

## Phase 2 Gate

Decision: `CONDITIONAL GO - BEGIN LOGICMOD/BLUEPRINT BRIDGE SPIKE`

Phase 2 may begin because the acquisition boundary, current-snapshot lifecycle, safe core fields, and return-to-title recovery are proven; human candidate admission remained zero; and both remaining technical blockers are reproducible UE4SS Lua limitations with the ADR-selected bridge as their next step. Phase 1 is not declared fully complete: capture/mount/travel/death scenarios, the 15-minute stability sample, and broader compatibility evidence remain follow-up validation. Multiplayer rows remain community-pending and must not be presented as verified.
