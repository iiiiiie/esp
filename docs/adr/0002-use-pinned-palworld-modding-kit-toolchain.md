# ADR-0002: Use a Pinned Palworld Modding Kit Toolchain

## Status

Proposed

## Date

2026-07-15

## Context

ADR-0001 selects a UE4SS Lua plus LogicMod/Blueprint Widget architecture. The Phase 1 runtime spike proved that the Lua acquisition core can discover loaded wild Pals, reject human players before candidate insertion, and survive map teardown. It also produced two reproducible blockers:

- Palworld does not invoke the tested `ReceiveDrawHUD` Lua hook.
- UE4SS Lua crashes while marshalling the tested gender enum, and complex array/struct fields remain unsafe to probe.

The active community documentation uses the unofficial `localcc/PalworldModdingKit` project to compile Blueprint LogicMods. That project is tied to Unreal Engine 5.1, .NET 6, Visual Studio 2022 with the MSVC 14.38 toolchain, and Wwise 2021.1.11 integration. These are substantial, version-sensitive dependencies and should not be inferred from whichever tools happen to be installed on a contributor machine.

The current maintainer machine has .NET 6, Visual Studio Build Tools 2022, and Windows SDK 10.0.26100. It does not currently have Unreal Engine 5.1, Epic Games Launcher, Wwise, or MSVC 14.38.

## Decision

Phase 2 will use a pinned external checkout of `localcc/PalworldModdingKit` as the LogicMod build environment.

- Initial PMK pin: commit `62fad4130238cb0aadf024b87496e7387d5f4bf5`.
- Required engine: Unreal Engine 5.1.x.
- Required runtime/build prerequisites: .NET 6, Visual Studio 2022 MSVC 14.38, and Wwise 2021.1.11 SDK/offline Unreal integration.
- The PMK checkout and third-party tool installations remain outside this repository.
- This repository stores only project-owned Lua, Blueprint assets, packaging metadata, plans, and test evidence.
- During local development, the project-owned LogicMod asset directory may be connected into PMK with a reversible Windows Junction after both paths are verified.
- The packaged LogicMod is named `PalworldResourceESP.pak`; it must not use the `_P` suffix.
- Lua and Blueprint communication uses project-unique event/function names prefixed with `PalworldResourceESP_`.
- Blueprint receives only actors already accepted by the Lua acquisition boundary. Blueprint does not enumerate players or widen the candidate set.
- Direct memory reads, signature scanning, external process access, server components, and mutation RPCs remain outside this decision.

The pin may be advanced only after recording the new PMK commit, Palworld build, tool versions, rebuild result, and a single-player smoke test.

## Options Considered

### Option 1: Pinned External Palworld Modding Kit

Advantages:

- Matches the current Palworld Modding community workflow.
- Provides Pal classes and Blueprint-visible APIs needed for the bridge.
- Keeps the large third-party project and licensed tool integrations outside the product repository.
- Makes upgrades explicit and testable.

Disadvantages:

- Requires an old Unreal/Wwise/MSVC compatibility matrix.
- Blueprint assets are binary and harder to review than Lua.
- Contributors must reproduce a comparatively heavy local toolchain.

### Option 2: Continue with Pure UE4SS Lua

Advantages:

- No Unreal Editor installation.
- Text-only source and fast iteration.

Disadvantages:

- Phase 1 reproduced native crashes for enum marshalling.
- The registered HUD draw hook never executes on the tested game build.
- Continuing speculative probes would expose the maintainer to repeated process crashes.

### Option 3: Build a Custom Unreal/C++ Toolchain

Advantages:

- Maximum control over bridge types and rendering.
- Potentially better performance for future hot paths.

Disadvantages:

- Much higher implementation and game-update maintenance cost.
- Premature before the Blueprint bridge and performance limits are measured.
- Would expand ADR-0001 and require a separate architecture decision.

### Option 4: Patch Existing Blueprint Assets Without Unreal Editor

Advantages:

- Smaller local installation.

Disadvantages:

- Fragile binary editing and poor reproducibility.
- Difficult to validate Blueprint graphs, references, cooking, and package dependencies.
- Encourages coupling to another Mod's assets or undocumented serialized formats.

## Rationale

The pinned PMK is the smallest toolchain that directly addresses both proven Lua blockers while preserving the acquisition-layer player exclusion. Keeping it external prevents third-party SDK and plugin churn from overwhelming the product repository. Pinning the commit and compiler versions makes the otherwise fragile compatibility matrix explicit.

The decision is initially Proposed because the current machine has not yet compiled and packaged a minimal LogicMod with this toolchain. It can become Accepted after the bridge-ready event, one top-anchored guide, safe gender read, map teardown, and normal exit all pass on the maintainer's Steam build.

## Consequences

Positive effects:

- Rendering and complex field access gain a supported Blueprint path.
- PMK upgrades are reviewable decisions instead of silent environment drift.
- The product repository stays focused on project-owned artifacts.

Negative effects:

- Initial setup requires Epic and Audiokinetic accounts and a large local installation.
- Blueprint binary diffs provide limited semantic review.
- Builds depend on legacy tool versions that may become harder to obtain.

Follow-up concerns:

- Record checksums or installer versions for UE, Wwise, and MSVC where practical.
- Keep an exportable graph description and screenshots for critical Blueprint logic.
- Never commit Wwise SDK files, Unreal Engine files, PMK third-party plugins, cooked game assets, or credentials.
- Treat PMK updates as compatibility changes and rerun the bridge smoke matrix.

## Follow-ups

- [ ] Install or verify Unreal Engine 5.1.x.
- [ ] Add MSVC 14.38 to Visual Studio Build Tools 2022.
- [ ] Install and integrate Wwise 2021.1.11.
- [x] Clone PMK outside the repository and pin commit `62fad4130238cb0aadf024b87496e7387d5f4bf5`.
- [ ] Compile and launch the PMK without project assets.
- [ ] Implement the minimal Lua/Blueprint bridge and top-anchored guide.
- [ ] Prove the Blueprint gender adapter without exposing human players.
- [ ] Package, deploy, and complete the Phase 2 single-player smoke test.
- [ ] Change this ADR to Accepted only after the toolchain and bridge pass.
