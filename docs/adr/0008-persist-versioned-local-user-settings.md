# ADR-0008: Persist Versioned Local User Settings

## Status

Proposed

## Date

2026-07-17

## Context

The in-game panel currently resets to Blueprint defaults whenever Palworld starts. Re-entering display, filter, language, and runtime preferences is unnecessary work for users and makes repeated testing less consistent.

The Mod remains a pure client-side UE4SS Lua plus LogicMod package. Settings must not introduce a runtime DLL, service, server component, player identity record, entity snapshot, or coordinate history. A failed or partially written settings record must not prevent the Mod from loading.

The first in-game deployment exposed an additional runtime constraint: UE4SS can omit a usable script filename from `debug.getinfo()`. The original implementation depended exclusively on that value, logged `USER_SETTINGS_UNAVAILABLE reason=storage_path`, and therefore never attempted a read or append.

## Decision

Persist panel preferences in `PalworldResourceESP/Scripts/user-settings.log` as an append-only sequence of complete versioned snapshots. New writes use `v10`; the reader accepts strict complete `v1` through `v10` records.

- Every snapshot contains a fixed whitelist of booleans, bounded integers, and two bounded passive-ID strings. The scalar fields cover runtime/profile/preset, language, level endpoints, maximum distance, visible target limit, top guide, name/level/distance/IV/passive visibility, legacy IV minimum, HP/attack/defense IV minima, gender filter, Lucky filter, Boss filter, collection-completion filter, nine element toggles, and eight passive-category expansion states.
- Blueprint remains the owner of the include/exclude `TArray<FName>` values. It mirrors mutations into delimiter-safe `|SkillId|SkillId|` strings on the passive ModActor. Lua accepts only ASCII `[A-Za-z0-9_]+` IDs, deduplicates them, caps includes at four and excludes at 256, serializes an empty list as `-`, and never probes the Blueprint arrays.
- A valid `v1` snapshot migrates in memory by defaulting Lucky and Boss to `all`; `v2` defaults Boss to `all`; `v1`, `v2`, and `v3` default all element toggles to unselected; `v1` through `v4` default IV display to disabled; `v1` through `v5` default the legacy IV minimum to zero; and `v1` through `v6` default passive-skill display to disabled. For `v1` through `v7`, the legacy IV minimum migrates into all three dimensional minima. `v1` through `v8` default passive selections to empty and all passive categories to collapsed. `v1` through `v9` default collection completion to `all`. Old records are never rewritten in place. The next stable user change appends a complete `v10` snapshot.
- Capture state, players, entities, names, IDs, and coordinates are never persisted.
- Lua loads the last valid complete snapshot and ignores malformed, incomplete, unknown-version, or unknown-field lines.
- Loaded values are clamped to the public UI limits and written once to the passive ModActor before the panel is initialized. Lua then invokes `PalworldResourceESP_ApplyPersistedPanelState`; Blueprint parses the two mirrors, rebuilds both `TArray<FName>` values, and increments the passive filter revision.
- UE4SS exposes a Blueprint `FString` property as userdata on the validated build. The reader therefore accepts a plain Lua string or, only for these two project-owned properties, userdata that reports type `FString` and returns a Lua string from `ToString()`. Arbitrary userdata conversion remains prohibited.
- Runtime changes update the normalized in-memory snapshot immediately and are coalesced for 750 ms before one complete snapshot is appended. A new ModActor created during a save transition therefore receives the latest stable panel state even before the debounced disk append completes. Slider preview changes do not change the ModActor and therefore do not schedule writes.
- Storage failures are logged and retried without disabling ESP behavior.
- Resolve the settings location from a valid `debug.getinfo()` source when available. Otherwise use `package.searchpath("config", package.path)` to locate the already loaded sibling `config.lua`, then place `user-settings.log` in that directory. If neither source resolves, log the specific resolution failure and continue with defaults.
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
- Supporting older strict schemas adds a small compatibility surface to the parser.
- Passive selections depend on stable game-internal skill IDs; removed or renamed IDs are discarded instead of guessed.

Follow-up considerations:
- Do not accept this ADR until a real restart proves restoration and normal exit.
- Consider bounded compaction only after actual file growth justifies it.

## Follow-ups

- [x] Add strict parser, normalization, last-valid loading, and append tests.
- [x] Add a 750 ms coalesced runtime save path.
- [x] Add strict `v1` read compatibility and `v2` writes for the Lucky selector.
- [x] Add strict `v2` read compatibility and `v3` writes for the Boss selector.
- [x] Add strict `v3` read compatibility and `v4` writes for the multi-select element toggles.
- [x] Add strict `v4` read compatibility and `v5` writes for the IV visibility toggle.
- [x] Add strict `v5` read compatibility and `v6` writes for the IV minimum.
- [x] Add strict `v6` read compatibility and `v7` writes for the passive-skill visibility toggle.
- [x] Add strict `v7` read compatibility and `v8` writes for independent HP/attack/defense IV minima.
- [x] Add strict `v8` read compatibility and `v9` writes for passive include/exclude IDs and eight category expansion states.
- [x] Add strict `v9` read compatibility and `v10` writes for the three-state local collection-completion filter.
- [x] Rebuild Blueprint-owned passive arrays from persisted mirrors without moving entity, player, or coordinate data into Lua.
- [x] Add and runtime-stub test the `package.searchpath` fallback required by UE4SS when the debug source is unavailable.
- [ ] Verify settings restoration after a full Palworld restart.
- [ ] Verify a malformed final line falls back to the preceding valid line in the game environment.
