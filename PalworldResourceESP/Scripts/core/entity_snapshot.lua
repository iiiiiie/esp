local entity_snapshot = {}

local function is_finite_number(value)
    return type(value) == "number" and value == value and value ~= math.huge and value ~= -math.huge
end

local function is_safe_scalar(value)
    local value_type = type(value)
    if value_type == "number" then
        return is_finite_number(value)
    end
    return value_type == "string" or value_type == "boolean"
end

local function copy_safe_value(value)
    if is_safe_scalar(value) then
        return value
    end
    if type(value) ~= "table" then
        return nil, "value_not_scalar_or_list"
    end

    local length = #value
    local count = 0
    local result = {}
    for key, item in pairs(value) do
        count = count + 1
        if type(key) ~= "number" or key < 1 or key > length or key % 1 ~= 0 then
            return nil, "list_not_contiguous"
        end
        if not is_safe_scalar(item) then
            return nil, "list_item_not_scalar"
        end
        result[key] = item
    end
    if count ~= length then
        return nil, "list_not_contiguous"
    end
    return result
end

local function require_text(value, name)
    assert(type(value) == "string" and value ~= "", name .. " must be a non-empty string")
    return value
end

function entity_snapshot.known(value, source)
    local safe_value, reason = copy_safe_value(value)
    assert(safe_value ~= nil, reason)
    return {
        state = "known",
        value = safe_value,
        source = require_text(source, "known source"),
    }
end

function entity_snapshot.bridge(reason)
    return {
        state = "bridge",
        reason = require_text(reason, "bridge reason"),
    }
end

function entity_snapshot.unavailable(reason)
    return {
        state = "unavailable",
        reason = require_text(reason, "unavailable reason"),
    }
end

function entity_snapshot.validate_cell(cell)
    if type(cell) ~= "table" then
        return false, "cell_not_table"
    end
    if cell.state == "known" then
        local safe_value = copy_safe_value(cell.value)
        if safe_value == nil then
            return false, "known_value_not_safe"
        end
        if type(cell.source) ~= "string" or cell.source == "" then
            return false, "known_source_missing"
        end
        return true
    end
    if cell.state == "bridge" or cell.state == "unavailable" then
        if type(cell.reason) ~= "string" or cell.reason == "" then
            return false, "field_reason_missing"
        end
        return true
    end
    return false, "field_state_invalid"
end

local function validate_fields(fields)
    if type(fields) ~= "table" then
        return false, "fields_not_table"
    end
    for name, cell in pairs(fields) do
        if type(name) ~= "string" or name == "" then
            return false, "field_name_invalid"
        end
        local valid, reason = entity_snapshot.validate_cell(cell)
        if not valid then
            return false, name .. ":" .. reason
        end
    end
    return true
end

local function empty_generation(session_id, generation_id, source, created_at)
    return {
        session_id = session_id,
        generation_id = generation_id,
        source = source,
        created_at = created_at,
        records = {},
        lookup = {},
    }
end

function entity_snapshot.new_store()
    return {
        session_id = 0,
        generation_id = 0,
        current = empty_generation(0, 0, "initial", 0),
    }
end

function entity_snapshot.reset_session(store, session_id)
    assert(type(store) == "table", "store must be a table")
    assert(type(session_id) == "number", "session_id must be a number")
    store.session_id = session_id
    store.generation_id = 0
    store.current = empty_generation(session_id, 0, "session_reset", 0)
    return store.current
end

function entity_snapshot.begin_generation(store, source, created_at)
    assert(type(store) == "table", "store must be a table")
    return empty_generation(
        store.session_id,
        store.generation_id + 1,
        require_text(source, "generation source"),
        created_at
    )
end

function entity_snapshot.add(generation, entity)
    assert(type(generation) == "table", "generation must be a table")
    assert(type(entity) == "table", "entity must be a table")
    assert(entity.actor ~= nil, "entity actor is required")
    require_text(entity.kind, "entity kind")

    if generation.lookup[entity.actor] ~= nil then
        return false, generation.lookup[entity.actor]
    end

    local valid_fields, field_error = validate_fields(entity.fields)
    assert(valid_fields, field_error)

    local record = {
        kind = entity.kind,
        actor = entity.actor,
        session_id = generation.session_id,
        generation_id = generation.generation_id,
        ordinal = #generation.records + 1,
        source = entity.source or generation.source,
        accepted_at = entity.accepted_at or generation.created_at,
        fields = entity.fields,
    }
    generation.records[#generation.records + 1] = record
    generation.lookup[entity.actor] = record
    return true, record
end

function entity_snapshot.replace(store, generation)
    assert(type(store) == "table", "store must be a table")
    assert(type(generation) == "table", "generation must be a table")
    assert(generation.session_id == store.session_id, "generation belongs to another session")
    assert(generation.generation_id == store.generation_id + 1, "generation is not the next generation")
    store.current = generation
    store.generation_id = generation.generation_id
    return generation
end

function entity_snapshot.clear(store, source)
    assert(type(store) == "table", "store must be a table")
    store.current = empty_generation(
        store.session_id,
        store.generation_id,
        source or "clear",
        0
    )
    return store.current
end

function entity_snapshot.current(store)
    return store.current
end

return entity_snapshot
