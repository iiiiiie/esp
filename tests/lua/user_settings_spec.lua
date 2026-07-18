local helper = require("test_helper")
local user_settings = require("core.user_settings")

local function complete(overrides)
    local values = {
        runtime_enabled = true,
        profile_id = 2,
        preset_id = 1,
        language_id = 0,
        level_min = 0,
        level_max = 0,
        distance_max = 330,
        display_limit = 64,
        show_top = true,
        show_name = true,
        show_level = true,
        show_distance = true,
        show_iv = false,
        iv_min = 0,
        iv_hp_min = 0,
        iv_attack_min = 0,
        iv_defense_min = 0,
        show_passives = false,
        gender = 0,
        lucky = 0,
        boss = 0,
        collection = 0,
        element_normal = false,
        element_fire = false,
        element_water = false,
        element_leaf = false,
        element_electricity = false,
        element_ice = false,
        element_earth = false,
        element_dark = false,
        element_dragon = false,
        passive_includes = "",
        passive_excludes = "",
        expand_rainbow = false,
        expand_legend = false,
        expand_gold3 = false,
        expand_gold2 = false,
        expand_normal = false,
        expand_negative1 = false,
        expand_negative2 = false,
        expand_negative3 = false,
    }
    for name, value in pairs(overrides or {}) do
        values[name] = value
    end
    return values
end

local function legacy_snapshot(values, version)
    local snapshot = user_settings.serialize(values):gsub("^v10 ", version .. " ")
        :gsub(" collection=%-?%d+", "")
    if version ~= "v9" then
        snapshot = snapshot:gsub(" passive_includes=[^ ]+", "")
            :gsub(" passive_excludes=[^ ]+", "")
            :gsub(" expand_[%a%d_]+=[^ ]+", "")
    end
    if version ~= "v8" and version ~= "v9" then
        snapshot = snapshot:gsub(" iv_hp_min=%-?%d+", "")
            :gsub(" iv_attack_min=%-?%d+", "")
            :gsub(" iv_defense_min=%-?%d+", "")
    end
    return snapshot
end

local function read_backend(lines)
    return {
        open = function(_, mode)
            helper.equal(mode, "r")
            local index = 0
            return {
                lines = function()
                    return function()
                        index = index + 1
                        return lines[index]
                    end
                end,
                close = function() end,
            }
        end,
    }
end

return {
    helper.case("settings round-trip preserves whitelisted scalars", function()
        local input = complete({
            runtime_enabled = false,
            language_id = 1,
            level_min = 12,
            show_name = false,
            gender = 2,
            lucky = 1,
            boss = 2,
            collection = 2,
            element_fire = true,
            element_water = true,
            show_iv = true,
            iv_hp_min = 73,
            iv_attack_min = 64,
            iv_defense_min = 88,
            show_passives = true,
            passive_includes = "|Legend|Rare|",
            passive_excludes = "|PAL_Coward|CraftSpeed_down1|",
            expand_legend = true,
            expand_negative1 = true,
        })
        local parsed = assert(user_settings.parse_line(user_settings.serialize(input)))
        helper.equal(parsed.runtime_enabled, false)
        helper.equal(parsed.language_id, 1)
        helper.equal(parsed.level_min, 12)
        helper.equal(parsed.show_name, false)
        helper.equal(parsed.gender, 2)
        helper.equal(parsed.lucky, 1)
        helper.equal(parsed.boss, 2)
        helper.equal(parsed.collection, 2)
        helper.equal(parsed.element_fire, true)
        helper.equal(parsed.element_water, true)
        helper.equal(parsed.element_normal, false)
        helper.equal(parsed.show_iv, true)
        helper.equal(parsed.iv_hp_min, 73)
        helper.equal(parsed.iv_attack_min, 64)
        helper.equal(parsed.iv_defense_min, 88)
        helper.equal(parsed.show_passives, true)
        helper.equal(parsed.passive_includes, "|Legend|Rare|")
        helper.equal(parsed.passive_excludes, "|PAL_Coward|CraftSpeed_down1|")
        helper.equal(parsed.expand_legend, true)
        helper.equal(parsed.expand_negative1, true)
    end),

    helper.case("settings v9 snapshots default collection filter to all", function()
        local v9 = legacy_snapshot(complete({
            passive_includes = "|Legend|",
            expand_legend = true,
            collection = 2,
        }), "v9")
        local parsed, parse_error, version = user_settings.parse_line(v9)
        helper.equal(parse_error, nil)
        helper.equal(version, "v9")
        helper.equal(parsed.passive_includes, "|Legend|")
        helper.equal(parsed.expand_legend, true)
        helper.equal(parsed.collection, 0)
    end),

    helper.case("settings v8 snapshots default passive selections and category expansion", function()
        local v8 = legacy_snapshot(complete({ iv_hp_min = 61, iv_attack_min = 62, iv_defense_min = 63 }), "v8")
        local parsed, parse_error, version = user_settings.parse_line(v8)
        helper.equal(parse_error, nil)
        helper.equal(version, "v8")
        helper.equal(parsed.passive_includes, "")
        helper.equal(parsed.passive_excludes, "")
        helper.equal(parsed.expand_rainbow, false)
        helper.equal(parsed.expand_negative3, false)
    end),

    helper.case("passive ID normalization is delimiter-safe, unique, and bounded", function()
        local normalized = user_settings.normalize(complete({
            passive_includes = "|Legend|Rare|Legend|PAL_FastRunner|PAL_CraftSpeed1|Fifth|",
            passive_excludes = "|PAL_Coward|CraftSpeed_down1|PAL_Coward|",
        }))
        helper.equal(normalized.passive_includes, "|Legend|Rare|PAL_FastRunner|PAL_CraftSpeed1|")
        helper.equal(normalized.passive_excludes, "|PAL_Coward|CraftSpeed_down1|")
    end),

    helper.case("settings v1 snapshots remain readable with Lucky defaulted to all", function()
        local v1 = legacy_snapshot(complete(), "v1")
            :gsub(" lucky=0", ""):gsub(" boss=0", "")
            :gsub(" element_[%a_]+=false", ""):gsub(" show_iv=false", ""):gsub(" iv_min=0", ""):gsub(" show_passives=false", "")
        local parsed, parse_error, version = user_settings.parse_line(v1)
        helper.equal(parse_error, nil)
        helper.equal(version, "v1")
        helper.equal(parsed.lucky, 0)
        helper.equal(parsed.boss, 0)
        helper.equal(parsed.element_fire, false)
        helper.equal(parsed.show_iv, false)
        helper.equal(parsed.iv_min, 0)
        helper.equal(parsed.iv_hp_min, 0)
        helper.equal(parsed.iv_attack_min, 0)
        helper.equal(parsed.iv_defense_min, 0)
        helper.equal(parsed.show_passives, false)
    end),

    helper.case("settings v2 snapshots remain readable with Boss defaulted to all", function()
        local v2 = legacy_snapshot(complete({ lucky = 2 }), "v2")
            :gsub(" boss=0", ""):gsub(" element_[%a_]+=false", ""):gsub(" show_iv=false", ""):gsub(" iv_min=0", ""):gsub(" show_passives=false", "")
        local parsed, parse_error, version = user_settings.parse_line(v2)
        helper.equal(parse_error, nil)
        helper.equal(version, "v2")
        helper.equal(parsed.lucky, 2)
        helper.equal(parsed.boss, 0)
        helper.equal(parsed.element_dragon, false)
        helper.equal(parsed.show_iv, false)
        helper.equal(parsed.iv_min, 0)
        helper.equal(parsed.show_passives, false)
    end),

    helper.case("settings v3 snapshots remain readable with elements defaulted to all", function()
        local v3 = legacy_snapshot(complete({ boss = 1 }), "v3")
            :gsub(" element_[%a_]+=false", ""):gsub(" show_iv=false", ""):gsub(" iv_min=0", ""):gsub(" show_passives=false", "")
        local parsed, parse_error, version = user_settings.parse_line(v3)
        helper.equal(parse_error, nil)
        helper.equal(version, "v3")
        helper.equal(parsed.boss, 1)
        helper.equal(parsed.element_normal, false)
        helper.equal(parsed.element_fire, false)
        helper.equal(parsed.element_water, false)
        helper.equal(parsed.element_leaf, false)
        helper.equal(parsed.element_electricity, false)
        helper.equal(parsed.element_ice, false)
        helper.equal(parsed.element_earth, false)
        helper.equal(parsed.element_dark, false)
        helper.equal(parsed.element_dragon, false)
        helper.equal(parsed.show_iv, false)
        helper.equal(parsed.iv_min, 0)
        helper.equal(parsed.show_passives, false)
    end),

    helper.case("settings v4 snapshots remain readable with IV display disabled", function()
        local v4 = legacy_snapshot(complete({ show_iv = true }), "v4")
            :gsub(" show_iv=true", ""):gsub(" iv_min=0", ""):gsub(" show_passives=false", "")
        local parsed, parse_error, version = user_settings.parse_line(v4)
        helper.equal(parse_error, nil)
        helper.equal(version, "v4")
        helper.equal(parsed.show_iv, false)
        helper.equal(parsed.element_normal, false)
        helper.equal(parsed.iv_min, 0)
        helper.equal(parsed.show_passives, false)
    end),

    helper.case("settings v5 snapshots remain readable with IV minimum disabled", function()
        local v5 = legacy_snapshot(complete({ show_iv = true, iv_min = 80 }), "v5")
            :gsub(" iv_min=80", ""):gsub(" show_passives=false", "")
        local parsed, parse_error, version = user_settings.parse_line(v5)
        helper.equal(parse_error, nil)
        helper.equal(version, "v5")
        helper.equal(parsed.show_iv, true)
        helper.equal(parsed.iv_min, 0)
        helper.equal(parsed.show_passives, false)
    end),

    helper.case("settings v6 snapshots remain readable with passive display disabled", function()
        local v6 = legacy_snapshot(complete({ iv_min = 80, show_passives = true }), "v6")
            :gsub(" show_passives=true", "")
        local parsed, parse_error, version = user_settings.parse_line(v6)
        helper.equal(parse_error, nil)
        helper.equal(version, "v6")
        helper.equal(parsed.iv_min, 80)
        helper.equal(parsed.iv_hp_min, 80)
        helper.equal(parsed.iv_attack_min, 80)
        helper.equal(parsed.iv_defense_min, 80)
        helper.equal(parsed.show_passives, false)
    end),

    helper.case("settings v7 snapshots migrate one IV minimum into all dimensions", function()
        local v7 = legacy_snapshot(complete({ iv_min = 61, show_passives = true }), "v7")
        local parsed, parse_error, version = user_settings.parse_line(v7)
        helper.equal(parse_error, nil)
        helper.equal(version, "v7")
        helper.equal(parsed.show_passives, true)
        helper.equal(parsed.iv_hp_min, 61)
        helper.equal(parsed.iv_attack_min, 61)
        helper.equal(parsed.iv_defense_min, 61)
    end),

    helper.case("settings parser rejects unknown or incomplete snapshots", function()
        local serialized = user_settings.serialize(complete())
        helper.equal(user_settings.parse_line(serialized .. " player_name=test"), nil)
        helper.equal(user_settings.parse_line("v1 runtime_enabled=true"), nil)
        helper.equal(user_settings.parse_line(serialized:gsub("show_name=true", "show_name=maybe")), nil)
        helper.equal(user_settings.parse_line(serialized:gsub("passive_includes=%-", "passive_includes=|Legend|bad-id|")), nil)
        helper.equal(user_settings.parse_line(serialized:gsub("passive_excludes=%-", "passive_excludes=Legend")), nil)
    end),

    helper.case("settings normalization clamps numeric bounds", function()
        local normalized = user_settings.normalize(complete({
            profile_id = 99,
            language_id = -5,
            level_min = 101,
            distance_max = 999,
            display_limit = 0,
            gender = 9,
            lucky = -4,
            boss = 99,
            collection = 99,
            iv_min = 999,
            iv_hp_min = -9,
            iv_attack_min = 47,
            iv_defense_min = 999,
        }))
        helper.equal(normalized.profile_id, 3)
        helper.equal(normalized.language_id, 0)
        helper.equal(normalized.level_min, 100)
        helper.equal(normalized.distance_max, 330)
        helper.equal(normalized.display_limit, 1)
        helper.equal(normalized.gender, 2)
        helper.equal(normalized.lucky, 0)
        helper.equal(normalized.boss, 2)
        helper.equal(normalized.collection, 2)
        helper.equal(normalized.iv_min, 100)
        helper.equal(normalized.iv_hp_min, 0)
        helper.equal(normalized.iv_attack_min, 47)
        helper.equal(normalized.iv_defense_min, 100)
    end),

    helper.case("settings loader uses the last valid snapshot", function()
        local first = user_settings.serialize(complete({ display_limit = 12 }))
        local last = user_settings.serialize(complete({ display_limit = 77, show_name = false }))
        local loaded = assert(user_settings.load_latest(
            "memory.log",
            read_backend({ first, "invalid line", last })
        ))
        helper.equal(loaded.display_limit, 77)
        helper.equal(loaded.show_name, false)
    end),

    helper.case("settings append writes one complete versioned line", function()
        local written = ""
        local backend = {
            open = function(_, mode)
                helper.equal(mode, "a")
                return {
                    write = function(_, value)
                        written = written .. value
                        return true
                    end,
                    flush = function() end,
                    close = function() end,
                }
            end,
        }
        local ok = user_settings.append("memory.log", complete({
            show_name = false,
            show_iv = true,
            iv_hp_min = 67,
            iv_attack_min = 68,
            iv_defense_min = 69,
            show_passives = true,
            passive_includes = "|Legend|Rare|",
            passive_excludes = "|PAL_Coward|",
            expand_legend = true,
            collection = 1,
        }), backend)
        helper.truthy(ok)
        helper.truthy(written:match("^v10 "))
        helper.truthy(written:match("show_name=false"))
        helper.truthy(written:match("lucky=0"))
        helper.truthy(written:match("boss=0"))
        helper.truthy(written:match("element_normal=false"))
        helper.truthy(written:match("element_dragon=false"))
        helper.truthy(written:match("show_iv=true"))
        helper.truthy(written:match("iv_hp_min=67"))
        helper.truthy(written:match("iv_attack_min=68"))
        helper.truthy(written:match("iv_defense_min=69"))
        helper.truthy(written:match("show_passives=true"))
        helper.truthy(written:match("passive_includes=|Legend|Rare|"))
        helper.truthy(written:match("passive_excludes=|PAL_Coward|"))
        helper.truthy(written:match("expand_legend=true"))
        helper.truthy(written:match("expand_rainbow=false"))
        helper.truthy(written:match("collection=1"))
        helper.truthy(written:match("\n$"))
    end),

    helper.case("settings path stays beside the Lua entrypoint", function()
        helper.equal(
            user_settings.path_for_script("@E:\\Mods\\PalworldResourceESP\\Scripts\\main.lua"),
            "E:\\Mods\\PalworldResourceESP\\Scripts\\user-settings.log"
        )
    end),

    helper.case("settings path resolution prefers a valid debug source", function()
        local search_called = false
        local path, resolution = user_settings.resolve_path(
            "@E:\\Mods\\PalworldResourceESP\\Scripts\\main.lua",
            "ignored",
            function()
                search_called = true
            end
        )
        helper.equal(path, "E:\\Mods\\PalworldResourceESP\\Scripts\\user-settings.log")
        helper.equal(resolution, "debug_source")
        helper.equal(search_called, false)
    end),

    helper.case("settings path resolution falls back to the loaded config module", function()
        local path, resolution = user_settings.resolve_path(
            "=[C]",
            "E:\\Mods\\PalworldResourceESP\\Scripts\\?.lua",
            function(module_name, package_path)
                helper.equal(module_name, "config")
                helper.truthy(package_path:match("Scripts"))
                return "E:\\Mods\\PalworldResourceESP\\Scripts\\config.lua"
            end
        )
        helper.equal(path, "E:\\Mods\\PalworldResourceESP\\Scripts\\user-settings.log")
        helper.equal(resolution, "package_search")
    end),

    helper.case("settings path resolution reports unavailable storage", function()
        local path, resolution = user_settings.resolve_path("=[C]", nil, nil)
        helper.equal(path, nil)
        helper.equal(resolution, "package_path_missing")

        path, resolution = user_settings.resolve_path("=[C]", "?.lua", function()
            return nil, "not found"
        end)
        helper.equal(path, nil)
        helper.equal(resolution, "package_search_failed")
    end),
}
