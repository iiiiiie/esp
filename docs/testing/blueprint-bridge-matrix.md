# Blueprint Bridge Smoke Matrix

## Status

`PASS - Phase 2 bridge baseline; panel checkpoint regression pending`

Multiplayer is community-pending and is not part of this maintainer-run matrix.

## Environment Fingerprint

| Field | Value |
|---|---|
| Palworld Steam build | `24181527` |
| UE4SS version/SHA | `v3.0.1 Beta #0`, Git SHA `c2ac2464` |
| PMK commit | `62fad4130238cb0aadf024b87496e7387d5f4bf5` |
| Unreal Engine | `5.1.1-23901901`, installed through Epic |
| MSVC | `14.38` installed; UBT selected `14.39.33523` for the current editor-plugin build |
| Wwise | Not required by the accepted no-Wwise PMK path |
| Last fully verified LogicMod pak | `C3AFD891EDF00E671BB2ACD677E275843F79C0DC6BA472AB5BD7E96573245B14`; 8 files |
| Current deployed panel pak | `364AF89C38EFE57FF4783EE141A575B0DF55EC0D21C0C6ED504AB6571AE145F6`; `v10` local collection-completion filter with complete panel persistence; Steam verification pending |
| Current Pak contents | 16 files under `../../../Pal/Content/Mods/PalworldResourceESP/`; no DLL |
| Current Lua script hash | `B8A6488DD49626A3D77C67C54B8083A9E34CCF6541A28E142FA4F3BE90B2B5A6` |
| Current Lua config hash | `DA3B598DD1854402D5A3ABE7BC012C2D910A4D77F5427E55948399F58F18EC53` |
| Current settings module hash | `AE36A98D226848889BBC493379B405C9ACA2DB23D1FD48CC4D46D02DB3318D29` |

## Required Cases

| ID | Scenario | Expected evidence | Status | Notes |
|---|---|---|---|---|
| BP-01 | LogicMod load | BPModLoader spawns `/Game/Mods/PalworldResourceESP/ModActor` | Pass | BPModLoader spawned `ModActor_C` in Login, Title, and MainWorld maps. |
| BP-02 | Bridge initialization | Lua discovers the current passive `ModActor_C` after BeginPlay and emits one `BRIDGE_READY mode=lua_discovery` | Pass | Blueprint-to-UE4SS custom-event self-callback was rejected after two reproducible splash crashes; Lua discovery is stable. |
| BP-03 | Player boundary | `PLAYER_AUDIT ... candidate_player_count=0`; no player object reaches bridge logs | Pass | Gameplay session reported `audited=1 candidate_player_count=0`. Critical invariant retained. |
| BP-04 | Wild target handoff | `BRIDGE_SESSION` then `BRIDGE_TARGETS_SYNCED`; submitted count never exceeds accepted candidates | Pass | Runtime `raw/candidates/submitted` stayed equal, growing from `6/6/6` to `23/23/23`; `candidate_player_count=0` remained intact. |
| BP-05 | Top guide | A thin green line begins at screen top-center and terminates at a visible Pal | Pass | Maintainer confirmed the packaged guide is visible. |
| BP-06 | Camera/target motion | Line endpoint follows the Pal; top anchor remains fixed | Pass | Maintainer confirmed the endpoint tracks the Pal while playing. |
| BP-07 | Gender adapter | Blueprint reports male/female/unknown without returning `EPalGenderType` through Lua | Pass | Final gate run emitted `BLUEPRINT_GENDER value=female` with no `BLUEPRINT_GENDER_PENDING`. Blueprint maps the enum to `0/1/2`; Lua reads only the integer. |
| BP-08 | Invalid target | Line hides when the target unloads, is destroyed, or is no longer selected | Pass | Capture removed its guide. Death removed its guide immediately, `CLASS_REJECT_DEAD` prevented re-admission, and the guide remained absent after six seconds. |
| BP-09 | Return to title | `BRIDGE_CLEARED reason=load_map_pre`; no stale bridge call or crash | Pass | Final gate run cleared the bridge and 23 candidates before travel; Title received a fresh passive actor. |
| BP-10 | Normal exit | No new crash report after title wait and normal game exit | Pass | Maintainer confirmed normal exit after validating the gender-filter package. |
| BP-11 | Multi-target guide | More than one accepted loaded Pal produces simultaneous guide lines; each endpoint follows its own Pal | Pass | Maintainer confirmed simultaneous lines, each following a different Pal, including guides toward targets outside the current screen. Far targets appear only after the client exposes an initialized target actor. |
| BP-12 | Panel toggle and input | `Shift+Y` opens/closes the panel; mouse cursor, UI input, and gameplay input restore correctly | Pending | Earlier panel packages passed. A 2026-07-18 run ended after a toggle request during synchronous reconciliation; the current package serializes toggle consumption and scanning on the 250 ms runtime tick and requires a Steam regression run. |
| BP-13 | Localization | Panel starts in Chinese and `Language` switches all panel labels to/from English | Pass | Maintainer confirmed the Language control switches both directions. |
| BP-14 | Runtime master switch | Disabling the Mod immediately clears every guide and stops discovery; enabling it resumes the selected mode | Pass | Maintainer confirmed guides disappear when disabled and return when enabled. |
| BP-15 | Diagnostic modes | Off, snapshot-once, safe snapshot, and event-first remain functional; each change emits one marker | Pending | Internal `chunked_current` ID is retained only for marker compatibility; automated wrapper-safe transitions pass. |
| BP-16 | Panel lifecycle | Capture/death cleanup, return to Title, and normal exit remain crash-free with the panel package | Pending | Required before ADR-0006 can be accepted. |
| BP-17 | Gender filter behavior | All/male/female modes filter already-admitted wild Pals | Pass | Maintainer confirmed male and female filtering both work and then exited normally. |
| BP-18 | Gender selector visual state | Exactly the selected segment is green after a click and after panel recreation | Pass | Maintainer confirmed the rebuilt selector highlight is correct. |
| BP-19 | Slider commit behavior | Dragging updates only the visible number; releasing applies one filter revision without continuous hitching | Pending | Source and automated contract complete; Steam verification required. |
| BP-20 | Name label and outline | Name visibility is independent; all name/level/distance text uses a black outline | Pending | Generated Blueprint and Steam verification required. |
| BP-21 | Settings restoration | Panel reopen, save transition, and restart restore the last stable functional settings, passive selections, category expansion, and language without restoring capture state | Pending | Strict `v10` parser/runtime, legacy migration, FString bridge, array reconstruction, compile, Cook, and package checks pass; maintainer reports persistence appears functional, while a controlled full-restart verification remains. |
| BP-22 | Lucky filter | All/only Lucky/exclude Lucky filters already-admitted wild Pals; unknown states fail closed in restricted modes | Pending | `IsRarePal()` provider, three-state UI, persistence, Blueprint compilation, and 12-file Pak checks pass; Steam verification requires an ordinary Pal and ideally a Lucky sample. |
| BP-23 | Boss filter | All/only Boss/exclude Boss filters already-admitted wild Pals; unknown states fail closed in restricted modes | Pending | `GetCharacterID()` + character database `GetIsBoss()` provider, three-state UI, `v3` persistence, Blueprint compilation, and 12-file Pak checks pass; Steam verification requires a fixed-map Boss and ordinary Pal. |
| BP-24 | Element filter | Nine compact toggles filter already-admitted wild Pals with match-any semantics; no selection means all and unknown masks fail closed | Pending | `HasElementType()` provider, 3x3 UI, scalar mask bridge, strict `v4` persistence, Blueprint compilation, clean Cook, and 12-file/0-DLL Pak checks pass; ordinary Pals are sufficient for Steam verification. |
| BP-25 | IV display | Optional `IV HP x / ATK y / DEF z` uses typed save-parameter fields for already-admitted wild Pals; unknown values are hidden instead of shown as zero | Pending | `GetSaveParameter()` plus `Talent_HP`/`Talent_Shot`/`Talent_Defense`, three indexed arrays, `v5` persistence, Blueprint compilation, clean Cook, and 12-file/0-DLL Pak checks pass; ordinary-Pal value correctness requires Steam verification. |
| BP-26 | IV minimum filter | Independent HP/ATK/DEF `0..100` minima hide a Pal unless every dimension meets its own threshold; zero disables that dimension | Pending | Typed provider, fail-closed three-comparison graph, actor-free scalar update, strict `v8` persistence, Blueprint compilation, clean Cook, and package checks pass; Steam threshold behavior requires verification. |
| BP-27 | Passive-skill display | Optional passive-skill text uses the game's localized names for already-admitted wild Pals and hides immediately when disabled | Pending | Blueprint calls `GetPassiveSkillList()` and resolves IDs with `PalUIUtility::GetPassiveSkillName()` without sending the source array through Lua; strict `v7` persistence, Blueprint compilation, clean Cook, and package checks pass. |
| BP-28 | Display checkbox contrast | Every display row has a clearly visible unchecked and checked state and an unambiguous label association | Pending | Generated controls use a fixed 28x24 outlined checkbox with medium-gray unchecked and green checked fills; each checkbox is immediately left of its automatic-width label; Steam visual verification remains. |
| BP-29 | Passive-skill AND filter | Zero to four skills may be selected; every selected game-provided passive ID must be present on a target | Pending | Blueprint-owned capped `TArray<FName>`, delimiter-safe per-target IDs, include/exclude mirror persistence, startup reconstruction, reset actions, clean Cook, and 16-file/0-DLL Pak checks pass; Steam behavior remains. |
| BP-30 | Passive catalog and tooltips | Catalog categories use the game's Rank field and hover text uses localized `SkillDesc` rich text with exact-ID fallback for missing/malformed rows | Pending | Eight visible groups map `>=5/4/3/2/1/-1/-2/-3` to Rainbow, Legend, Gold III, Gold II, Normal, and negative I/II/III; Rank 0 is hidden and `LotteryWeight` is not consulted. Each valid catalog ID resolves its localized name directly; Panel status 3 has 1560 nodes and the 16-file/0-DLL candidate package passes; Steam category counts remain. |
| BP-31 | Tabbed panel layout | 1180x680 panel switches between Display, Filters, and pending Display style without losing input or filter state | Pending | WidgetSwitcher, tab highlighting, two-column filter page, eight persisted expansion states defaulting collapsed, and repeat-generation dependency contract compile; 720p and higher visual verification remains. |
| BP-32 | Passive catalog search and invalid rows | Enter, focus loss, or Search applies a case-insensitive localized-name query; Clear restores the full valid catalog; empty, `None`, and missing-data rows never render | Pending | Generated graph binds `OnTextCommitted`, performs one catalog rebuild per commit, and gates widget creation on ID, data, localized name, and query validity; Steam interaction verification remains. |
| BP-33 | Passive right-click exclusion | Right-click toggles an unlimited exclusion, removes the same include, marks the entry `[排除]`, and hides any target carrying that ID; left-click inclusion removes the same exclusion | Pending | Blueprint-owned include/exclude arrays, mutual removal, both clear actions, three Overlay synchronization paths, and pre-projection exclusion compile; the mouse override function has 41 nodes and returns Unhandled on all paths. Steam interaction verification remains. |
| BP-34 | Local collection-completion filter | All shows every otherwise-matching target; incomplete requires a local species capture count below five; complete requires at least five; unknown counts are hidden in restricted modes | Pending | Blueprint uses the public local record and capture-count APIs only after Lua admission, stores one indexed count per target with `-1` fallback, applies the fixed threshold before projection, exposes a three-state selector, and persists it through strict `v10`; Steam verification remains. |

## Panel Regression Evidence

| Time | Checkpoint | Result |
|---|---|---|
| 2026-07-17 10:29 | Immediate Lua key callback, `Shift+E` | Open passed; Mod off/on revisions applied; second hotkey crashed before a completion marker. Crash: `UECC-Windows-80EE4EB444ECE760ECF7CD93A90F3836_0000`, access violation at `0xffffffffffffffff`, stack hash `2FA5718AD4F57F6F86CD5562225EC795F904F1AE`. |
| 2026-07-17 15:04 / 15:07 | Two-Actor/16-ms delayed reconcile while moving | Two matching GameThread access violations at `0x000000010000201b`, stack hash `2FA5718AD4F57F6F86CD5562225EC795F904F1AE`; retaining `FindAllOf` wrappers across callbacks is rejected. |
| 2026-07-17 10:41 | Deferred Lua key callback, `Shift+T`, 50 ms | Six toggle requests reached `PANEL_TOGGLE_COMPLETED`; no crash; mouse and camera input restored. All buttons were inert and `ESP_ControlRevision` stayed 0. |
| 2026-07-17 11:01 | Deferred `Shift+Y`, `UIOnlyEx` panel focus | Built and deployed; no game run yet. Pak contains 10 files and no DLL; offline Lua/control-plane tests pass. |
| 2026-07-17 functional panel run | Shift+Y, localization, master switch, range and visibility controls | Maintainer confirmed panel interaction, all numeric filters, target limit, and live level/distance display; no reported crash. |
| 2026-07-17 gender filter run | All/male/female behavior and normal exit | Male/female filtering passed and the game exited normally. Selected-button accent remained on the wrong segment, exposing BP-18. |
| 2026-07-17 gender highlight follow-up | Gender selector highlight fix | Maintainer confirmed the selected segment highlight is correct. |
| 2026-07-18 panel termination | Toggle request during synchronous reconciliation | The UE4SS log ended immediately after `PANEL_TOGGLE_REQUESTED`; there was no dispatch/completion marker and no new dump. The current package removes the second delayed GameThread callback and consumes requests only on an idle runtime tick. This is the strongest evidence-backed fix, not a dump-confirmed root cause. |
| Next run | Toggle safety, IV minimum, passive skills, and checkbox contrast | Pending. Repeatedly toggle the panel first, then verify the all-three IV threshold, localized passive-skill text, and distinct checkbox states. Lucky/Boss positive matching and restart persistence remain sample/runtime-pending. |

## Privacy Check

```powershell
rg -ni "\[PalworldResourceESP\].*(player name|player uid|platform id|steam id|location=)" "E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.log"
```

Expected result: no matches.

## Completion Rule

The original Blueprint bridge spike passed BP-01 through BP-11 on Steam single-player. The current panel checkpoint is complete only when BP-12 through BP-34 pass without regressing BP-01 through BP-11 or `candidate_player_count=0`.
