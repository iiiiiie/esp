local filter_engine = {}

local function values_set(values)
    local result = {}
    if type(values) ~= "table" then
        return result
    end
    for _, value in ipairs(values) do
        result[value] = true
    end
    return result
end

local function selected_contains(condition, value)
    if type(condition.values) == "table" then
        return values_set(condition.values)[value] == true
    end
    return value == condition.value
end

local function match_list(value, condition)
    if type(value) ~= "table" then
        return false, "field_value_not_list"
    end

    local selected = values_set(condition.values)
    local present = values_set(value)
    if condition.mode == "any" then
        for selected_value in pairs(selected) do
            if present[selected_value] then
                return true
            end
        end
        return false, "list_any_not_matched"
    end
    if condition.mode == "all" then
        for selected_value in pairs(selected) do
            if not present[selected_value] then
                return false, "list_all_not_matched"
            end
        end
        return true
    end
    if condition.mode == "exclude" then
        for selected_value in pairs(selected) do
            if present[selected_value] then
                return false, "list_excluded_value_present"
            end
        end
        return true
    end
    return false, "filter_mode_invalid"
end

local function match_condition(cell, condition)
    if type(cell) ~= "table" or cell.state ~= "known" then
        return false, "field_not_known"
    end

    local mode = condition.mode or "exact"
    local value = cell.value
    if mode == "exact" then
        if selected_contains(condition, value) then
            return true
        end
        return false, "exact_not_matched"
    end
    if mode == "range" then
        if type(value) ~= "number" then
            return false, "field_value_not_number"
        end
        if condition.min ~= nil and value < condition.min then
            return false, "range_below_min"
        end
        if condition.max ~= nil and value > condition.max then
            return false, "range_above_max"
        end
        return true
    end
    if mode == "boolean" then
        if type(value) ~= "boolean" or type(condition.value) ~= "boolean" then
            return false, "field_value_not_boolean"
        end
        if value == condition.value then
            return true
        end
        return false, "boolean_not_matched"
    end
    if mode == "any" or mode == "all" or mode == "exclude" then
        return match_list(value, condition)
    end
    return false, "filter_mode_invalid"
end

local function sorted_keys(values)
    local keys = {}
    for key in pairs(values or {}) do
        keys[#keys + 1] = key
    end
    table.sort(keys)
    return keys
end

function filter_engine.match(record, filters)
    filters = filters or {}

    if type(filters.kinds) == "table" and #filters.kinds > 0 then
        local allowed = values_set(filters.kinds)
        if not allowed[record.kind] then
            return false, "kind_not_included"
        end
    end

    for _, field_name in ipairs(sorted_keys(filters.fields)) do
        local condition = filters.fields[field_name]
        if condition ~= nil and condition.active ~= false then
            local matched, reason = match_condition(record.fields[field_name], condition)
            if not matched then
                return false, field_name .. ":" .. reason
            end
        end
    end

    return true
end


function filter_engine.filter(records, filters)
    local matched = {}
    local rejected = {}
    for _, record in ipairs(records or {}) do
        local accepted, reason = filter_engine.match(record, filters)
        if accepted then
            matched[#matched + 1] = record
        else
            rejected[reason] = (rejected[reason] or 0) + 1
        end
    end
    return matched, rejected
end

local function known_distance(record)
    local cell = record.fields and record.fields.distance_m
    if type(cell) == "table" and cell.state == "known" and type(cell.value) == "number" then
        return cell.value
    end
    return nil
end

function filter_engine.order(records)
    local ordered = {}
    for index, record in ipairs(records or {}) do
        ordered[index] = record
    end
    table.sort(ordered, function(first, second)
        local first_distance = known_distance(first)
        local second_distance = known_distance(second)
        if first_distance ~= nil and second_distance ~= nil and first_distance ~= second_distance then
            return first_distance < second_distance
        end
        if first_distance ~= nil and second_distance == nil then
            return true
        end
        if first_distance == nil and second_distance ~= nil then
            return false
        end
        return first.ordinal < second.ordinal
    end)
    return ordered
end

function filter_engine.budget(records, limit)
    limit = math.max(0, math.floor(tonumber(limit) or 0))
    local displayed = {}
    for index = 1, math.min(#records, limit) do
        displayed[index] = records[index]
    end
    return displayed, math.max(0, #records - #displayed)
end

return filter_engine
