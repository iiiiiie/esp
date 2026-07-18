# Entity Core Test Matrix

## Status

- Implementation: wrapper-safe snapshots, live target distance, independent metadata visibility, numeric panel ranges, gender, Lucky, Boss, multi-select element filtering, all-three IV minimum filtering, localized passive-skill display, and capture tooling complete; persistence and runtime regression pending
- Maintainer-required environment: Steam PC single-player
- Server environments: community pending and not a Phase 2 gate
- Human player invariant: every runtime run must report `candidate_player_count=0`
- Last updated: 2026-07-18

## Current Fingerprint

| Field | Value |
|---|---|
| Steam build ID | `24181527` |
| UE4SS version | `v3.0.1 Beta #0`, Git SHA `c2ac2464` |
| Active UE4SS root | `E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss` |
| Deployment | Junction from the active Mod directory to repository `PalworldResourceESP` |
| Implementation branch | `codex/entity-core`; use `git log -1` for the current checkpoint commit |
| LogicMod pak SHA-256 | `8A0492FBEBBB032E47836CE8AD6E237C2BA7EC671C519361F6AA595C966E1E60`; IV minimum, passive-skill display, and panel-safety checkpoint |
| `main.lua` SHA-256 | `2670C212EAE60D2DA2E9477DE7B9DB69DE09B08F5D5C857AF1345DE8B843C107` |
| `config.lua` SHA-256 | `DA3B598DD1854402D5A3ABE7BC012C2D910A4D77F5427E55948399F58F18EC53` |
| `user_settings.lua` SHA-256 | `FDC1740A4528750B3DFB17FB5B9889CB96E0B19BCFF23679E318C589F4D3DBEE` |
| Newest reported termination | `2026-07-18` after `PANEL_TOGGLE_REQUESTED` during synchronous reconciliation; no new dump, so the serialized-toggle fix remains regression-pending |
| Runtime baseline backup | `E:/AAA_qian/ji_ji_tui_jin/palworld_mod/esp_backups/20260716_entity_core_baseline` |

Record final source hashes and the implementation commit immediately before the runtime run.

## Automated Evidence

| ID | Check | Required result | Status | Evidence |
|---|---|---|---|---|
| AT-01 | Lua parser | Every shipped and test Lua file parses as Lua 5.3 | Pass | `npm test` parsed 16 files |
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
| AT-17 | Panel control plane | Shift+Y serialized bridge call, scalar revision polling, runtime-off clearing, capture markers, and stale-job invalidation | Pass | Stubbed runtime verifies key callbacks only register requests, active reconciliation defers them, and a toggle tick performs no scan |
| AT-18 | Capture segmentation | Concatenated UE4SS markers and PresentMon absolute timestamps split by mode with a 2-second transition exclusion | Pass | Synthetic parser plus 31-frame end-to-end segmented analysis pass |
| AT-19 | Panel range filters | Level endpoints compose with a fixed 0m distance lower bound; unavailable fields remain fail-closed | Pass | Stubbed runtime ignores legacy `ESP_DistanceMin=999`, clamps the maximum to 330m, and validates both 0-0m and 0-330m ranges |
| AT-20 | Top-guide style control | The panel boolean round-trips through scalar polling without resubmitting snapshot Actors | Pass | Stubbed runtime records both hidden and shown actor-free style transitions |
| AT-21 | LogicMod package | Generated package contains exactly five `.uasset` and five `.uexp` files with no runtime DLL | Pass | Clean Cook completed 408/408 with 0 errors; UnrealPak lists 10 files, 0 DLL, SHA-256 `8A0492FBEBBB032E47836CE8AD6E237C2BA7EC671C519361F6AA595C966E1E60` |
| AT-22 | Target display metadata | The exact Entity Core level and rounded snapshot distance accompany each submitted target | Pass | Stubbed bridge receives level 1 and distance 1m for the first synthetic target |
| AT-23 | Numeric panel controls | Slider plus exact numeric input preserves scalar polling for level endpoints, 0-330m maximum distance, and the 1-100 display limit | Pass | Stubbed runtime reduces five displayed targets to three, clamps an attempted 999-target limit to 100, and restores the configured limit |
| AT-24 | Metadata visibility | Level and distance booleans round-trip independently through the actor-free style bridge | Pass | Stub records hidden and shown combinations without resubmitting snapshot Actors |
| AT-25 | Live distance graph | Overlay computes meters from current player and target locations during each paint pass | Pass | Generator links `GetPlayerPawn`, both live Actor locations, `Vector_Distance`, meter conversion, and integer text; final Blueprint statuses are warning-only/up-to-date |
| AT-26 | Gender filter bridge | Scalar all/male/female control reaches Blueprint without resubmitting Actors; invalid selectors clamp to female | Pass | Stub records `0/1/2`, generator stores one normalized gender code per admitted target, and OnPaint filters before projection |
| AT-27 | Gender selector visual state | Click and panel initialization set exactly one segment to the green accent and reset the other two | Pass | Generated panel graph uses `SetBackgroundColor` on all three segments in every click path, clamps initialization to `0..2`, and derives colors from `GenderFilterId`; Blueprint compiles with status 3 |
| AT-28 | Versioned local settings | Only whitelisted bounded scalars round-trip; invalid lines fail closed; the last valid snapshot wins | Pass | Unit suite covers strict `v1` through `v7` migration, clamping, append format, debug-source and `package.searchpath` resolution, and coalesced runtime writes; the runtime stub omits the path override and resolves beside `config.lua` |
| AT-29 | Slider deferred commit | Slider movement only updates the paired SpinBox; mouse/controller release and SpinBox commit each increment once | Pass | Source contract test passes; regenerated panel compiles with status 3 and clean Cook completes with 0 errors |
| AT-30 | Name and outlined labels | Blueprint nickname remains indexed with each target; name visibility is independent and text has a black outline | Pass | Overlay stores one nickname per target, compiles with status 5, and clean Cook completes with 0 errors; UE 5.1 outline uses four black offset draws plus white foreground |
| AT-31 | Lucky filter provider | Blueprint reads `IsRarePal()` only after Lua admission; all/only/exclude is scalar-controlled and unknown fails closed | Pass | Unit/static contracts cover old-schema migration, `v3` round-trip, ID clamping, bridge payloads, indexed `-1/0/1` states, and pre-projection filtering; Overlay/Panel/ModActor compile and clean Cook completes 408/408 with 0 errors |
| AT-32 | Boss filter provider | Blueprint resolves CharacterID and `GetIsBoss()` only after Lua admission; all/only/exclude is scalar-controlled and unknown fails closed | Pass | Static/runtime contracts cover `v2` migration, `v3` round-trip, ID clamping, bridge payloads, indexed `-1/0/1` states, and pre-projection filtering; Overlay/Panel/ModActor compile, clean Cook completes 408/408 with 0 errors, and the Pak contains 10 files/0 DLL |
| AT-33 | Element filter provider | Blueprint reads nine `EPalElementType` values only after Lua admission; selected elements use match-any and unknown fails closed | Pass | Unit/static contracts cover `v3` migration, `v4` round-trip, Fire+Water mask `6`, aggregate bridge payload, indexed `-1/0..511` masks, and pre-projection bitwise filtering; Overlay/Panel/ModActor compile, clean Cook completes 408/408 with 0 errors, and the Pak contains 10 files/0 DLL |
| AT-34 | IV display provider | Blueprint reads `Talent_HP`, `Talent_Shot`, and `Talent_Defense` only after Lua admission; unknown stays `-1` and the visibility toggle persists | Pass | Unit/static contracts cover `v4` migration, `v5` round-trip, actor-free style payload, three indexed integer arrays, typed struct break, and display formatting; Overlay/Panel/ModActor compile, clean Cook completes 408/408 with 0 errors, and the Pak contains 10 files/0 DLL |
| AT-35 | IV minimum filter | Zero disables filtering; otherwise all three IVs must be known and greater than or equal to the clamped `0..100` minimum | Pass | Runtime/static contracts cover scalar polling, `v5` migration, `v6` round-trip, unknown fail-closed behavior, exact equality, and one-below-threshold rejection before projection |
| AT-36 | Passive-skill display provider | Blueprint resolves localized passive-skill names only after Lua admission and Lua receives only the visibility scalar | Pass | Static contracts cover the typed list provider, `PalUIUtility` localization, twelfth same-order array, actor-free visibility update, `v6` migration, and `v7` round-trip |
| AT-37 | Display checkbox and toggle serialization | Unchecked display controls remain visible, and a toggle requested during reconciliation runs only on the next idle tick | Pass | Generator uses outlined gray/green 28x24 checkboxes; the runtime regression stub triggers Shift+Y from inside the synthetic monster scan and confirms no delayed callback or same-tick dispatch |

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
| EC-13 | Gender filtering | All shows both known genders; male/female modes show only matching targets; reopening the panel preserves the value and highlighted segment | Pass | Maintainer confirmed filtering, selected-state highlighting, and normal exit. |
| EC-14 | Slider commit and label rendering | Dragging remains responsive; release applies once; name/level/distance text is outlined and independently switchable | Pending | Current runtime package has not been tested. |
| EC-15 | Settings restart | Last stable controls and language restore after a full restart; capture remains stopped | Pending | Current runtime package has not been tested. |
| EC-16 | Lucky filtering | All shows normal and Lucky Pals; only Lucky hides normal Pals; exclude Lucky hides Lucky Pals; selection survives panel recreation and restart | Pending | Automated provider and persistence contracts pass; requires a real Lucky sample for the positive/negative split. |
| EC-17 | Boss filtering | All shows normal and fixed-map Alpha/Boss Pals; only Boss hides ordinary Pals; exclude Boss hides the Boss; selection survives panel recreation and restart | Pending | Automated provider and persistence contracts pass; requires one known fixed-map Alpha/Boss plus an ordinary Pal. |
| EC-18 | Element filtering | No selection shows all; one selected element shows matching Pals; Fire+Water shows either type and preserves dual-element matches; selection survives panel recreation and restart | Pending | Automated provider, match-any, fail-closed, bridge, Blueprint compile, Cook, and persistence contracts pass; ordinary Pals are sufficient for Steam verification. |
| EC-19 | IV display | Enabling IVs shows `IV HP x / ATK y / DEF z` for ordinary Pals; values are plausible `0..100`, disabling hides the line, and the toggle survives panel recreation and restart | Pending | Typed provider, unknown sentinel, display formatting, `v5` persistence, Blueprint compile, Cook, and package contracts pass; runtime value correctness remains unverified. |
| EC-20 | IV minimum filtering | Zero shows all otherwise-matching Pals; a positive minimum shows only Pals whose HP, attack, and defense IVs all meet it | Pending | Automated all-three, equality, one-below, unknown fail-closed, bridge, Blueprint compile, Cook, and `v6` persistence contracts pass. |
| EC-21 | Passive-skill display | Enabling passive skills shows localized names for ordinary Pals; disabling hides them and the setting survives panel recreation | Pending | Typed provider, localization call, same-order storage, Blueprint compile, Cook, and `v7` persistence contracts pass. |
| EC-22 | Panel safety and display-state clarity | Repeated Shift+Y toggles remain crash-free around scan boundaries; every display checkbox has clearly distinct selected and unselected states | Pending | Serialized runtime-tick implementation and automated regression pass; current Steam package requires runtime and visual verification. |

## Runtime Procedure

1. Confirm the game process is stopped and record source/deployment hashes plus the newest crash timestamp.
2. Launch Palworld, enter the Steam single-player save, and remain near multiple wild Pals for at least 20 seconds.
3. Confirm multiple independent top-anchored guides remain visible. No guide may target a human player.
4. Switch gender through all, male, and female. Confirm each restricted mode hides the opposite known gender, then close/reopen the panel and confirm the current selection is preserved.
5. Switch Lucky through all, only Lucky, and exclude Lucky. Without a Lucky sample, at minimum confirm only Lucky hides all ordinary Pals; record the other two cases as sample-pending rather than passing them.
6. Near a fixed-map Alpha/Boss, switch Boss through all, only Boss, and exclude Boss. Confirm only Boss hides ordinary Pals and exclude Boss hides the Boss; close/reopen the panel to confirm the highlighted selection.
7. Test elements with ordinary Pals: clear all selections, select one element represented nearby, then select a second element. Confirm no selection shows all and two selections use OR rather than AND. Close/reopen the panel and confirm both selected chips remain checked.
8. Before changing filters, open and close the panel at least five times, including once near an expected five-second scan boundary. Confirm no crash and normal mouse/camera restoration, then confirm each display checkbox remains visible and clearly distinguishes checked from unchecked.
9. Enable IV display near ordinary Pals. Confirm each visible label contains plausible `0..100` HP/ATK/DEF values. Set the IV minimum to zero, then to one displayed Pal's lowest IV and one point above it; equality must retain that Pal and one point above must hide it because all three values are required.
10. Enable passive-skill display and confirm localized names appear. Disable it and confirm the line hides immediately; close/reopen the panel and confirm the selected visibility state is preserved.
11. Capture one guided Pal and kill another. Confirm both guides disappear and stay absent after at least six seconds.
12. Change at least three visible settings, wait two seconds, return to Title, and exit. Restart once and confirm the settings restore while capture remains stopped.
13. Extract `USER_SETTINGS_PATH`, `USER_SETTINGS_SAVED`, `USER_SETTINGS_LOADED`, `ADAPTER_REGISTERED`, `PLAYER_AUDIT`, `ENTITY_SNAPSHOT`, `FILTER_RESULT`, `DISPLAY_BUDGET`, `SCAN_DONE`, `DISPLAY_STYLE`, panel-toggle, bridge, lifecycle, and error rows.
14. Re-run the privacy/capability scans and compare the crash directory timestamp.

## Completion Rule

ADR-0005 remains `Proposed` until EC-01 through EC-22 pass. After that evidence is recorded, change the ADR to `Accepted`, mark Phase 2 complete in the PRD, update source/deployment hashes, and make the Draft PR ready for review. Multiplayer remains community-pending.
