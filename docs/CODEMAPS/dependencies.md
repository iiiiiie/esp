<!-- Generated: 2026-07-20 | Files scanned: 72 | Token estimate: ~600 -->

# Dependency Codemap

## Runtime

| Dependency | Status | Purpose |
| --- | --- | --- |
| Palworld Steam PC | Supported target | Game/runtime data source |
| `UE4SS_v3.0.1-1009-gc2ac2464` | Tested pin | Lua runtime, hooks, keybinds and object APIs |
| UE4SS BPModLoader/LogicMods | Required | Loads `/Game/Mods/PalworldResourceESP/ModActor` |
| Server component | None | Mod is pure client-side |
| Runtime DLL | None | Pak contains assets only |

Other UE4SS builds, Game Pass PC and multiplayer servers are not guaranteed.

## Build/Cook

| Dependency | Purpose |
| --- | --- |
| Unreal Engine 5.1.1 | Compile editor plugin, generate assets, Cook and UnrealPak inspection |
| External Palworld Modding Kit | Pal schemas/assets required for LogicMod Cook |
| No-Wwise PMK compatibility variant | Builds this audio-free Mod without Wwise SDK |
| Visual Studio 2022 + MSVC 14.38 | Compiles `ESPBlueprintAutomation` |
| .NET 6 | PMK/Unreal build tooling prerequisite |

Engine, PMK, Wwise files, generated caches and cooked third-party assets stay outside this repository.

## Development/Test

| Dependency | Source | Purpose |
| --- | --- | --- |
| Node.js/npm | Local toolchain | Runs repository test harness |
| `fengari@0.1.5` | `package-lock.json` | Executes Lua tests/runtime stubs |
| `luaparse@0.3.1` | `package-lock.json` | Parses Lua 5.3 sources |
| PowerShell | Windows | Sync, capture and performance tests |
| PresentMon 2.5.1 | Optional external tool | ETW frame-time capture |

There are no environment-variable or network-service runtime dependencies.
