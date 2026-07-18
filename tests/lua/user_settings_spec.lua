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
        gender = 0,
        lucky = 0,
        boss = 0,
        element_normal = false,
        element_fire = false,
        element_water = false,
        element_leaf = false,
        element_electricity = false,
        element_ice = false,
        element_earth = false,
        element_dark = false,
        element_dragon = false,
    }
    for name, value in pairs(overrides or {}) do
        values[name] = value
    end
    return values
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
            element_fire = true,
            element_water = true,
            show_iv = true,
        })
        local parsed = assert(user_settings.parse_line(user_settings.serialize(input)))
        helper.equal(parsed.runtime_enabled, false)
        helper.equal(parsed.language_id, 1)
        helper.equal(parsed.level_min, 12)
        helper.equal(parsed.show_name, false)
        helper.equal(parsed.gender, 2)
        helper.equal(parsed.lucky, 1)
        helper.equal(parsed.boss, 2)
        helper.equal(parsed.element_fire, true)
        helper.equal(parsed.element_water, true)
        helper.equal(parsed.element_normal, false)
        helper.equal(parsed.show_iv, true)
    end),

    helper.case("settings v1 snapshots remain readable with Lucky defaulted to all", function()
        local v1 = user_settings.serialize(complete()):gsub("^v5 ", "v1 ")
            :gsub(" lucky=0", ""):gsub(" boss=0", "")
            :gsub(" element_[%a_]+=false", ""):gsub(" show_iv=false", "")
        local parsed, parse_error, version = user_settings.parse_line(v1)
        helper.equal(parse_error, nil)
        helper.equal(version, "v1")
        helper.equal(parsed.lucky, 0)
        helper.equal(parsed.boss, 0)
        helper.equal(parsed.element_fire, false)
        helper.equal(parsed.show_iv, false)
    end),

    helper.case("settings v2 snapshots remain readable with Boss defaulted to all", function()
        local v2 = user_settings.serialize(complete({ lucky = 2 })):gsub("^v5 ", "v2 ")
            :gsub(" boss=0", ""):gsub(" element_[%a_]+=false", ""):gsub(" show_iv=false", "")
        local parsed, parse_error, version = user_settings.parse_line(v2)
        helper.equal(parse_error, nil)
        helper.equal(version, "v2")
        helper.equal(parsed.lucky, 2)
        helper.equal(parsed.boss, 0)
        helper.equal(parsed.element_dragon, false)
        helper.equal(parsed.show_iv, false)
    end),

    helper.case("settings v3 snapshots remain readable with elements defaulted to all", function()
        local v3 = user_settings.serialize(complete({ boss = 1 })):gsub("^v5 ", "v3 ")
            :gsub(" element_[%a_]+=false", ""):gsub(" show_iv=false", "")
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
    end),

    helper.case("settings v4 snapshots remain readable with IV display disabled", function()
        local v4 = user_settings.serialize(complete({ show_iv = true })):gsub("^v5 ", "v4 ")
            :gsub(" show_iv=true", "")
        local parsed, parse_error, version = user_settings.parse_line(v4)
        helper.equal(parse_error, nil)
        helper.equal(version, "v4")
        helper.equal(parsed.show_iv, false)
        helper.equal(parsed.element_normal, false)
    end),

    helper.case("settings parser rejects unknown or incomplete snapshots", function()
        local serialized = user_settings.serialize(complete())
        helper.equal(user_settings.parse_line(serialized .. " player_name=test"), nil)
        helper.equal(user_settings.parse_line("v1 runtime_enabled=true"), nil)
        helper.equal(user_settings.parse_line(serialized:gsub("show_name=true", "show_name=maybe")), nil)
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
        }))
        helper.equal(normalized.profile_id, 3)
        helper.equal(normalized.language_id, 0)
        helper.equal(normalized.level_min, 100)
        helper.equal(normalized.distance_max, 330)
        helper.equal(normalized.display_limit, 1)
        helper.equal(normalized.gender, 2)
        helper.equal(normalized.lucky, 0)
        helper.equal(normalized.boss, 2)
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
        local ok = user_settings.append("memory.log", complete({ show_name = false, show_iv = true }), backend)
        helper.truthy(ok)
        helper.truthy(written:match("^v5 "))
        helper.truthy(written:match("show_name=false"))
        helper.truthy(written:match("lucky=0"))
        helper.truthy(written:match("boss=0"))
        helper.truthy(written:match("element_normal=false"))
        helper.truthy(written:match("element_dragon=false"))
        helper.truthy(written:match("show_iv=true"))
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
