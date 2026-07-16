# Entity Core Test Matrix

## Status

- Implementation: complete, runtime regression pending
- Maintainer-required environment: Steam PC single-player
- Server environments: community pending and not a Phase 2 gate
- Human player invariant: every runtime run must report `candidate_player_count=0`
- Last updated: 2026-07-16

## Current Fingerprint

| Field | Value |
|---|---|
| Steam build ID | `24181527` |
| UE4SS version | `v3.0.1 Beta #0`, Git SHA `c2ac2464` |
| Active UE4SS root | `E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss` |
| Deployment | Junction from the active Mod directory to repository `PalworldResourceESP` |
| Implementation branch | `codex/entity-core`; use `git log -1` for the current checkpoint commit |
| LogicMod pak SHA-256 | `C3AFD891EDF00E671BB2ACD677E275843F79C0DC6BA472AB5BD7E96573245B14` |
| `main.lua` SHA-256 | `9E3617DB781C409A3C4C9E9D2059151E3CA1CADD8A993524F7C3B061BA8EB583`; source and active Junction match |
| `config.lua` SHA-256 | `C43B30E3B984668431F5A28D4ECCF1F4750FBF7249E07F4ABA09D9977F609C31` |
| Newest pre-test crash | `2026-07-16 09:56:11`, `UECC-Windows-FCB962FF452A9E89E36EB182C7EE5C0C_0000` |
| Runtime baseline backup | `E:/AAA_qian/ji_ji_tui_jin/palworld_mod/esp_backups/20260716_entity_core_baseline` |

Record final source hashes and the implementation commit immediately before the runtime run.

## Automated Evidence

| ID | Check | Required result | Status | Evidence |
|---|---|---|---|---|
| AT-01 | Lua parser | Every shipped and test Lua file parses as Lua 5.3 | Pass | `npm test` parsed 12 files |
| AT-02 | Pure core isolation | Core modules contain no UE4SS runtime globals | Pass | Test runner static check passed |
| AT-03 | Snapshot cells | Only finite scalars or scalar lists can be `known` | Pass | Opaque-value test rejects a function value |
| AT-04 | Generation lifecycle | Replacement drops the old lookup; clear does not dereference actors | Pass | Snapshot generation and teardown tests pass |
| AT-05 | Filter semantics | exact/range/boolean/any/all/exclude and fail-closed states pass | Pass | Filter suite passes |
| AT-06 | Ordering/budget | Known distance ascends, ordinal breaks ties, budget truncates after ordering | Pass | Ordering/budget suite passes |
| AT-07 | Player gate ordering | Player gate executes before adapter classification | Pass | Registry trace contains only `player_gate` for a player rejection |
| AT-08 | Adapter extension | A synthetic second adapter requires no core changes | Pass | Fake resource adapter test passes |
| AT-09 | Pal field normalization | Opaque species is not retained; unsafe fields have explicit states | Pass | Pal adapter test passes |
| AT-10 | Runtime entrypoint load | Stubbed UE4SS registration sequence loads without an error | Pass | Adapter, bridge, notification, lifecycle, draw, and loop registration completes |
| AT-11 | Dependency audit | Development dependency audit has no known vulnerability | Pass | `fengari 0.1.5`, `tmp 0.2.7`, `luaparse 0.3.1`; `npm audit` reports zero |
| AT-12 | Capability/privacy scan | No sensitive player diagnostics or prohibited mutation primitives | Pass | Both source scans return no matches |

## Runtime Matrix

| ID | Scenario | Required evidence | Status | Actual/Notes |
|---|---|---|---|---|
| EC-01 | Boot and adapter registration | `ADAPTER_REGISTERED id=pal`, `BOOT_FILE_LOADED`, no import/load error | Pending | Stub load passes; real UE4SS run required |
| EC-02 | Player boundary | `PLAYER_AUDIT ... candidate_player_count=0`; no player record or bridge actor | Pending | Release-blocking invariant |
| EC-03 | Generation replacement | `ENTITY_SNAPSHOT` generation increments; counts do not accumulate across scans | Pending | Unit lifecycle passes |
| EC-04 | Field states | Level/distance are known when available; unsafe fields remain bridge/unavailable | Pending | Unit normalization passes; inspect runtime field logs |
| EC-05 | Filter semantics | Automated predicate suite passes | Pass | Covered by AT-05 |
| EC-06 | Ordering and budget | Counts satisfy `raw >= admitted >= matched >= displayed`; displayed is at most 64 | Pending | Unit ordering passes; runtime bridge count required |
| EC-07 | Notification/reconcile interaction | Notification adds no duplicate wrapper in the current generation; next scan replaces it | Pending | Current-generation lookup is covered by AT-04 |
| EC-08 | Capture and death | Captured and dead targets disappear and stay absent after reconciliation | Pending | Previous bridge baseline passed; Entity Core regression required |
| EC-09 | Return to Title | Snapshot and bridge clear before old-world teardown | Pending | Return, wait 10 seconds, then inspect lifecycle log order |
| EC-10 | Performance | `discovery_ms`, `admission_ms`, `filter_ms`, `order_ms`, `budget_ms`, and bridge timing emit | Pending | Instrumentation exists; runtime values required |
| EC-11 | Adapter extension | Synthetic second adapter changes no gate/filter/renderer module | Pass | Covered by AT-08 |
| EC-12 | Normal exit | No new crash report; source and active deployment hashes match | Pending | Game is not running |

## Runtime Procedure

1. Confirm the game process is stopped and record source/deployment hashes plus the newest crash timestamp.
2. Launch Palworld, enter the Steam single-player save, and remain near multiple wild Pals for at least 20 seconds.
3. Confirm multiple independent top-anchored guides remain visible. No guide may target a human player.
4. Capture one guided Pal and kill another. Confirm both guides disappear and stay absent after at least six seconds.
5. Return to Title, wait at least 10 seconds, and exit normally.
6. Extract `ADAPTER_REGISTERED`, `PLAYER_AUDIT`, `ENTITY_SNAPSHOT`, `FILTER_RESULT`, `DISPLAY_BUDGET`, `SCAN_DONE`, `METRIC`, bridge, lifecycle, and error rows.
7. Re-run the privacy/capability scans and compare the crash directory timestamp.

## Completion Rule

ADR-0005 remains `Proposed` until EC-01 through EC-12 pass. After that evidence is recorded, change the ADR to `Accepted`, mark Phase 2 complete in the PRD, update source/deployment hashes, and make the Draft PR ready for review. Multiplayer remains community-pending.
