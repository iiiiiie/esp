# ADR-0004: Use editor-only C++ automation for LogicMod blueprint graphs

## Status

Accepted

## Date

2026-07-15

## Context

The PMK assets must be generated without requiring the maintainer to operate Unreal Editor manually. UE 5.1 exposes blueprint creation through Python, but its Python wrapper does not expose `EdGraph.Nodes` or node creation/linking APIs. The LogicMod itself must remain a pure client-side asset package and must not depend on a new server component or a runtime native DLL.

## Decision

Add an editor-only `ESPBlueprintAutomation` plugin to the experimental PMK. It constructs and links the project-owned Blueprint graphs and overlay widget tree through Unreal's C++ editor APIs. The plugin is enabled only while generating/cooking assets and is not included in the packaged LogicMod runtime.

## Options Considered

### Option 1: Manual Unreal Editor authoring

Advantages:
- Uses the standard editor UI.

Disadvantages:
- Requires user interaction and is difficult to reproduce or verify.

### Option 2: Python-only graph editing

Advantages:
- No additional C++ source.

Disadvantages:
- UE 5.1 Python does not expose the protected graph node collection or node creation APIs needed for deterministic generation.

### Option 3: Editor-only C++ automation

Advantages:
- Uses the supported native graph APIs and can be run unattended.
- Does not add runtime dependencies to the LogicMod.

Disadvantages:
- Adds a small PMK-only build dependency and requires rebuilding the editor target.

## Rationale

Option 3 meets the user's requirement that the assistant perform the editor work while preserving a pure asset-based runtime mod. The added module is isolated from the packaged chunk and can be removed or disabled after cooking without changing the generated assets.

## Consequences

Positive:
- Blueprint graph generation is reproducible and scriptable.
- No manual editor operation is required.
- Runtime remains client-only and asset-based.

Negative:
- The experimental PMK editor target must be rebuilt once after adding the plugin.
- Generated assets should be regenerated when the graph contract changes.

## Follow-ups

- [ ] Validate the generated bridge events in a Steam single-player smoke test.
- [ ] Keep the editor-only plugin out of the final LogicMod package.
