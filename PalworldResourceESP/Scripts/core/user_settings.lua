local user_settings = {}

local VERSION = "v2"

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

local fields = {}
for _, field in ipairs(v1_fields) do
    fields[#fields + 1] = field
end
fields[#fields + 1] = { name = "lucky", kind = "integer", min = 0, max = 2, default = 0 }

local schemas = {
    v1 = v1_fields,
    v2 = fields,
}

local fields_by_version = {}
for version, schema_fields in pairs(schemas) do
    local by_name = {}
    for _, field in ipairs(schema_fields) do
        by_name[field.name] = field
    end
    fields_by_version[version] = by_name
end

local function normalize_value(field, value)
    if field.kind == "boolean" then
        if type(value) == "boolean" then
            return value
        end
        return field.default
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
        else
            if not raw_value:match("^%-?%d+$") then
                return nil, "invalid_integer"
            end
            parsed[name] = tonumber(raw_value)
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
    return normalized, nil, version
end

function user_settings.serialize(values)
    local normalized = user_settings.normalize(values)
    local tokens = { VERSION }
    for _, field in ipairs(fields) do
        tokens[#tokens + 1] = string.format("%s=%s", field.name, tostring(normalized[field.name]))
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

return user_settings
