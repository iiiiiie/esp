<!-- Generated: 2026-07-20 | Files scanned: 72 | Token estimate: ~720 -->

# Data Codemap

## Entity Record

`entity_snapshot.lua` stores one generation-scoped record per admitted actor:

```text
session_id, generation_id, ordinal, source, accepted_at, actor
fields: is_wild, level, distance_m, species, gender, passive_skills,
        iv_hp, iv_attack, iv_melee, iv_defense, lucky, alpha_boss,
        elements, capture_count
```

Each field is a cell:

- `known(value, source)`: safe scalar/list available to Lua.
- `bridge(reason)`: value belongs to a typed Blueprint provider.
- `unavailable(reason)`: provider is intentionally absent.

Unknown/bridge values fail closed when a corresponding Lua filter is active.

## Settings Schema

Source: `core/user_settings.lua`. Current writes use `v12`; strict readers support `v1..v12`.

| Version | Added state |
| --- | --- |
| `v1..v8` | Runtime, language, ranges, display flags, gender/Lucky/Boss/elements, IV/passive controls |
| `v9` | Passive include/exclude IDs and 8 category expansion flags |
| `v10` | Collection-completion filter |
| `v11` | Selected Pal species IDs |
| `v12` | Last primary and nested filter pages |

Format is one append-only complete snapshot per stable change. The last valid line wins. Writes are coalesced for 750 ms. Storage path is `Scripts/user-settings.log`, resolved from script source or `package.searchpath`.

## Runtime Game Data

- Pal species catalog: current `DT_PalMonsterParameter` rows.
- Passive catalog: current passive manager rows and localized descriptions.
- Partner effects: game formatter `GetFormatedFirstActivatedInfoTextFixedRank`.
- Icons and hover details remain in Blueprint; no versioned game-data snapshot is committed.

## Privacy Boundary

Never persist players, platform IDs, actor snapshots, coordinates, catalog rows, hover data or capture sessions. `user-settings.log` is ignored by Git. Human-player objects are rejected before record creation.
