local user_settings = {}

local VERSION = "v10"

local v1_fields = {
    { name = "runtime_enabled", kind = "boolean", default = true },
    { name = "profile_id", kind = "integer", min = 0, max = 3, default = 2 },
    { name = "preset_id", kind = "integer", min = 0, max = 2, default = 1 },
    { name = "language_id", kind = "integer", min = 0, max = 1, default = 0 },
    { name = "level_min", kind = "integer", min = 0, max = 100, default = 0 },
    { name = "level_max", kind = "integer", min = 0, max = 100, default = 0 },
    { name = "distance_max", kind = "integer", min = 0, max = 330, default = 330 },
    { name = "display_limit", kind = "integer", min = 1, max = 100, default = 64 },
    { name = "show_top", kind = "boolean", default = true },
    { name = "show_name", kind = "boolean", default = true },
    { name = "show_level", kind = "boolean", default = true },
    { name = "show_distance", kind = "boolean", default = true },
    { name = "gender", kind = "integer", min = 0, max = 2, default = 0 },
}

local v2_fields = {}
for _, field in ipairs(v1_fields) do
    v2_fields[#v2_fields + 1] = field
end
v2_fields[#v2_fields + 1] = { name = "lucky", kind = "integer", min = 0, max = 2, default = 0 }

local v3_fields = {}
for _, field in ipairs(v2_fields) do
    v3_fields[#v3_fields + 1] = field
end
v3_fields[#v3_fields + 1] = { name = "boss", kind = "integer", min = 0, max = 2, default = 0 }

local v4_fields = {}
for _, field in ipairs(v3_fields) do
    v4_fields[#v4_fields + 1] = field
end
for _, name in ipairs({
    "element_normal", "element_fire", "element_water", "element_leaf", "element_electricity",
    "element_ice", "element_earth", "element_dark", "element_dragon",
}) do
    v4_fields[#v4_fields + 1] = { name = name, kind = "boolean", default = false }
end

local v5_fields = {}
for _, field in ipairs(v4_fields) do
    v5_fields[#v5_fields + 1] = field
end
v5_fields[#v5_fields + 1] = { name = "show_iv", kind = "boolean", default = false }

local v6_fields = {}
for _, field in ipairs(v5_fields) do
    v6_fields[#v6_fields + 1] = field
end
v6_fields[#v6_fields + 1] = { name = "iv_min", kind = "integer", min = 0, max = 100, default = 0 }

local v7_fields = {}
for _, field in ipairs(v6_fields) do
    v7_fields[#v7_fields + 1] = field
end
v7_fields[#v7_fields + 1] = { name = "show_passives", kind = "boolean", default = false }

local v8_fields = {}
for _, field in ipairs(v7_fields) do
    v8_fields[#v8_fields + 1] = field
end
v8_fields[#v8_fields + 1] = { name = "iv_hp_min", kind = "integer", min = 0, max = 100, default = 0 }
v8_fields[#v8_fields + 1] = { name = "iv_attack_min", kind = "integer", min = 0, max = 100, default = 0 }
v8_fields[#v8_fields + 1] = { name = "iv_defense_min", kind = "integer", min = 0, max = 100, default = 0 }

local fields = {}
for _, field in ipairs(v8_fields) do
    fields[#fields + 1] = field
end
fields[#fields + 1] = { name = "passive_includes", kind = "passive_ids", max_count = 4, default = "" }
fields[#fields + 1] = { name = "passive_excludes", kind = "passive_ids", max_count = 256, default = "" }
for _, name in ipairs({
    "expand_rainbow", "expand_legend", "expand_gold3", "expand_gold2",
    "expand_normal", "expand_negative1", "expand_negative2", "expand_negative3",
}) do
    fields[#fields + 1] = { name = name, kind = "boolean", default = false }
end

local v9_fields = {}
for _, field in ipairs(fields) do
    v9_fields[#v9_fields + 1] = field
end
fields[#fields + 1] = { name = "collection", kind = "integer", min = 0, max = 2, default = 0 }

local schemas = {
    v1 = v1_fields,
    v2 = v2_fields,
    v3 = v3_fields,
    v4 = v4_fields,
    v5 = v5_fields,
    v6 = v6_fields,
    v7 = v7_fields,
    v8 = v8_fields,
    v9 = v9_fields,
    v10 = fields,
}

local fields_by_version = {}
for version, schema_fields in pairs(schemas) do
    local by_name = {}
    for _, field in ipairs(schema_fields) do
        by_name[field.name] = field
    end
    fields_by_version[version] = by_name
end

local function normalize_passive_ids(value, max_count, strict)
    if value == nil or value == "" or value == "-" then
        return ""
    end
    if type(value) ~= "string" or value:sub(1, 1) ~= "|" or value:sub(-1) ~= "|" then
        if strict then
            return nil
        end
        return ""
    end

    local ids = {}
    local seen = {}
    for id in value:gmatch("[^|]+") do
        if not id:match("^[A-Za-z0-9_]+$") then
            if strict then
                return nil
            end
            return ""
        end
        if not seen[id] and #ids < max_count then
            seen[id] = true
            ids[#ids + 1] = id
        end
    end
    if #ids == 0 then
        if strict then
            return nil
        end
        return ""
    end
    return "|" .. table.concat(ids, "|") .. "|"
end

local function normalize_value(field, value)
    if field.kind == "boolean" then
        if type(value) == "boolean" then
            return value
        end
        return field.default
    end
    if field.kind == "passive_ids" then
        return normalize_passive_ids(value, field.max_count, false)
    end

    value = tonumber(value)
    if value == nil or value ~= value or value == math.huge or value == -math.huge then
        return field.default
    end
    value = math.floor(value)
    return math.max(field.min, math.min(field.max, value))
end

function user_settings.normalize(values)
    local normalized = {}
    for _, field in ipairs(fields) do
        normalized[field.name] = normalize_value(field, values and values[field.name])
    end
    return normalized
end

function user_settings.parse_line(line)
    if type(line) ~= "string" then
        return nil, "line_not_string"
    end
    local version, payload = line:match("^%s*(v%d+)%s+(..-)%s*$")
    local schema_fields = schemas[version]
    local fields_by_name = fields_by_version[version]
    if schema_fields == nil or payload == nil or payload == "" then
        return nil, "unsupported_version"
    end

    local parsed = {}
    local count = 0
    for token in payload:gmatch("%S+") do
        local name, raw_value = token:match("^([%a_][%w_]*)=([^=]+)$")
        local field = name and fields_by_name[name] or nil
        if not field or parsed[name] ~= nil then
            return nil, "invalid_field"
        end

        if field.kind == "boolean" then
            if raw_value == "true" then
                parsed[name] = true
            elseif raw_value == "false" then
                parsed[name] = false
            else
                return nil, "invalid_boolean"
            end
        elseif field.kind == "integer" then
            if not raw_value:match("^%-?%d+$") then
                return nil, "invalid_integer"
            end
            parsed[name] = tonumber(raw_value)
        else
            parsed[name] = normalize_passive_ids(raw_value, field.max_count, true)
            if parsed[name] == nil then
                return nil, "invalid_passive_ids"
            end
        end
        count = count + 1
    end

    if count ~= #schema_fields then
        return nil, "incomplete_snapshot"
    end
    local normalized = {}
    for _, field in ipairs(schema_fields) do
        normalized[field.name] = normalize_value(field, parsed[field.name])
    end
    if version == "v1" then
        normalized.lucky = 0
    end
    if version ~= "v3" and version ~= "v4" and version ~= "v5" and version ~= "v6" and version ~= "v7" and version ~= "v8" and version ~= "v9" and version ~= "v10" then
        normalized.boss = 0
    end
    if version ~= "v4" and version ~= "v5" and version ~= "v6" and version ~= "v7" and version ~= "v8" and version ~= "v9" and version ~= "v10" then
        normalized.element_normal = false
        normalized.element_fire = false
        normalized.element_water = false
        normalized.element_leaf = false
        normalized.element_electricity = false
        normalized.element_ice = false
        normalized.element_earth = false
        normalized.element_dark = false
        normalized.element_dragon = false
    end
    if version ~= "v5" and version ~= "v6" and version ~= "v7" and version ~= "v8" and version ~= "v9" and version ~= "v10" then
        normalized.show_iv = false
    end
    if version ~= "v6" and version ~= "v7" and version ~= "v8" and version ~= "v9" and version ~= "v10" then
        normalized.iv_min = 0
    end
    if version ~= "v7" and version ~= "v8" and version ~= "v9" and version ~= "v10" then
        normalized.show_passives = false
    end
    if version ~= "v8" and version ~= "v9" and version ~= "v10" then
        normalized.iv_hp_min = normalized.iv_min or 0
        normalized.iv_attack_min = normalized.iv_min or 0
        normalized.iv_defense_min = normalized.iv_min or 0
    end
    if version ~= "v9" and version ~= "v10" then
        normalized.passive_includes = ""
        normalized.passive_excludes = ""
        normalized.expand_rainbow = false
        normalized.expand_legend = false
        normalized.expand_gold3 = false
        normalized.expand_gold2 = false
        normalized.expand_normal = false
        normalized.expand_negative1 = false
        normalized.expand_negative2 = false
        normalized.expand_negative3 = false
    end
    if version ~= "v10" then
        normalized.collection = 0
    end
    return normalized, nil, version
end

function user_settings.serialize(values)
    local normalized = user_settings.normalize(values)
    local tokens = { VERSION }
    for _, field in ipairs(fields) do
        local value = normalized[field.name]
        if field.kind == "passive_ids" and value == "" then
            value = "-"
        end
        tokens[#tokens + 1] = string.format("%s=%s", field.name, tostring(value))
    end
    return table.concat(tokens, " ")
end

function user_settings.load_latest(path, io_backend)
    if type(path) ~= "string" or path == "" or type(io_backend) ~= "table"
        or type(io_backend.open) ~= "function" then
        return nil, "storage_unavailable"
    end

    local file, open_error = io_backend.open(path, "r")
    if file == nil then
        return nil, open_error or "not_found"
    end

    local latest = nil
    local latest_version = nil
    local ok, read_error = pcall(function()
        for line in file:lines() do
            local parsed, _, version = user_settings.parse_line(line)
            if parsed ~= nil then
                latest = parsed
                latest_version = version
            end
        end
    end)
    pcall(function()
        file:close()
    end)
    if not ok then
        return nil, tostring(read_error)
    end
    if latest == nil then
        return nil, "no_valid_snapshot"
    end
    return latest, nil, latest_version
end

function user_settings.append(path, values, io_backend)
    if type(path) ~= "string" or path == "" or type(io_backend) ~= "table"
        or type(io_backend.open) ~= "function" then
        return false, "storage_unavailable"
    end

    local file, open_error = io_backend.open(path, "a")
    if file == nil then
        return false, open_error or "open_failed"
    end
    local line = user_settings.serialize(values) .. "\n"
    local ok, write_result = pcall(function()
        local result, write_error = file:write(line)
        if result == nil then
            error(write_error or "write_failed")
        end
        if type(file.flush) == "function" then
            file:flush()
        end
    end)
    pcall(function()
        file:close()
    end)
    if not ok then
        return false, tostring(write_result)
    end
    return true, line
end

function user_settings.path_for_script(source)
    if type(source) ~= "string" then
        return nil
    end
    source = source:gsub("^@", "")
    local directory = source:match("^(.*)[/\\][^/\\]+$")
    if directory == nil or directory == "" then
        return nil
    end
    local separator = source:find("\\", 1, true) and "\\" or "/"
    return directory .. separator .. "user-settings.log"
end

function user_settings.resolve_path(script_source, package_path, searchpath)
    local path = user_settings.path_for_script(script_source)
    if path ~= nil then
        return path, "debug_source"
    end

    if type(package_path) ~= "string" or package_path == "" then
        return nil, "package_path_missing"
    end
    if type(searchpath) ~= "function" then
        return nil, "package_search_unavailable"
    end

    local ok, config_path = pcall(searchpath, "config", package_path)
    if not ok or type(config_path) ~= "string" or config_path == "" then
        return nil, "package_search_failed"
    end

    path = user_settings.path_for_script(config_path)
    if path == nil then
        return nil, "package_result_invalid"
    end
    return path, "package_search"
end

return user_settings
