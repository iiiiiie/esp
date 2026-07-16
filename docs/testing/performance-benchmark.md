# Performance Benchmark

## Purpose

Use objective presented-frame timings to evaluate ESP overhead. Maintainer perception remains useful for reporting symptoms, but it is not the pass/fail gate.

The benchmark combines:

- PresentMon `FrameTime` for end-to-end frame pacing;
- UE4SS `SCAN_DONE` and `METRIC` rows for discovery, admission, batch, filter, ordering, budget, and bridge CPU time;
- identical Mod-enabled and Mod-disabled routes.

## Tool Fingerprint

| Field | Value |
|---|---|
| Tool | PresentMon Console `v2.5.1` |
| Source | `GameTechDev/PresentMon` GitHub release |
| Executable | `D:/AllDownload/PresentMon-2.5.1-x64.exe` |
| SHA-256 | `9BEC3083069F58F911E6A512F4806DB51A27BD096103087BC1D05EF54C80A191` |
| Metrics | PresentMon v2 with `FrameTime` and relative `CPUStartTime` |

The executable is a portable development tool and is not shipped with the Mod.

## Fixed Procedure

1. Use the same Steam build, save, graphics settings, resolution, frame cap, other Mods, route, camera behavior, and 60-second duration.
2. Enter the save and wait until world loading settles before capture.
3. Capture at least two Mod-enabled runs and two Mod-disabled runs. Alternate order when possible to reduce thermal and cache bias.
4. Begin the same movement route when the five-second capture delay expires. Move through an area that streams new Pals.
5. Do not open menus, fast travel, or change graphics settings during a capture.
6. Retain dropped and long frames; they are part of the result.
7. Compare the median run from each mode and attach sanitized UE4SS events covering the same wall-clock window.

Capture command:

```powershell
./tools/performance/Capture-Palworld.ps1 -Label mod-on-run1 -DurationSeconds 60 -DelaySeconds 5
```

Analyze one capture:

```powershell
./tools/performance/Measure-PresentMon.ps1 -CandidateCsv <capture.csv>
```

Compare Mod enabled against disabled:

```powershell
./tools/performance/Measure-PresentMon.ps1 `
    -BaselineCsv <mod-off.csv> `
    -CandidateCsv <mod-on.csv>
```

## Metrics

Primary metrics:

- P95 and P99 frame time in milliseconds;
- approximate 1% and 0.1% low FPS derived from P99 and P99.9 frame time;
- frames over 33.33 ms, 50 ms, and 100 ms;
- hitch counts normalized per minute.

Supporting Mod metrics:

- `discovery_ms` and `admission_ms`;
- `max_batch_ms` and reconcile batch count;
- filter, ordering, budget, and bridge synchronization time;
- raw, admitted, matched, and displayed entity counts.

Average FPS is reported but is not a sufficient acceptance metric.

## Acceptance Gate

Compared with the Mod-disabled baseline:

| Metric | Maximum regression |
|---|---:|
| P95 frame time | 5% |
| P99 frame time | 10% |
| Frames over 50 ms | 2 additional frames per minute |
| Frames over 100 ms | 0 additional frames |
| Mod `max_batch_ms` target | Less than 8 ms |

The gate is provisional until at least two complete A/B pairs establish normal run-to-run variance. A result fails if any gate fails or if capture/death cleanup, the player boundary, map teardown, or normal exit regresses.

## Safety

- PresentMon attaches read-only ETW capture to an existing Palworld process; it does not launch, stop, inject into, or modify the game.
- The capture command reports expected local start/end times so the matching UE4SS log window can be extracted.
- Benchmark CSV files stay outside the repository by default under `D:/AllDownload/PalworldResourceESP-Benchmarks`.
- Never record player names, platform IDs, UIDs, or player coordinates in benchmark metadata.
