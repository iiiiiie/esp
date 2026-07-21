# ADR-0006: Use a Panel Control Plane and External Performance Capture

## Status

Proposed

## Date

2026-07-17

## Context

The Entity Core smoke tests proved multi-target guides, capture/death cleanup, map teardown, and the hard player boundary. They also measured reconciliation callbacks above the frame budget and showed that subjective remote-play reports are not sufficient to compare algorithms.

The maintainer needs to switch Mod behavior and mark benchmark intervals without restarting the game or repeating a streaming route. The runtime remains a pure client-side UE4SS Lua plus LogicMod package, while PresentMon remains an external ETW tool. Earlier startup tests also showed that a Blueprint-initiated UE4SS custom event can crash before the first frame.

## Decision

Add a dedicated `WBP_ESPPanel` controlled by `Shift+Y` and keep it separate from the per-frame overlay.

- Lua owns the key bind and invokes Blueprint panel methods.
- The key callback records one pending request and never schedules another GameThread callback. The existing 250 ms runtime tick consumes the request only after reconciliation is idle, then returns without starting discovery in the same tick.
- While open, the panel owns input through `UIOnlyEx`, applies the Pal SDK's reversible `PalPlayerController::SetDisableInputFlag` under the unique `PalworldResourceESP_Panel` flag, and returns `Handled` for otherwise-unconsumed key-down/key-up events. Text boxes retain first chance to consume editing input. Escape key-down records a widget-local pending flag and remains `Handled` while UI-only ownership is intact; the matching key-up clears the flag and dispatches the existing panel toggle path. Escape remains native game input while the panel is closed.
- The panel writes only scalar control properties and a monotonically increasing revision on the passive `ModActor`.
- User-facing numeric controls pair a Slider for rapid adjustment with a compact integer SpinBox for exact entry. Level remains one grouped two-endpoint range; visible targets are clamped to 1-100.
- Distance exposes only a 0-330m maximum control. Its lower bound is fixed at 0m, and the deprecated `ESP_DistanceMin` property is neither displayed nor read by Lua.
- Runtime, top-guide, level, and distance visibility each use one compact toggle. Recreated panel widgets initialize numeric and toggle controls from the passive `ModActor`; programmatic initialization still crosses only the Blueprint-to-Blueprint boundary.
- Binary display controls use a 28x24 checkbox with a visible light outline in both states, a medium-gray unchecked fill, and a green checked fill so unchecked rows cannot blend into the panel surface.
- Gender uses an all/male/female segmented selector backed by the scalar `ESP_GenderFilterId`. Exactly one segment uses the green accent; click handlers update the accent immediately, and panel initialization restores it from the current scalar value. Blueprint reads each already-admitted wild Pal's typed gender, normalizes it to `0/1/2`, and rejects non-matching targets before projection and drawing. The human-player gate remains earlier and cannot be changed by this selector.
- The panel uses a restrained neutral surface, green state accent, compact rounded controls, and a viewport-filling frame with a 16px margin. It exposes `Display` and `Filters` tabs; the filter page is split between a scrollable expandable passive-skill catalog and regular filters so the full workflow remains usable at 720p-class heights.
- Passive skills use eight visible Rank-only groups: Rainbow (`>=5`), Legend (`4`), Gold III (`3`), Gold II (`2`), Normal (`1`), and negative I/II/III (`-1/-2/-3`). Rank 0 and unknown ranks remain hidden instead of being guessed from acquisition weight. Dynamic catalog text inherits the cooked UMG font. Empty, `None`, and missing-data rows fail closed before widget creation. A committed localized-name search rebuilds the catalog only on Enter, focus loss, or an explicit Search action. Hover uses a dedicated rich-text tooltip with project-owned styles for confirmed numeric tags. Clear-passive and clear-all actions update both the Blueprint-owned filter state and visible controls.
- Visible distance is computed from live player and target locations in the Blueprint paint pass. The snapshot distance remains the filter and ordering value.
- Lua polls those properties every 250 ms and applies changes on the GameThread.
- No panel option can bypass the registry-owned human-player rejection gate.
- Runtime profiles are `off`, `snapshot_once`, `chunked_current`, and `event_first`.
- The internal profile IDs remain `off`, `snapshot_once`, `chunked_current`, and `event_first` for marker compatibility, but the UI labels profile 2 as `safe snapshot` after the chunking experiment was rejected.
- Reconciliation normalizes every `FindAllOf` result before returning from the discovery callback. No UE4SS Actor wrapper may be retained in a delayed Lua admission job.
- `event_first` treats construction notifications as dirty markers and starts a fresh safe snapshot on the next runtime tick; it never queues the notification's Actor wrapper.
- Filter and display-limit changes rebuild a fresh snapshot. Actor-free style changes use `PalworldResourceESP_SetDisplayStyle` and do not resubmit old targets.
- Capture buttons emit timestamped `PERF_SESSION_START`, `PERF_MODE_CHANGED`, and `PERF_SESSION_STOP` log markers.
- A repository PowerShell companion controls PresentMon and writes benchmark data outside the repository. The packaged Mod never launches a process or adds a runtime DLL.
- Fixed-view segmented captures measure steady-state behavior. A separate movement capture remains required for entity-streaming behavior.

The advanced diagnostics controls are present in the public package but collapsed by default. The resulting winning discovery strategy is selected by functional correctness, then the lowest rate of frames over 50 ms, then P99 frame time.

## Options Considered

### Option 1: Continue restart-based A/B captures

Advantages:
- No panel work is required.

Disadvantages:
- World streaming, cache state, route timing, and remote input make comparisons noisy.
- The maintainer must repeatedly leave and restart the game.

### Option 2: Let Blueprint launch or embed PresentMon

Advantages:
- One visible control surface.

Disadvantages:
- Violates the asset-only runtime boundary.
- Adds process-management and distribution risk to the Mod.

### Option 3: Panel markers with an external capture companion

Advantages:
- Keeps runtime pure-client and asset-only.
- Produces exact segment boundaries while the player remains in one world session.
- Reuses the proven UE4SS log and PresentMon measurement paths.

Disadvantages:
- Capture buttons require the companion tool to be running for frame data to exist.
- Panel-to-Lua scalar polling adds a small fixed control-plane cost that must be measured.

## Rationale

Option 3 isolates benchmark modes without expanding runtime authority. Scalar polling avoids the failed Blueprint-to-Lua startup event direction, and the external companion preserves PresentMon as a development tool instead of a player dependency.

## Consequences

Positive effects:
- The maintainer can run a complete segmented comparison from inside the game.
- Runtime-off, render-only, safe-snapshot, and event-first behavior can be compared in one process.
- Benchmark logs remain free of player identity and coordinates.

Negative effects:
- The panel and polling loop remain loaded when ESP discovery is off.
- A separate fully-disabled launch is still needed to quantify that fixed control-plane overhead.
- Immediate admission can produce a larger single GameThread callback than chunking; performance optimization resumes only after the crash-free functional baseline is restored.

## Runtime Safety Amendment (2026-07-17)

Two movement runs crashed at `15:04:37` and `15:07:26` with the same GameThread stack hash `2FA5718AD4F57F6F86CD5562225EC795F904F1AE` and access address `0x000000010000201b`. In the latter run, the last completed events were repeated two-target snapshot synchronizations; the next two-Actor/16-ms reconcile had discovered 33 wrappers and had not completed its first admission batch. UE4SS `pcall` cannot contain this native failure.

The delayed chunking option is therefore rejected for the functional baseline. This amendment favors wrapper lifetime safety over frame-time smoothing and follows ADR-0007's rule that a reproducible crash blocks feature work. A future performance design must avoid retaining UE4SS wrappers across callbacks, for example by moving the hot path into a native or Blueprint-owned representation.

## Panel Interaction Amendment (2026-07-17)

[Material slider](https://m3.material.io/components/sliders/overview), [Carbon slider](https://carbondesignsystem.com/components/slider/usage/), and [Carbon toggle](https://carbondesignsystem.com/components/toggle/usage/) guidance is applied to the generated UMG panel: sliders expose the complete supported range and preview their numeric value while dragging, then commit one scalar revision when mouse or controller capture ends. Exact SpinBox values commit once through `OnValueCommitted`. Binary settings use one reversible toggle, and command actions remain buttons. UE 5.1 UMG has no native two-thumb RangeSlider, so the level range uses two compact synchronized Slider/SpinBox endpoint rows rather than a custom runtime widget.

## Panel Toggle Serialization Amendment (2026-07-18)

A Steam run ended immediately after `PANEL_TOGGLE_REQUESTED` while a synchronous reconciliation was active; no dispatch/completion marker or new dump was produced. This does not prove a native stack cause, but the old key callback scheduled a second GameThread callback while reconciliation already owned that thread. Panel toggles now share the existing runtime tick with reconciliation. Requests remain pending until reconciliation is idle, a toggle tick performs no discovery, and stale lifecycle requests are skipped. The delayed callback path remains only as dated deprecated source for rollback history.

## Pal Input Isolation Amendment (2026-07-19)

`UIOnlyEx`, keyboard focus, `bStopAction`, and the generic Actor `DisableInput` call did not prevent Palworld shortcuts such as Escape, Backspace, and T from changing game UI or cursor state. The panel now uses the game's public `PalPlayerController::SetDisableInputFlag` contract with a Mod-owned flag and clears that same flag on close. Panel `OnKeyDown` and `OnKeyUp` overrides return `Handled` only after a focused child has had the opportunity to consume the event, preserving editable-text behavior while blocking unconsumed gameplay shortcuts. A Steam run showed that closing immediately inside Escape key-down restored `GameOnly` before the same physical press finished routing, so Palworld also opened its native menu. The amended graph records Escape on key-down and invokes the canonical ModActor toggle only on the matching key-up; input isolation therefore remains active throughout key-down while flag clearing, cursor restoration, `GameOnly`, and input flushing still have one implementation. An unconditional `OnPreviewKeyDown` handler is rejected because it would intercept letters and IME editing before the search fields receive them.

## Follow-ups

- [x] Generate, cook, and package `WBP_ESPPanel` through ADR-0004 automation.
- [x] Implement the PresentMon watcher, marker parser, segmented analyzer, and synthetic contract tests.
- [x] Revalidate deferred open/close and gameplay input restoration in Steam single-player.
- [x] Validate Shift+Y button clicks and UI/game input restoration after the `UIOnlyEx` fix.
- [x] Validate the V2 Slider/SpinBox synchronization, 1-100 target ceiling, fixed 0m distance lower bound, and 330m distance ceiling in Steam single-player.
- [x] Validate all/male/female filtering in Steam single-player.
- [x] Validate selected-segment highlighting and restoration after reopening the panel.
- [ ] Run forward and reverse fixed-view profile sequences with PresentMon.
- [ ] Run one movement capture after a steady-state strategy passes.
- [ ] Accept this ADR only after panel lifecycle, capture segmentation, and normal exit pass.
