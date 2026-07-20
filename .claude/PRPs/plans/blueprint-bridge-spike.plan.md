# Blueprint Bridge Spike Plan

## Summary

Phase 2 proves the minimum LogicMod/Blueprint path required by ADR-0001 and ADR-0002. It does not build the final ESP panel or broad resource registry. The spike was initially one-target and was amended after that path passed: it now hands every accepted loaded wild Pal to one Blueprint Widget, draws top-anchored guides, and reads gender inside Blueprint without letting Blueprint enumerate or accept human players.

## Current Evidence

- Steam PC build: `24088745` at the Phase 1 test time; `24181527` at the Phase 2 gate.
- UE4SS: v3.0.1 Beta #0, Git SHA `c2ac2464`.
- Lua acquisition, player rejection, snapshot reconciliation, and map teardown passed.
- `ReceiveDrawHUD` registered but produced zero callbacks.
- Direct `Gender` access and `GetGenderType()` enum marshalling crashed UE4SS Lua.
- Existing architecture: `docs/adr/0001-use-ue4ss-hybrid-client-mod-architecture.md`.
- Toolchain decision: `docs/adr/0002-use-pinned-palworld-modding-kit-toolchain.md`.

## Success Criteria

1. PMK opens and compiles at the pinned commit with the recorded UE/MSVC/Windows SDK versions and the ADR-0003 no-Wwise variant.
2. `PalworldResourceESP` loads as a LogicMod and emits one unique bridge-ready event per gameplay ModActor instance.
3. Lua caches only the current valid bridge actor and clears it before map teardown.
4. Lua passes only actors that already passed the Phase 1 wild/non-player classification, up to `MAX_DISPLAY_TARGETS`.
5. Blueprint creates one unframed viewport Widget and draws a thin line from the top-center anchor to every accepted target.
6. Blueprint reads and reports the target Pal gender without routing the enum through Lua.
7. A local human player never enters candidate, bridge, Widget target, label, or diagnostic state.
8. Entering a save, waiting near wild Pals, returning to title, and normal exit do not crash.
9. The `.pak` is reproducibly produced and installed under `Pal/Content/Paks/LogicMods/PalworldResourceESP.pak`.

## Non-Goals

- Final `Shift+Y` settings panel.
- Boxes, outlines, names, health, weak points, or dedicated off-screen indicators. Multi-target guides were added by the scope amendment.
- Passive-skill arrays, IVs, Lucky/Alpha, elements, capture count, or resource adapters.
- Multiplayer verification.
- Forced entity loading, direct memory reads, save mutation, server components, or C++ hot paths.

## Scope Amendment (2026-07-16)

The original spike limited the Lua-to-Blueprint handoff and guide to one target. Packaged single-target testing proved the passive bridge, Widget projection, map teardown, and top-anchored guide, after which the maintainer approved extending the same spike to all currently accepted loaded wild Pals.

The amended scope is:

- Lua submits every accepted candidate up to `MAX_DISPLAY_TARGETS`; it never submits a player or lets Blueprint enumerate actors.
- One overlay Widget owns a typed `PalMonsterCharacter` array and draws all guides in one `OnPaint` pass.
- Snapshot synchronization clears the Widget array and then appends the current accepted candidates.
- The Blueprint-local gender adapter diagnoses the first accepted target once per Widget session as `unknown`, `male`, or `female`; `EPalGenderType` never crosses into Lua.
- Targets outside the client's usable actor/individual-parameter state remain outside the bridge. The observed far-distance visibility boundary is documented as a loaded-entity limitation, not a renderer omission.

This is a runtime-evidence-based extension of Tasks 5 and 6. Boxes, labels, the settings panel, broad resource adapters, and forced loading remain non-goals for this spike.

## Minimal Bridge Contract

All names are project-unique.

### Bridge Discovery

- `ModActor.PostBeginPlay` remains passive.
- Lua observes `ModActor_C` with `RegisterBeginPlayPostHook` and caches only the current matching class instance.
- The packaged `PalworldResourceESP_LuaBridgeReady` signature remains for rollback compatibility but is not invoked; packaged Blueprint-to-UE4SS calls with `self` crashed during startup.

### Lua to Blueprint

- Function: `PalworldResourceESP_ResetSession(SessionId: int)`
- Function: `PalworldResourceESP_SetTarget(Target: PalMonsterCharacter, SessionId: int)`
- Function: `PalworldResourceESP_ClearTarget(SessionId: int)`

Rules:

- Lua calls bridge functions only on the GameThread and only when the bridge UObject is valid.
- The target comes exclusively from the existing candidate snapshot.
- Session IDs prevent delayed calls from reviving a target after map travel.
- No player object, player identity, player position, or player parameter is passed to Blueprint.
- Blueprint owns per-frame projection only; it does not scan the world.

## Planned Repository Layout

```text
LogicMod/
  Content/
    Mods/
      PalworldResourceESP/
        ModActor.uasset
        BP_ESPBridge.uasset
        WBP_ESPOverlay.uasset
        PalworldResourceESP.uasset
  README.md
docs/
  adr/
  research/
  testing/
PalworldResourceESP/
  Scripts/
```

The `LogicMod/Content/Mods/PalworldResourceESP` directory is project-owned. The full PMK checkout, Wwise files, Unreal-generated caches, packaged `Windows` directory, and game assets remain untracked outside this repository.

## Tasks

### Task 1: Bootstrap and Record the Toolchain

- Verify .NET 6, VS Build Tools, Windows SDK, UE 5.1, the ADR-0003 no-Wwise variant, and the PMK commit.
- Add the missing MSVC 14.38 component without replacing newer installed toolsets.
- Keep installer/account steps explicit and record any maintainer interaction.
- Verify PMK opens before adding project assets.

### Task 2: Create the External PMK Checkout

- Clone `localcc/PalworldModdingKit` under the sibling `palworld_mod/tooling` directory.
- Pin commit `62fad4130238cb0aadf024b87496e7387d5f4bf5`.
- Keep the upstream Wwise integration out of the product repository and use the ADR-0003 no-Wwise compatibility module for this non-audio LogicMod.
- Record the external path and hashes without committing third-party files.

### Task 3: Establish Project-Owned LogicMod Assets

- Create `LogicMod/Content/Mods/PalworldResourceESP` in this repository.
- Connect it to PMK with a verified reversible Junction or a documented copy step.
- Assign a unique non-zero chunk ID after inspecting existing project labels.
- Create `ModActor`, `BP_ESPBridge`, and `WBP_ESPOverlay` in Unreal Editor.

### Task 4: Implement Bridge Initialization

- `ModActor` passively owns the Widget bridge events.
- Lua discovers the current `ModActor_C`, validates it, and replaces stale bridge state.
- LoadMap pre-hook clears the cached bridge before the old world tears down.

### Task 5: Connect the Existing Candidate Snapshot

- Preserve all Phase 1 classification and ownership checks.
- Copy every current accepted wild candidate up to `MAX_DISPLAY_TARGETS`.
- Call `ClearTarget` before each non-empty snapshot, then call `SetTarget` once per accepted target.
- Do not increase scan frequency for rendering.

### Task 6: Draw the Top-Anchored Guide

- Create one viewport overlay Widget with stable full-screen dimensions.
- Project each target actor location in one Widget `OnPaint` pass.
- Draw a thin line from `(viewport width / 2, TOP_ANCHOR_Y)` to every projected target.
- Hide the line for invalid, destroyed, behind-camera, or off-world targets.
- Do not place the line origin at screen center.

### Task 7: Prove the Gender Adapter

- Blueprint obtains the accepted Pal's individual parameter through typed Blueprint nodes.
- Blueprint reads `EPalGenderType` locally and converts it to a diagnostic string or integer inside Blueprint.
- Only the normalized diagnostic value may be shown/logged; the enum does not return through Lua.
- Failure remains explicit and does not crash or fabricate a value.

### Task 8: Package and Deploy Reversibly

- Package the unique chunk.
- Rename the produced chunk to `PalworldResourceESP.pak`.
- Preflight `Pal/Content/Paks/LogicMods` and preserve any existing target in a dated `__DEPRECATED_` rollback snapshot before replacement.
- Keep the Lua Junction deployment unchanged unless the bridge integration requires a documented update.

### Task 9: Execute the Phase 2 Smoke Matrix

- Launch, enter the existing single-player save, and wait near a wild Pal.
- Verify bridge-ready, `candidate_player_count=0`, simultaneous top lines, and gender evidence.
- Move/rotate to confirm the endpoint tracks the Pal without changing the top anchor.
- Return to title, wait, and exit normally.
- Record build/tool versions, logs, screenshot evidence, and crash-report timestamps.

## Validation Commands

```powershell
git status --short --branch
git diff --check
rg -n "PalworldResourceESP_(LuaBridgeReady|ResetSession|SetTarget|ClearTarget)" PalworldResourceESP LogicMod docs
rg -ni "player name|player uid|platform id|steam id|location=" PalworldResourceESP LogicMod
```

Runtime validation remains required because static checks cannot validate Blueprint serialization, cooking, Widget projection, or UE4SS custom-event marshalling.

## Rollback

- Tool installations are removed through their installers, not filesystem deletion.
- External PMK or generated directories are renamed with `__DEPRECATED_YYYYMMDD_` before any later cleanup.
- Junctions are renamed with the same protocol and never replaced over an existing path.
- A failed `.pak` is renamed with `__MISTAKE_YYYYMMDD_` and the previous package is restored by name.
- Git changes are reverted with normal commits; no hard reset or force push.

## Phase Gate

Phase 2 passes only when the minimal bridge, top-anchored guide, Blueprint-local gender adapter, player exclusion invariant, map teardown, and normal exit all pass in the maintainer's Steam single-player environment. Multiplayer remains community-pending.

Outcome (2026-07-16): `PASS`. BP-01 through BP-11 passed on Steam build `24181527`. The final run emitted `BLUEPRINT_GENDER value=female`, retained `candidate_player_count=0`, cleared 23 candidates before returning to Title, and exited without a new crash. Capture and death cleanup passed in dedicated runs. The deployed eight-file pak SHA-256 is `C3AFD891EDF00E671BB2ACD677E275843F79C0DC6BA472AB5BD7E96573245B14`.
