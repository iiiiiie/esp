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
| Current deployed panel pak | `A803671ACD24D5DD60594022958802D851094E2E1D78C65B9231548F91B8E5BE`; multi-select element filter checkpoint, runtime regression pending |
| Current Pak contents | 10 files under `../../../Pal/Content/Mods/PalworldResourceESP/`; no DLL |
| Current Lua script hash | `BD0E003522CEF9A97D38E2D53C7A386371C594996F0124047C25FAB763A56FA4` |
| Current Lua config hash | `00C56472CC7B2BBA2A7BA1488093EEB3C6B0C2D9BB5DE3B0E9B8A0881763F33E` |
| Current settings module hash | `D7D2695B6A326DF6FDAABB68D362004AED1EE978B973DAF6EF1E7E6C1353E738` |

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
| BP-12 | Panel toggle and input | `Shift+Y` opens/closes the panel; mouse cursor, UI input, and gameplay input restore correctly | Pass | Maintainer confirmed repeated Shift+Y open/close, clickable controls, and restored mouse/camera input without a crash. |
| BP-13 | Localization | Panel starts in Chinese and `Language` switches all panel labels to/from English | Pass | Maintainer confirmed the Language control switches both directions. |
| BP-14 | Runtime master switch | Disabling the Mod immediately clears every guide and stops discovery; enabling it resumes the selected mode | Pass | Maintainer confirmed guides disappear when disabled and return when enabled. |
| BP-15 | Diagnostic modes | Off, snapshot-once, safe snapshot, and event-first remain functional; each change emits one marker | Pending | Internal `chunked_current` ID is retained only for marker compatibility; automated wrapper-safe transitions pass. |
| BP-16 | Panel lifecycle | Capture/death cleanup, return to Title, and normal exit remain crash-free with the panel package | Pending | Required before ADR-0006 can be accepted. |
| BP-17 | Gender filter behavior | All/male/female modes filter already-admitted wild Pals | Pass | Maintainer confirmed male and female filtering both work and then exited normally. |
| BP-18 | Gender selector visual state | Exactly the selected segment is green after a click and after panel recreation | Pass | Maintainer confirmed the rebuilt selector highlight is correct. |
| BP-19 | Slider commit behavior | Dragging updates only the visible number; releasing applies one filter revision without continuous hitching | Pending | Source and automated contract complete; Steam verification required. |
| BP-20 | Name label and outline | Name visibility is independent; all name/level/distance text uses a black outline | Pending | Generated Blueprint and Steam verification required. |
| BP-21 | Settings restoration | Restart restores the last stable functional settings and language without restoring capture state | Pending | Parser/runtime tests pass; full restart verification required. |
| BP-22 | Lucky filter | All/only Lucky/exclude Lucky filters already-admitted wild Pals; unknown states fail closed in restricted modes | Pending | `IsRarePal()` provider, three-state UI, persistence, Blueprint compilation, and 10-file Pak checks pass; Steam verification requires an ordinary Pal and ideally a Lucky sample. |
| BP-23 | Boss filter | All/only Boss/exclude Boss filters already-admitted wild Pals; unknown states fail closed in restricted modes | Pending | `GetCharacterID()` + character database `GetIsBoss()` provider, three-state UI, `v3` persistence, Blueprint compilation, and 10-file Pak checks pass; Steam verification requires a fixed-map Boss and ordinary Pal. |
| BP-24 | Element filter | Nine compact toggles filter already-admitted wild Pals with match-any semantics; no selection means all and unknown masks fail closed | Pending | `HasElementType()` provider, 3x3 UI, scalar mask bridge, strict `v4` persistence, Blueprint compilation, 408/408 clean Cook, and 10-file/0-DLL Pak checks pass; ordinary Pals are sufficient for Steam verification. |

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
| Next run | Settings restoration plus element selector | Pending. Verify no-selection, one-element, and two-element OR behavior with ordinary Pals, then restart and confirm selected element chips restore. Lucky/Boss positive matching remains sample-pending. |

## Privacy Check

```powershell
rg -ni "\[PalworldResourceESP\].*(player name|player uid|platform id|steam id|location=)" "E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.log"
```

Expected result: no matches.

## Completion Rule

The original Blueprint bridge spike passed BP-01 through BP-11 on Steam single-player. The current panel checkpoint is complete only when BP-12 through BP-24 pass without regressing BP-01 through BP-11 or `candidate_player_count=0`.
