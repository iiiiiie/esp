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
| Current deployed panel pak | `B5690A72AEC77D5E3B7E9E0027E76A61476FB4FCCC6A2664AB11F93B20DA9D6B`; runtime regression pending |
| Current Pak contents | 10 files under `../../../Pal/Content/Mods/PalworldResourceESP/`; no DLL |
| Current Lua script hash | `A08DFFA0808594440497A0EF5651DB9FAA133B2393CEB3EF0CB37DF5E3BF118B` |
| Current Lua config hash | `EFDDC99B1E233D527CB36BF55F215A1C1444BCB31BF2B7F1F348E2C24EF6EF39` |

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
| BP-10 | Normal exit | No new crash report after title wait and normal game exit | Pass | Final gate run exited normally at 16:45; the newest crash directory remained the pre-fix 09:56 record. |
| BP-11 | Multi-target guide | More than one accepted loaded Pal produces simultaneous guide lines; each endpoint follows its own Pal | Pass | Maintainer confirmed simultaneous lines, each following a different Pal, including guides toward targets outside the current screen. Far targets appear only after the client exposes an initialized target actor. |
| BP-12 | Panel toggle and input | `Shift+E` opens/closes the panel; mouse cursor, Game/UI input, and gameplay input restore correctly | Pending | Requires the next Steam single-player run. |
| BP-13 | Localization | Panel starts in Chinese and `Language` switches all panel labels to/from English | Pending | Requires the next Steam single-player run. |
| BP-14 | Runtime master switch | Disabling the Mod immediately clears every guide and stops discovery; enabling it resumes the selected mode | Pending | Automated scalar-poll and runtime-off behavior pass; visual regression remains. |
| BP-15 | Diagnostic modes | Off, snapshot-once, current chunking, and event-first remain functional; each change emits one marker | Pending | Automated mode transitions, stale-job invalidation, and marker output pass. |
| BP-16 | Panel lifecycle | Capture/death cleanup, return to Title, and normal exit remain crash-free with the panel package | Pending | Required before ADR-0006 can be accepted. |

## Privacy Check

```powershell
rg -ni "\[PalworldResourceESP\].*(player name|player uid|platform id|steam id|location=)" "E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.log"
```

Expected result: no matches.

## Completion Rule

The original Blueprint bridge spike passed BP-01 through BP-11 on Steam single-player. The panel checkpoint is complete only when BP-12 through BP-16 also pass without regressing BP-01 through BP-11 or `candidate_player_count=0`.
