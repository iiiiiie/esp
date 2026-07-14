# Runtime Spike Test Matrix

## Status

- Maintainer-required environment: Steam PC single-player
- Server environments: community pending, not a Phase 1 completion gate
- Human player invariant: any future server run must still report `candidate_player_count=0`
- Last updated: 2026-07-15

## Run Fingerprint

Complete this block for every test session before recording results.

| Field | Value |
|---|---|
| Date/time | 2026-07-15 02:27:47-02:28:51 (eleventh run; restored stable profile passed) |
| Steam build ID | `24088745` at plan time; re-read before launch |
| UE4SS version | v3.0.1 Beta #0 |
| UE4SS Git SHA | `c2ac2464` observed on 2026-07-15; re-read newest log |
| Active UE4SS root | `E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss` |
| Project commit | Uncommitted `codex/runtime-spike` working tree |
| Save/world | Existing maintainer single-player save |
| Other overlay mods enabled | Maintainer's normal Mod set; UE4SS log includes BPModLoader-managed Mods |
| Relevant log time range | 2026-07-15 02:27:47-02:28:51 |

Fingerprint commands:

```powershell
rg -n '"buildid"' "E:/Steam/steamapps/appmanifest_1623730.acf"
rg -n "UE4SS -|Git SHA|Loading mods from:|root directory:" "E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.log"
git rev-parse --short HEAD
```

## Evidence Rules

- Record the exact `[PalworldResourceESP]` event code and log line for each result.
- Do not record player names, platform IDs, player UIDs, or player coordinates.
- A missing field is `unavailable`; do not replace it with `0`, `false`, or an inferred value.
- A server result must state the server type and Mod/anti-cheat environment, but server testing is optional for the maintainer.
- Any `ERROR_PLAYER_BOUNDARY classification=wild` line is a critical failure.

## Required Single-Player Cases

| ID | Scenario | Steps | Expected Evidence | Status | Actual/Notes |
|---|---|---|---|---|---|
| SP-01 | Initial load | Launch game, enter an existing single-player save, wait 10 seconds | One `BOOT_FILE_LOADED`, one registration set, `BOOT_OK`, no Lua load error | Pass | Mod loaded once; hooks, notification, and loop registered |
| SP-02 | Local player hard reject | Remain in world after bootstrap | `PLAYER_AUDIT ... candidate_player_count=0`; no player identity/location log | Pass | `audited=1 candidate_player_count=0` |
| SP-03 | Existing wild Pal discovery | Stand near at least one naturally spawned wild Pal | `SCAN_DONE raw>0`, at least one `PAL_ACCEPT`, field-path rows emitted once | Pass for acquisition core | Seventh run produced `PAL_ACCEPT`, found 8-12 raw monsters, retained 3-6 current wild candidates, rejected player-owned/trainer-associated Pals, and retained `candidate_player_count=0`. Field-path validation remains separate. |
| SP-04 | New spawn discovery | Move into a new spawn area and wait through one reconcile interval | `notification_count` or later `SCAN_DONE` increases; no duplicate notification registration | Pass for notification | Sixth run recorded 16 construction notifications. Repeated wrapper identity caused candidate accumulation across scans; the cache now rebuilds from the current scan instead of retaining wrapper keys. |
| SP-05 | Party/base Pal exclusion | Approach a party Pal and a base Pal | No `PAL_ACCEPT` for owned Pals; aggregate owned rejection increases | Pending | |
| SP-06 | Capture transition | Capture a currently guided wild Pal | Candidate is removed after ownership/classification changes or object invalidation | Pending | |
| SP-07 | Mount/dismount | Mount and dismount a Pal | No player candidate; mounted/owned Pal is not treated as wild | Pending | |
| SP-08 | Fast travel/interior | Fast travel, enter/leave an interior or dungeon if available | Session/candidate state rebuilds without stale UObject errors | Pending | |
| SP-09 | Death/respawn | Allow player death and respawn | `candidate_player_count=0`; no crash; bootstrap/session reset remains idempotent | Pending | |
| SP-10 | Return to title/reload | Return to title and load the save again | No duplicate notification/hook behavior; candidates repopulate safely | Pass | Seventh run cleared four candidates at `load_map_pre`, loaded the title, skipped later scans without a local player, and exited normally. |
| SP-11 | Diagnostic guide | Keep one accepted wild Pal on screen | `DRAW_READY anchor=top_center target_count=1`; line begins at screen top, not center | Blocked - LogicMod required | Ninth run registered the HUD Hook but recorded `draw_callbacks=0` throughout gameplay. `/Script/Engine.HUD:ReceiveDrawHUD` is not invoked by the current Palworld HUD, so Lua Canvas probing stops here. |
| SP-12 | Field availability | Inspect `FIELD_PATH` lines | Each required field is available or explicitly unavailable with a path/note | Partial, complex types blocked | Core scalar/FName methods passed. Tenth run proved `GetGenderType()` enum return marshalling crashes UE4SS just like direct `Gender`; gender, arrays, and struct-based utility fields now require a Blueprint bridge. |
| SP-13 | Stability sample | Play normally for at least 15 minutes | No Mod-caused crash; periodic `METRIC` lines continue | Partial - short recovery smoke passed | Eleventh run validated the restored profile through save entry, repeated reconciliation, return to title, and normal exit. No crash occurred; the full 15-minute sample remains pending. |
| SP-14 | Normal mod compatibility | Repeat load/discovery with the maintainer's normal Mod set | No player boundary failure; conflicts documented | Pass for short smoke | Eleventh run used the maintainer's normal UE4SS Mod set, retained `candidate_player_count=0`, accepted wild Pals, rejected owned/trainer-associated Pals, returned to title, and exited normally. |

## Optional Community Server Cases

These rows are intentionally not maintainer-blocking. Contributors should attach the relevant log excerpt with player-identifying values removed.

| ID | Environment | Steps | Required Result | Status | Actual/Notes |
|---|---|---|---|---|---|
| MP-01 | Host/co-op client | Join or host a co-op world and wait for bootstrap | Mod loads without a server component | Community pending | |
| MP-02 | Host/co-op remote player | Have another human enter loaded range | `candidate_player_count=0`; no guide or field probe for the human | Community pending | |
| MP-03 | Dedicated-server client | Join a dedicated server and move through populated areas | No crash; only client-replicated objects are considered | Community pending | |
| MP-04 | Dedicated remote players | Observe other humans at several distances | Zero player candidates and zero identifying diagnostics | Community pending | |
| MP-05 | Replication boundary | Move toward and away from a known Pal spawn | Record discovery/removal distances as observations, not loading guarantees | Community pending | |
| MP-06 | Reconnect/travel | Disconnect, reconnect, die/respawn, and travel | No duplicate registration or stale UObject crash | Community pending | |
| MP-07 | Field replication | Compare `FIELD_PATH`/values with single-player | Missing server fields remain unavailable, never guessed | Community pending | |

## Log Extraction

```powershell
rg -n "\[PalworldResourceESP\].*(BOOT_|SESSION_|CLASS_|PLAYER_AUDIT|PAL_|FIELD_PATH|SCAN_|METRIC|DRAW_|ERROR)" "E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.log"
```

Privacy check:

```powershell
rg -ni "\[PalworldResourceESP\].*(player name|player uid|platform id|steam id|location=)" "E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.log"
```

Expected privacy-check result: no matches.

## Completion Rule

Phase 2 bridge work may start under a `CONDITIONAL GO` when acquisition/player-boundary invariants pass and any rendering or complex-field blockers have reproducible evidence plus an ADR-consistent next step. Phase 1 is fully complete only after the remaining applicable single-player rows and stability sample pass. MP rows remain `Community pending` and must not be presented as verified compatibility.
