# ADR-0003: Evaluate a Wwise-Free PMK Compatibility Layer

## Status

Proposed

## Date

2026-07-15

## Context

Audiokinetic account activation and download access are currently unreliable for the maintainer (account activation failures and repeated CloudFront 403 responses). The ESP does not play, author, or modify game audio. Wwise is nevertheless coupled to the pinned Palworld Modding Kit (PMK): the `Pal` module declares an `AkAudio` dependency and Pal headers expose Wwise types.

The first no-Wwise probe disabled the Wwise plugin and removed the direct `AkAudio` dependency in an isolated PMK copy. UnrealBuildTool did not reach source compilation because the machine also lacks a discoverable Windows 10/11 SDK; this is an independent environment blocker. The clean PMK checkout remains unchanged.

## Decision

We will pursue a Wwise-free PMK development path for the ESP, initially as an isolated experiment:

- Keep Wwise disabled and do not require an Audiokinetic account for product contributors.
- Provide a minimal local `AkAudio` compatibility module only for the Pal source declarations required by the PMK build. The compatibility module is a no-op for audio and is not a replacement game audio system.
- Keep all compatibility-layer changes outside the product repository until a clean UE 5.1.1 build and single-player smoke test pass.
- Do not add Wwise SDK files, generated audio assets, credentials, memory readers, or server components to this repository.
- Retain pure UE4SS Lua as a fallback acquisition path, but do not expand its known unsafe enum/struct probes until the Blueprint bridge is evaluated.

## Options Considered

### Option 1: Install Wwise 2021.1.11

Advantages:

- Matches the PMK's documented environment.
- Avoids maintaining compatibility stubs.

Disadvantages:

- Blocked by the current Audiokinetic account/download path.
- Adds a large, version-sensitive third-party SDK that the ESP does not use.
- Makes contributor setup and reproducibility dependent on a vendor account.

### Option 2: Wwise-free `AkAudio` compatibility module (selected for evaluation)

Advantages:

- Removes the vendor account from the minimum build path.
- Preserves Pal declarations needed by Blueprint reflection while leaving audio behavior as no-op.
- Keeps the change isolated and reversible until validated.

Disadvantages:

- The Pal source may require more Wwise types than the initial scan shows.
- The resulting PMK editor is not suitable for testing Pal audio behavior.
- Requires a Windows SDK and a full UHT/UBT validation before it can be trusted.

### Option 3: Pure UE4SS Lua only

Advantages:

- Requires no Unreal Editor, PMK, or Wwise.

Disadvantages:

- Phase 1 already reproduced a HUD hook gap and unsafe enum/struct marshalling.
- It does not currently provide a reliable rendering and complex-field path for the planned ESP.

## Rationale

The ESP has no audio requirement, so carrying Wwise solely to compile unrelated Pal declarations is undesirable. A small compatibility module is a narrower experiment than rewriting the Pal source or switching immediately to an unproven C++ runtime. The proposal remains unaccepted until UHT, UBT, packaging, and the single-player smoke matrix pass.

## Consequences

Positive effects:

- Audiokinetic access is no longer a hard prerequisite for the ESP build investigation.
- The product repository remains free of Wwise binaries and licensed SDK content.
- Audio behavior is explicitly out of scope for the compatibility layer.

Negative effects:

- PMK setup still requires UE 5.1.1, MSVC, and a Windows SDK.
- The shim may expose additional compile or reflection incompatibilities.
- Contributors who need to work on Pal audio systems will still need the real Wwise integration.

## Follow-ups

- [x] Install or otherwise expose a Windows 10/11 SDK to UBT.
- [x] Add the minimal `AkAudio` compatibility module in the experimental PMK copy.
- [x] Run UHT/UBT; the stub passed UHT and the incremental `PalEditor` build completed.
- [x] Launch the no-Wwise editor target headlessly; `AkAudioStub` mounted and the editor exited normally after the module-entry fix.
- [ ] Package the bridge and run the approved Steam single-player smoke matrix.
- [ ] Change this ADR to `Accepted` only after the no-Wwise path passes those checks.
