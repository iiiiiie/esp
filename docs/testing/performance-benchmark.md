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
| Metrics | PresentMon v2 with `FrameTime` and absolute `CPUStartDateTime` for panel captures |

The executable is a portable development tool and is not shipped with the Mod.

## Fixed-View Panel Procedure

1. Keep the same Steam build, save, graphics settings, resolution, frame cap, and other Mods for the complete run.
2. Start the external watcher before launching or entering Palworld. It waits for one game PID and starts one named PresentMon session.
3. Enter the save, wait 20 seconds near several wild Pals, then remain still without moving or rotating the camera.
4. Open the panel with `Shift+Y`, expand advanced diagnostics, and start capture.
5. Run `off -> snapshot_once -> chunked_current -> event_first`, holding each mode for at least 15 seconds.
6. Run the same modes in reverse order, again holding each for at least 15 seconds, then stop capture.
7. Retain dropped and long frames. The analyzer excludes only the first two seconds after every mode marker.

The assistant normally starts the watcher so the maintainer only operates the in-game panel:

```powershell
./tools/performance/Watch-PalworldPanelCapture.ps1
```

The watcher writes one absolute-time CSV plus one `.segments.json` file under `D:/AllDownload/PalworldResourceESP-Benchmarks`. Analyze them together:

```powershell
./tools/performance/Measure-PresentMon.ps1 `
    -CandidateCsv <capture.csv> `
    -SegmentMetadataPath <capture.segments.json>
```

Each segment reports its profile, preset, timestamps, frame percentiles, and normalized hitch counts. A mode remains eligible only if guide behavior, capture/death cleanup, lifecycle, and `candidate_player_count=0` are correct. Among eligible modes, select the lowest `Hitch50PerMinute`, then the lowest P99 frame time.

## Follow-up Captures

After one steady-state strategy passes, run one movement capture to measure new-Pal streaming and admission cost. The older fixed-duration tool remains available for a fully disabled process-level A/B baseline:

```powershell
./tools/performance/Capture-Palworld.ps1 -Label mod-off-run1 -DurationSeconds 60 -DelaySeconds 5
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

- PresentMon attaches read-only ETW capture to an existing Palworld process; it does not launch, inject into, or modify the game.
- The watcher stops only its unique PresentMon trace session after the matching panel stop marker or game-process exit.
- Segment metadata contains only session, mode, preset, reason, and timestamps.
- Benchmark CSV files stay outside the repository by default under `D:/AllDownload/PalworldResourceESP-Benchmarks`.
- Never record player names, platform IDs, UIDs, or player coordinates in benchmark metadata.
