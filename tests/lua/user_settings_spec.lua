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
        gender = 0,
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
        })
        local parsed = assert(user_settings.parse_line(user_settings.serialize(input)))
        helper.equal(parsed.runtime_enabled, false)
        helper.equal(parsed.language_id, 1)
        helper.equal(parsed.level_min, 12)
        helper.equal(parsed.show_name, false)
        helper.equal(parsed.gender, 2)
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
        }))
        helper.equal(normalized.profile_id, 3)
        helper.equal(normalized.language_id, 0)
        helper.equal(normalized.level_min, 100)
        helper.equal(normalized.distance_max, 330)
        helper.equal(normalized.display_limit, 1)
        helper.equal(normalized.gender, 2)
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
        local ok = user_settings.append("memory.log", complete({ show_name = false }), backend)
        helper.truthy(ok)
        helper.truthy(written:match("^v1 "))
        helper.truthy(written:match("show_name=false"))
        helper.truthy(written:match("\n$"))
    end),

    helper.case("settings path stays beside the Lua entrypoint", function()
        helper.equal(
            user_settings.path_for_script("@E:\\Mods\\PalworldResourceESP\\Scripts\\main.lua"),
            "E:\\Mods\\PalworldResourceESP\\Scripts\\user-settings.log"
        )
    end),
}
