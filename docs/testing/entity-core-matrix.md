# Entity Core Test Matrix

## Status

- Implementation: wrapper-safe snapshots, live target distance, independent metadata visibility, numeric panel ranges, gender filtering, and capture tooling complete; gender/runtime regression pending
- Maintainer-required environment: Steam PC single-player
- Server environments: community pending and not a Phase 2 gate
- Human player invariant: every runtime run must report `candidate_player_count=0`
- Last updated: 2026-07-17

## Current Fingerprint

| Field | Value |
|---|---|
| Steam build ID | `24181527` |
| UE4SS version | `v3.0.1 Beta #0`, Git SHA `c2ac2464` |
| Active UE4SS root | `E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss` |
| Deployment | Junction from the active Mod directory to repository `PalworldResourceESP` |
| Implementation branch | `codex/entity-core`; use `git log -1` for the current checkpoint commit |
| LogicMod pak SHA-256 | `1F253B790A39EA79FF602C955F3EC4999883DD9A4B8BAD24BA40CB0986952442`; gender-filter checkpoint |
| `main.lua` SHA-256 | `0F80D0C261B4A3E58926F7F4A37C75AB5027A0AEA26D32D6725D3A29881EE40F` |
| `config.lua` SHA-256 | `8DFE28F204DF66F1F2E212665F2D23B0A6D1789DA118FCF56717B0F7A19699C9` |
| Newest recorded crash | `2026-07-17 15:07:26`, `UECC-Windows-226A46F0425C013D3C68EAA1329F05E1_0000`; delayed-wrapper reconcile during movement |
| Runtime baseline backup | `E:/AAA_qian/ji_ji_tui_jin/palworld_mod/esp_backups/20260716_entity_core_baseline` |

Record final source hashes and the implementation commit immediately before the runtime run.

## Automated Evidence

| ID | Check | Required result | Status | Evidence |
|---|---|---|---|---|
| AT-01 | Lua parser | Every shipped and test Lua file parses as Lua 5.3 | Pass | `npm test` parsed 14 files |
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
| AT-13 | Wrapper-safe reconcile | Simulated Pal actors complete without scheduling an Actor-bearing delayed callback | Pass | Stub emits `RECONCILE_IMMEDIATE ... wrapper_lifetime_safety` and leaves the delayed queue empty |
| AT-14 | Wrapper-safe teardown | Periodic reconciliation completes inline and map pre-load leaves no stale Actor callback | Pass | Stub confirms one complete scan and an empty delayed queue before and after teardown |
| AT-15 | Frame-time analyzer | PresentMon v2 CSV statistics and same-file A/B comparison are deterministic | Pass | Existing 4,100-frame capture remains analyzable after the extension |
| AT-16 | Runtime profiles | Off, snapshot, compatibility-ID safe snapshot, and event-first resolve deterministic intervals and display budgets | Pass | Profile unit suite covers defaults, invalid IDs, fixed intervals, and 32/64/128 display budgets |
| AT-17 | Panel control plane | Shift+Y deferred bridge call, scalar revision polling, runtime-off clearing, capture markers, and stale-job invalidation | Pass | Stubbed runtime verifies two toggles run after, never inside, UE4SS key callbacks |
| AT-18 | Capture segmentation | Concatenated UE4SS markers and PresentMon absolute timestamps split by mode with a 2-second transition exclusion | Pass | Synthetic parser plus 31-frame end-to-end segmented analysis pass |
| AT-19 | Panel range filters | Level endpoints compose with a fixed 0m distance lower bound; unavailable fields remain fail-closed | Pass | Stubbed runtime ignores legacy `ESP_DistanceMin=999`, clamps the maximum to 330m, and validates both 0-0m and 0-330m ranges |
| AT-20 | Top-guide style control | The panel boolean round-trips through scalar polling without resubmitting snapshot Actors | Pass | Stubbed runtime records both hidden and shown actor-free style transitions |
| AT-21 | LogicMod package | Generated package contains exactly five `.uasset` and five `.uexp` files with no runtime DLL | Pass | Clean Cook completed 408/408 with 0 errors; UnrealPak lists 10 files, 0 DLL, SHA-256 `1F253B790A39EA79FF602C955F3EC4999883DD9A4B8BAD24BA40CB0986952442` |
| AT-22 | Target display metadata | The exact Entity Core level and rounded snapshot distance accompany each submitted target | Pass | Stubbed bridge receives level 1 and distance 1m for the first synthetic target |
| AT-23 | Numeric panel controls | Slider plus exact numeric input preserves scalar polling for level endpoints, 0-330m maximum distance, and the 1-100 display limit | Pass | Stubbed runtime reduces five displayed targets to three, clamps an attempted 999-target limit to 100, and restores the configured limit |
| AT-24 | Metadata visibility | Level and distance booleans round-trip independently through the actor-free style bridge | Pass | Stub records hidden and shown combinations without resubmitting snapshot Actors |
| AT-25 | Live distance graph | Overlay computes meters from current player and target locations during each paint pass | Pass | Generator links `GetPlayerPawn`, both live Actor locations, `Vector_Distance`, meter conversion, and integer text; final Blueprint statuses are warning-only/up-to-date |
| AT-26 | Gender filter bridge | Scalar all/male/female control reaches Blueprint without resubmitting Actors; invalid selectors clamp to female | Pass | Stub records `0/1/2`, generator stores one normalized gender code per admitted target, and OnPaint filters before projection |

## Performance Investigation

| Run | Implementation | Observed result |
|---|---|---|
| 2026-07-16 18:38 | Synchronous Entity Core | Functional pass; scans reached `239 ms` at 33 raw actors; admission consumed `1028 / 1581 ms` cumulative |
| 2026-07-16 19:10 | Deferred notifications and reduced probes | Functional pass but no perceived stutter improvement; scans reached `300 ms` at 37 raw actors; admission consumed `2038 / 3085 ms` cumulative |
| Offline checkpoint | Two actors per delayed GameThread batch, 16 ms between batches | Four simulated actors completed in two batches and one atomic replacement; real UE4SS timing remains unverified |
| 2026-07-16 23:43 | Chunked reconcile | Functional pass but maintainer perceived worse stutter; 51 raw actors required 26 batches, one batch reached `24 ms`, and scan CPU reached `358 ms` |
| 2026-07-17 15:04 / 15:07 | Delayed wrapper chunking while moving | Two matching GameThread access violations; stack hash `2FA5718AD4F57F6F86CD5562225EC795F904F1AE`; delayed Actor retention rejected |

All three real runs passed capture cleanup, death cleanup, return to Title, normal exit, and the player boundary. The second run deferred all 42 active construction notifications, proving that construction-time classification was not the remaining dominant stall. Chunking reduced the maximum single callback relative to the full scan, but repeated over-budget batches prolonged the disturbance and failed the subjective gate. PresentMon A/B measurement now replaces perception as the performance pass/fail gate; see [performance-benchmark.md](performance-benchmark.md).

## Runtime Matrix

| ID | Scenario | Required evidence | Status | Actual/Notes |
|---|---|---|---|---|
| EC-01 | Boot and adapter registration | `ADAPTER_REGISTERED id=pal`, `BOOT_FILE_LOADED`, no import/load error | Regression pending | Historical runs passed; wrapper-safe checkpoint has automated coverage only |
| EC-02 | Player boundary | `PLAYER_AUDIT ... candidate_player_count=0`; no player record or bridge actor | Regression pending | Both real runs reported `candidate_player_count=0`; recheck current checkpoint |
| EC-03 | Generation replacement | `ENTITY_SNAPSHOT` generation increments; counts do not accumulate across scans | Regression pending | Real generations incremented without accumulation; wrapper-safe replacement has automated coverage |
| EC-04 | Field states | Level/distance are known when available; unsafe fields remain bridge/unavailable | Regression pending | Real `FIELD_STATE` evidence passed before chunking |
| EC-05 | Filter semantics | Automated predicate suite passes | Pass | Covered by AT-05 |
| EC-06 | Ordering and budget | Counts satisfy `raw >= admitted >= matched >= displayed`; displayed is at most 64 | Regression pending | Pre-chunk runs passed with up to 31 displayed; recheck current checkpoint |
| EC-07 | Notification/reconcile interaction | Construction notifications retain no Actor wrapper; next safe scan replaces the generation without duplicates | Regression pending | Automated runtime proves an empty delayed queue; real movement run pending |
| EC-08 | Capture and death | Captured and dead targets disappear and stay absent after reconciliation | Regression pending | Maintainer confirmed both behaviors twice without a crash; recheck current checkpoint |
| EC-09 | Return to Title | Pending reconcile cancels without dereferencing actors; bridge clears before teardown | Regression pending | Pre-chunk teardown passed; chunk cancellation has static/stub coverage only |
| EC-10 | Performance | PresentMon A/B frame-time gates pass and Mod stage timings identify no over-budget callback | Pending | Chunked run reached `max_batch_ms=24`; standardized Mod-on/off captures are required |
| EC-11 | Adapter extension | Synthetic second adapter changes no gate/filter/renderer module | Pass | Covered by AT-08 |
| EC-12 | Normal exit | No new crash report; source and active deployment hashes match | Regression pending | Both pre-chunk runs exited normally; no crash newer than `09:56:11` |
| EC-13 | Gender filtering | All shows both known genders; male/female modes show only matching targets; reopening the panel preserves the selection | Pending | Automated scalar bridge and generated Blueprint graph pass; Steam single-player verification required |

## Runtime Procedure

1. Confirm the game process is stopped and record source/deployment hashes plus the newest crash timestamp.
2. Launch Palworld, enter the Steam single-player save, and remain near multiple wild Pals for at least 20 seconds.
3. Confirm multiple independent top-anchored guides remain visible. No guide may target a human player.
4. Switch gender through all, male, and female. Confirm each restricted mode hides the opposite known gender, then close/reopen the panel and confirm the current selection is preserved.
5. Capture one guided Pal and kill another. Confirm both guides disappear and stay absent after at least six seconds.
6. Return to Title, wait at least 10 seconds, and exit normally.
7. Extract `ADAPTER_REGISTERED`, `PLAYER_AUDIT`, `ENTITY_SNAPSHOT`, `FILTER_RESULT`, `DISPLAY_BUDGET`, `SCAN_DONE`, `DISPLAY_STYLE`, bridge, lifecycle, and error rows.
8. Re-run the privacy/capability scans and compare the crash directory timestamp.

## Completion Rule

ADR-0005 remains `Proposed` until EC-01 through EC-13 pass. After that evidence is recorded, change the ADR to `Accepted`, mark Phase 2 complete in the PRD, update source/deployment hashes, and make the Draft PR ready for review. Multiplayer remains community-pending.
