# ADR-0008: Persist Versioned Local User Settings

## Status

Proposed

## Date

2026-07-17

## Context

The in-game panel currently resets to Blueprint defaults whenever Palworld starts. Re-entering display, filter, language, and runtime preferences is unnecessary work for users and makes repeated testing less consistent.

The Mod remains a pure client-side UE4SS Lua plus LogicMod package. Settings must not introduce a runtime DLL, service, server component, player identity record, entity snapshot, or coordinate history. A failed or partially written settings record must not prevent the Mod from loading.

## Decision

Persist panel preferences in `PalworldResourceESP/Scripts/user-settings.log` as an append-only sequence of complete `v1` snapshots.

- Every snapshot contains only a fixed whitelist of booleans and bounded integers: runtime/profile/preset, language, level endpoints, maximum distance, visible target limit, top guide, name/level/distance visibility, and gender filter.
- Capture state, players, entities, names, IDs, and coordinates are never persisted.
- Lua loads the last valid complete snapshot and ignores malformed, incomplete, unknown-version, or unknown-field lines.
- Loaded values are clamped to the public UI limits and written once to the passive ModActor before the panel is initialized.
- Runtime changes are coalesced for 750 ms before one complete snapshot is appended. Slider preview changes do not change the ModActor and therefore do not schedule writes.
- Storage failures are logged and retried without disabling ESP behavior.
- The generated settings log is ignored by Git.

## Options Considered

### Option 1: Keep session-only settings

Advantages:
- No file I/O or schema maintenance.

Disadvantages:
- Users must recreate the same configuration after every launch.
- Repeated performance and functional tests start from inconsistent settings.

### Option 2: Overwrite one settings file in place

Advantages:
- Constant file size.
- Simple reading path.

Disadvantages:
- An interrupted write can destroy the only valid configuration.
- Overwrite rollback is weaker than retaining earlier complete records.

### Option 3: Append complete versioned snapshots

Advantages:
- The last earlier valid line survives an interrupted or malformed write.
- Schema versions and strict whitelisting are explicit.
- Writes are reversible and easy to inspect.

Disadvantages:
- The file grows by one short line for each stable settings change.
- A future compaction policy may be needed for unusually long-lived installations.

## Rationale

Option 3 matches the project's defensive and reversible workflow. The configuration volume is small, while preserving the previous valid snapshot is more valuable than minimizing a small local text file. A fixed whitelist also keeps persistence outside the human-player and entity-data boundaries.

## Consequences

Positive effects:
- The panel restores the user's last stable functional configuration and language.
- Rapid slider movement causes neither repeated filtering nor repeated file writes.
- Corrupt trailing lines fail open to the last valid snapshot.

Negative effects:
- Settings storage depends on the Mod directory being writable.
- Append-only history has unbounded theoretical growth.
- Schema changes require a new parser version or an explicit migration.

Follow-up considerations:
- Do not accept this ADR until a real restart proves restoration and normal exit.
- Consider bounded compaction only after actual file growth justifies it.

## Follow-ups

- [x] Add strict parser, normalization, last-valid loading, and append tests.
- [x] Add a 750 ms coalesced runtime save path.
- [ ] Verify settings restoration after a full Palworld restart.
- [ ] Verify a malformed final line falls back to the preceding valid line in the game environment.
