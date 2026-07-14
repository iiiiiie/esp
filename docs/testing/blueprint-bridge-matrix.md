# Blueprint Bridge Smoke Matrix

## Status

`NOT RUN - toolchain bootstrap in progress`

Multiplayer is community-pending and is not part of this maintainer-run matrix.

## Environment Fingerprint

| Field | Value |
|---|---|
| Palworld Steam build | Re-read before the first run |
| UE4SS version/SHA | Re-read newest `UE4SS.log` |
| PMK commit | `62fad4130238cb0aadf024b87496e7387d5f4bf5` |
| Unreal Engine | Pending; required 5.1.x |
| MSVC | Pending; required 14.38 |
| Wwise | Pending; required 2021.1.11 |
| LogicMod package hash | Pending |
| Lua script hash | Pending |

## Required Cases

| ID | Scenario | Expected evidence | Status | Notes |
|---|---|---|---|---|
| BP-01 | LogicMod load | BPModLoader spawns `/Game/Mods/PalworldResourceESP/ModActor` | Pending | |
| BP-02 | Bridge initialization | One `BRIDGE_READY event=PalworldResourceESP_LuaBridgeReady` for the current gameplay ModActor | Pending | |
| BP-03 | Player boundary | `PLAYER_AUDIT ... candidate_player_count=0`; no player object reaches bridge logs | Pending | Critical |
| BP-04 | Wild target handoff | `BRIDGE_SESSION` then `BRIDGE_TARGET_SET` for an accepted wild Pal | Pending | |
| BP-05 | Top guide | One thin line begins at screen top-center and terminates at the visible Pal | Pending | Screenshot required |
| BP-06 | Camera/target motion | Line endpoint follows the Pal; top anchor remains fixed | Pending | |
| BP-07 | Gender adapter | Blueprint reports male/female/unknown without returning `EPalGenderType` through Lua | Pending | |
| BP-08 | Invalid target | Line hides when the target unloads, is destroyed, or is no longer selected | Pending | |
| BP-09 | Return to title | `BRIDGE_CLEARED reason=load_map_pre`; no stale bridge call or crash | Pending | |
| BP-10 | Normal exit | No new crash report after title wait and normal game exit | Pending | |

## Privacy Check

```powershell
rg -ni "\[PalworldResourceESP\].*(player name|player uid|platform id|steam id|location=)" "E:/Steam/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/UE4SS.log"
```

Expected result: no matches.

## Completion Rule

The Blueprint bridge spike passes only when BP-01 through BP-10 pass on Steam single-player. A bridge load without a visible top guide, or a guide without `candidate_player_count=0`, is not sufficient.
