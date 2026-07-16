local config = require("config")
local adapter_registry = require("core.adapter_registry")
local entity_snapshot = require("core.entity_snapshot")
local filter_engine = require("core.filter_engine")
local pal_adapter = require("adapters.pal")

local MOD_NAME = "PalworldResourceESP"
local BRIDGE_READY_EVENT = "PalworldResourceESP_LuaBridgeReady"
local BRIDGE_METHOD_RESET_SESSION = "PalworldResourceESP_ResetSession"
local BRIDGE_METHOD_SET_TARGET = "PalworldResourceESP_SetTarget"
local BRIDGE_METHOD_CLEAR_TARGET = "PalworldResourceESP_ClearTarget"
local BRIDGE_CLASS_FULL_NAME = "BlueprintGeneratedClass /Game/Mods/PalworldResourceESP/ModActor.ModActor_C"

local CLASS_PATHS = {
    monster = "/Script/Pal.PalMonsterCharacter",
    player = "/Script/Pal.PalPlayerCharacter",
    player_controller = "/Script/Pal.PalPlayerController",
    player_state = "/Script/Pal.PalPlayerState",
}

local FIELD_PROBES = {
    {
        name = "species",
        sources = { "parameter" },
        methods = { "GetCharacterID" },
    },
    {
        name = "individual_id",
        sources = { "parameter" },
        methods = { "GetPalId" },
    },
    {
        name = "level",
        sources = { "parameter" },
        methods = { "GetLevel" },
    },
    {
        name = "gender",
        sources = {},
        note = "UE4SS Lua crashes on both direct enum access and GetGenderType return marshalling; requires Blueprint bridge",
    },
    {
        name = "passive_skills",
        sources = {},
        note = "UE4SS Lua array marshalling is not a safe probe path; requires Blueprint bridge",
    },
    {
        name = "iv_hp",
        sources = {},
        note = "SaveParameterUtility struct access is deferred to the Blueprint bridge",
    },
    {
        name = "iv_attack",
        sources = {},
        note = "SaveParameterUtility struct access is deferred to the Blueprint bridge",
    },
    {
        name = "iv_melee",
        sources = {},
        note = "requires a confirmed game-build field exposed through the Blueprint bridge",
    },
    {
        name = "iv_defense",
        sources = {},
        note = "SaveParameterUtility struct access is deferred to the Blueprint bridge",
    },
    {
        name = "lucky",
        sources = {},
        note = "deferred until an exact runtime-safe accessor is confirmed",
    },
    {
        name = "alpha_boss",
        sources = {},
        note = "deferred until an exact runtime-safe accessor is confirmed",
    },
    {
        name = "elements",
        sources = {},
        note = "requires a confirmed species database or runtime-safe accessor",
    },
    {
        name = "capture_count",
        sources = {},
        note = "requires a separate local player-save collection adapter",
    },
}

local state = {
    classes = {},
    candidates = {},
    candidate_count = 0,
    entity_registry = adapter_registry.new(),
    entity_store = entity_snapshot.new_store(),
    matched_records = {},
    display_records = {},
    adapters_registered = false,
    selected = nil,
    notification_registered = false,
    lifecycle_hooks_registered = false,
    load_map_hooks_registered = false,
    bridge_event_registered = false,
    bridge_discovery_registered = false,
    bridge_probe_actor = nil,
    bridge_actor = nil,
    bridge_session_sent = nil,
    -- __DEPRECATED_20260716__ [reason: retained for rollback; multi-target sync tracks a count]
    bridge_target_actor = nil,
    bridge_target_session = nil,
    bridge_target_count = 0,
    bridge_gender_logged = false,
    bridge_gender_pending_reason = nil,
    draw_hook_registered = false,
    reconcile_started = false,
    reconcile_handle = nil,
    bootstrap_pending = false,
    lifecycle_generation = 0,
    world_transitioning = false,
    gameplay_active = false,
    last_scan_skip_reason = nil,
    pal_accept_logged = false,
    session_index = 0,
    field_path_logged = {},
    rejection_reason_logged = {},
    draw_callback_logged = false,
    draw_block_reason_logged = {},
    draw_ready_logged = false,
    draw_failure_logged = false,
    metrics = {
        raw_monsters = 0,
        accepted = 0,
        rejected_player = 0,
        rejected_owned = 0,
        rejected_dead = 0,
        rejected_unknown = 0,
        invalidated = 0,
        notification_count = 0,
        scan_count = 0,
        scan_seconds = 0.0,
        draw_callbacks = 0,
        draw_calls = 0,
        draw_seconds = 0.0,
        bridge_syncs = 0,
        bridge_set_calls = 0,
        bridge_sync_seconds = 0.0,
        admitted = 0,
        matched = 0,
        displayed = 0,
        admission_rejected = 0,
        display_truncated = 0,
        discovery_seconds = 0.0,
        admission_seconds = 0.0,
        filter_seconds = 0.0,
        order_seconds = 0.0,
        budget_seconds = 0.0,
        last_emit_at = 0.0,
    },
}

local function now()
    return os.time() + (os.clock() % 1)
end

local function log_event(code, message)
    if message == nil or message == "" then
        print(string.format("[%s] %s", MOD_NAME, code))
        return
    end

    print(string.format("[%s] %s %s", MOD_NAME, code, message))
end

local function debug_event(code, message)
    if config.DEBUG then
        log_event(code, message)
    end
end

local function unwrap(value)
    if value == nil then
        return nil
    end

    if type(value) == "userdata" then
        local ok_getter, getter = pcall(function()
            return value.get
        end)
        if ok_getter and getter ~= nil then
            local ok, result = pcall(function()
                return getter(value)
            end)
            if ok then
                return result
            end
        end
    end

    return value
end

local function is_valid(object)
    object = unwrap(object)
    if object == nil then
        return false
    end

    if type(object) == "userdata" then
        local ok_validator, validator = pcall(function()
            return object.IsValid
        end)
        if ok_validator and validator ~= nil then
            local ok, result = pcall(function()
                return validator(object)
            end)
            return ok and result == true
        end
    end

    return true
end

local function safe_get_property(object, property_name)
    object = unwrap(object)
    if not is_valid(object) then
        return false, nil
    end

    local ok, result = pcall(function()
        return object[property_name]
    end)
    if not ok then
        return false, nil
    end

    return true, unwrap(result)
end

local function safe_call_no_args(object, method_name)
    object = unwrap(object)
    if not is_valid(object) then
        return false, nil
    end

    local ok_method, method = pcall(function()
        return object[method_name]
    end)
    if not ok_method or method == nil then
        return false, nil
    end

    local ok, result = pcall(function()
        return method(object)
    end)
    if not ok then
        return false, nil
    end

    return true, unwrap(result)
end

local function safe_call_with_args(object, method_name, ...)
    object = unwrap(object)
    if not is_valid(object) then
        return false, nil
    end

    local ok_method, method = pcall(function()
        return object[method_name]
    end)
    if not ok_method or method == nil then
        return false, nil
    end

    local args = { ... }
    local ok, result = pcall(function()
        return method(object, table.unpack(args))
    end)
    if not ok then
        return false, nil
    end

    return true, unwrap(result)
end

local function value_to_text(value)
    value = unwrap(value)
    if value == nil then
        return "<nil>"
    end

    local value_type = type(value)
    if value_type == "string" or value_type == "number" or value_type == "boolean" then
        return tostring(value)
    end

    if value_type == "table" then
        local length_ok, length = pcall(function()
            return #value
        end)
        if length_ok then
            return string.format("<table length=%s>", tostring(length))
        end
    end

    return string.format("<%s>", value_type)
end

local function read_number(value)
    value = unwrap(value)
    if type(value) == "number" then
        return value
    end

    local converted = tonumber(value)
    if converted ~= nil then
        return converted
    end

    return nil
end

local function read_xyz(value)
    value = unwrap(value)
    if value == nil then
        return nil, nil, nil
    end

    local ok, x, y, z = pcall(function()
        return value.X, value.Y, value.Z
    end)
    if not ok then
        return nil, nil, nil
    end

    x = read_number(x)
    y = read_number(y)
    z = read_number(z)
    if x == nil or y == nil or z == nil then
        return nil, nil, nil
    end

    return x, y, z
end

local function get_guid_parts(value)
    value = unwrap(value)
    if value == nil then
        return nil
    end

    local ok, part_a, part_b, part_c, part_d = pcall(function()
        return value.A, value.B, value.C, value.D
    end)
    if not ok then
        return nil
    end

    part_a = read_number(part_a)
    part_b = read_number(part_b)
    part_c = read_number(part_c)
    part_d = read_number(part_d)
    if part_a == nil or part_b == nil or part_c == nil or part_d == nil then
        return nil
    end

    return part_a, part_b, part_c, part_d
end

local function is_zero_guid(value)
    local part_a, part_b, part_c, part_d = get_guid_parts(value)
    if part_a == nil then
        return nil
    end

    return part_a == 0 and part_b == 0 and part_c == 0 and part_d == 0
end

local function resolve_class(name)
    local path = CLASS_PATHS[name]
    if path == nil then
        return nil
    end

    local cached = state.classes[name]
    if is_valid(cached) then
        return cached
    end

    local ok, result = pcall(function()
        return StaticFindObject(path)
    end)
    if ok and is_valid(result) then
        state.classes[name] = result
        debug_event("CLASS_RESOLVED", string.format("name=%s path=%s", name, path))
        return result
    end

    state.classes[name] = nil
    log_event("CLASS_UNAVAILABLE", string.format("name=%s path=%s", name, path))
    return nil
end

local function resolve_required_classes()
    resolve_class("player")
    resolve_class("monster")
    resolve_class("player_controller")
    resolve_class("player_state")
end

local function is_a(object, class_object)
    object = unwrap(object)
    if not is_valid(object) or not is_valid(class_object) then
        return false
    end

    local ok, result = pcall(function()
        return object:IsA(class_object)
    end)
    return ok and result == true
end

local function get_character_parameter_component(character)
    if not is_valid(character) then
        return nil
    end

    local ok, result = safe_call_no_args(character, "GetCharacterParameterComponent")
    if ok and is_valid(result) then
        return result
    end

    ok, result = safe_get_property(character, "CharacterParameterComponent")
    if ok and is_valid(result) then
        return result
    end

    return nil
end

local function try_get_individual_parameter(object, depth)
    depth = depth or 0
    if depth > 3 or not is_valid(object) then
        return nil
    end

    local ok, result = safe_call_no_args(object, "GetIndividualParameter")
    if ok and is_valid(result) then
        return result
    end

    ok, result = safe_call_no_args(object, "TryGetIndividualParameter")
    if ok and is_valid(result) then
        return result
    end

    local component = get_character_parameter_component(object)
    if is_valid(component) and component ~= object then
        result = try_get_individual_parameter(component, depth + 1)
        if is_valid(result) then
            return result
        end
    end

    ok, result = safe_get_property(object, "IndividualHandle")
    if ok and is_valid(result) and result ~= object then
        return try_get_individual_parameter(result, depth + 1)
    end

    return nil
end

local function get_save_parameter(parameter)
    if not is_valid(parameter) then
        return nil
    end

    local ok, result = safe_get_property(parameter, "SaveParameter")
    if ok and result ~= nil then
        return result
    end

    ok, result = safe_call_no_args(parameter, "GetSaveParameter")
    if ok and result ~= nil then
        return result
    end

    return nil
end

local function has_player_owner_chain(actor)
    local player_class = resolve_class("player")
    local controller_class = resolve_class("player_controller")
    local state_class = resolve_class("player_state")

    for _, method_name in ipairs({ "GetOwner", "GetInstigator", "GetController" }) do
        local ok, related = safe_call_no_args(actor, method_name)
        if ok and is_valid(related) then
            if is_a(related, player_class) or is_a(related, controller_class) or is_a(related, state_class) then
                return true
            end
        end
    end

    return false
end

local function get_owner_uid_status(parameter, save_parameter)
    local ok, value = safe_get_property(save_parameter, "OwnerPlayerUId")
    if not ok then
        ok, value = safe_get_property(save_parameter, "OwnerPlayerUid")
    end
    if not ok then
        ok, value = safe_call_no_args(parameter, "GetOwnerPlayerUId")
    end
    if not ok then
        ok, value = safe_get_property(parameter, "OwnerPlayerUId")
    end
    if not ok then
        return nil
    end

    local zero = is_zero_guid(value)
    if zero == nil then
        return nil
    end

    return zero and "none" or "player"
end

local function get_trainer_status(component)
    if not is_valid(component) then
        return nil
    end

    local any_path = false
    for _, access in ipairs({
        { kind = "method", name = "GetTrainer" },
        { kind = "property", name = "Trainer" },
        { kind = "property", name = "NPCSpawnedOtomoTrainer" },
    }) do
        local ok, value
        if access.kind == "method" then
            ok, value = safe_call_no_args(component, access.name)
        else
            ok, value = safe_get_property(component, access.name)
        end

        if ok then
            any_path = true
            if is_valid(value) then
                return "present"
            end
        end
    end

    if any_path then
        return "none"
    end

    return nil
end

local function classify_actor(actor)
    actor = unwrap(actor)
    if not is_valid(actor) then
        return "invalid", nil, nil
    end

    if is_a(actor, resolve_class("player")) then
        return "player_class", nil, nil
    end
    if is_a(actor, resolve_class("player_controller")) then
        return "player_controller", nil, nil
    end
    if is_a(actor, resolve_class("player_state")) then
        return "player_state", nil, nil
    end
    if not is_a(actor, resolve_class("monster")) then
        return "unknown_type", nil, nil
    end

    local component = get_character_parameter_component(actor)
    if not is_valid(component) then
        return "parameter_component_unavailable", nil, nil
    end

    local ok_is_dead, is_dead = safe_call_no_args(component, "IsDead")
    if ok_is_dead and is_dead == true then
        return "dead", component, nil
    end

    local parameter = try_get_individual_parameter(actor)
    if not is_valid(parameter) then
        return "individual_parameter_unavailable", component, nil
    end

    local save_parameter = get_save_parameter(parameter)
    if save_parameter == nil then
        return "save_parameter_unavailable", component, parameter, nil
    end

    local ok_is_player, is_player_parameter = safe_get_property(save_parameter, "IsPlayer")
    if not ok_is_player then
        ok_is_player, is_player_parameter = safe_get_property(parameter, "IsPlayer")
    end
    if ok_is_player and is_player_parameter == true then
        return "player_parameter", component, parameter, save_parameter
    end

    if has_player_owner_chain(actor) then
        return "player_owner_chain", component, parameter, save_parameter
    end

    local owner_status = get_owner_uid_status(parameter, save_parameter)
    if owner_status == nil then
        return "owner_uid_unavailable", component, parameter, save_parameter
    end
    if owner_status == "player" then
        return "player_owned", component, parameter, save_parameter
    end

    local trainer_status = get_trainer_status(component)
    if trainer_status == nil then
        return "trainer_unavailable", component, parameter, save_parameter
    end
    if trainer_status == "present" then
        return "trainer_associated", component, parameter, save_parameter
    end

    return "wild", component, parameter, save_parameter
end

local function reject_player_representation(actor)
    actor = unwrap(actor)
    if not is_valid(actor) then
        return nil
    end
    if is_a(actor, resolve_class("player")) then
        return "player_class"
    end
    if is_a(actor, resolve_class("player_controller")) then
        return "player_controller"
    end
    if is_a(actor, resolve_class("player_state")) then
        return "player_state"
    end
    return nil
end

local function probe_source_value(source, definition)
    if not is_valid(source) then
        return false, nil, nil
    end

    for _, method_name in ipairs(definition.methods or {}) do
        local ok, value = safe_call_no_args(source, method_name)
        if ok and value ~= nil then
            return true, value, "method:" .. method_name
        end
    end

    for _, property_name in ipairs(definition.properties or {}) do
        local ok, value = safe_get_property(source, property_name)
        if ok and value ~= nil then
            return true, value, "property:" .. property_name
        end
    end

    return false, nil, nil
end

local function probe_candidate_fields(actor, component, parameter, save_parameter)
    local sources = {
        actor = actor,
        component = component,
        parameter = parameter,
        save = save_parameter,
    }
    local fields = {}

    for _, definition in ipairs(FIELD_PROBES) do
        local found = false
        local value = nil
        local path = nil

        if not state.field_path_logged[definition.name] then
            debug_event("FIELD_PROBE_BEGIN", string.format("field=%s", definition.name))
        end

        for _, source_name in ipairs(definition.sources or {}) do
            local ok, candidate_value, candidate_path = probe_source_value(sources[source_name], definition)
            if ok then
                found = true
                value = candidate_value
                path = source_name .. "." .. candidate_path
                break
            end
        end

        fields[definition.name] = {
            available = found,
            value = value,
            path = path,
            note = definition.note,
        }

        if not state.field_path_logged[definition.name] then
            state.field_path_logged[definition.name] = true
            if found then
                log_event("FIELD_PATH", string.format(
                    "field=%s status=available path=%s value=%s",
                    definition.name,
                    path,
                    value_to_text(value)
                ))
            else
                log_event("FIELD_PATH", string.format(
                    "field=%s status=unavailable note=%s",
                    definition.name,
                    definition.note or "no tested accessor returned a value"
                ))
            end
        end
    end

    return fields
end

local function remove_candidate(actor, reason)
    local record = state.candidates[actor]
    if record == nil then
        return
    end

    state.candidates[actor] = nil
    state.candidate_count = math.max(0, state.candidate_count - 1)
    if state.selected == record then
        state.selected = nil
    end
    debug_event("PAL_REMOVE", string.format("reason=%s candidates=%d", reason, state.candidate_count))
end

local function record_rejection(reason)
    local function log_reason_once(code)
        if state.rejection_reason_logged[reason] then
            return
        end
        state.rejection_reason_logged[reason] = true
        debug_event(code, string.format("reason=%s", reason))
    end

    if reason == "player_class" or reason == "player_controller" or reason == "player_state"
        or reason == "player_parameter" or reason == "player_owner_chain" then
        state.metrics.rejected_player = state.metrics.rejected_player + 1
        log_reason_once("CLASS_REJECT_PLAYER")
        return
    end

    if reason == "player_owned" or reason == "trainer_associated" then
        state.metrics.rejected_owned = state.metrics.rejected_owned + 1
        log_reason_once("CLASS_REJECT_OWNED")
        return
    end

    if reason == "dead" then
        state.metrics.rejected_dead = state.metrics.rejected_dead + 1
        log_reason_once("CLASS_REJECT_DEAD")
        return
    end

    state.metrics.rejected_unknown = state.metrics.rejected_unknown + 1
    log_reason_once("CLASS_REJECT_UNKNOWN")
end

local function add_candidate(actor, source)
    if state.world_transitioning or not state.gameplay_active then
        return false
    end

    actor = unwrap(actor)
    if not config.ENABLED or not is_valid(actor) then
        return false
    end

    local classification, component, parameter, save_parameter = classify_actor(actor)
    if classification ~= "wild" then
        if state.candidates[actor] ~= nil then
            remove_candidate(actor, "classification_changed:" .. classification)
        end
        record_rejection(classification)
        return false
    end

    if state.candidates[actor] ~= nil then
        return false
    end

    if state.candidate_count >= config.MAX_CANDIDATES then
        debug_event("PAL_REJECT_BUDGET", string.format("max=%d", config.MAX_CANDIDATES))
        return false
    end

    local fields = {}
    if config.FIELD_PROBES_ENABLED then
        fields = probe_candidate_fields(actor, component, parameter, save_parameter)
    end

    local record = {
        actor = actor,
        source = source,
        accepted_at = now(),
        distance_m = nil,
    }

    state.candidates[actor] = record
    state.candidate_count = state.candidate_count + 1
    state.metrics.accepted = state.metrics.accepted + 1

    local species = fields.species and value_to_text(fields.species.value) or "<unknown>"
    local level = fields.level and value_to_text(fields.level.value) or "<unknown>"
    if not state.pal_accept_logged then
        state.pal_accept_logged = true
        log_event("PAL_ACCEPT", string.format(
            "source=%s species=%s level=%s candidates=%d",
            source,
            species,
            level,
            state.candidate_count
        ))
    end
    return true
end

local function clear_candidate_cache(reason)
    local previous_count = state.candidate_count
    entity_snapshot.clear(state.entity_store, reason)
    state.candidates = {}
    state.candidate_count = 0
    state.matched_records = {}
    state.display_records = {}
    state.metrics.admitted = 0
    state.metrics.matched = 0
    state.metrics.displayed = 0
    state.metrics.display_truncated = 0
    state.selected = nil

    if previous_count > 0 then
        debug_event("CANDIDATES_CLEARED", string.format(
            "reason=%s previous=%d",
            reason,
            previous_count
        ))
    end
end

-- __DEPRECATED_20260715__ [reason: per-object validity checks can crash during world teardown]
local function cleanup_candidates()
    clear_candidate_cache("legacy_cleanup")
end

local function get_actor_location(actor)
    local ok, location = safe_call_no_args(actor, "K2_GetActorLocation")
    if ok and location ~= nil then
        return location
    end

    ok, location = safe_call_no_args(actor, "GetActorLocation")
    if ok and location ~= nil then
        return location
    end

    return nil
end

local function find_player_controller()
    local ok, controller = pcall(function()
        return FindFirstOf("PalPlayerController")
    end)
    if ok and is_valid(controller) then
        return controller
    end

    ok, controller = pcall(function()
        return FindFirstOf("PlayerController")
    end)
    if ok and is_valid(controller) then
        return controller
    end

    return nil
end

local function get_camera_location(controller)
    local ok, camera_manager = safe_get_property(controller, "PlayerCameraManager")
    if not ok or not is_valid(camera_manager) then
        return nil
    end

    ok, camera_manager = safe_call_no_args(camera_manager, "GetCameraLocation")
    if ok and camera_manager ~= nil then
        return camera_manager
    end

    return nil
end

local function distance_squared(first, second)
    local x1, y1, z1 = read_xyz(first)
    local x2, y2, z2 = read_xyz(second)
    if x1 == nil or x2 == nil then
        return nil
    end

    local dx = x1 - x2
    local dy = y1 - y2
    local dz = z1 - z2
    return dx * dx + dy * dy + dz * dz
end

local function distance_m(actor, context)
    local camera_location = context and context.camera_location or nil
    if camera_location == nil then
        camera_location = get_camera_location(find_player_controller())
    end
    local actor_location = get_actor_location(actor)
    local squared = distance_squared(camera_location, actor_location)
    if squared == nil then
        return nil
    end
    return math.sqrt(squared) / 100.0
end

local function find_all(class_name)
    local ok, objects = pcall(function()
        return FindAllOf(class_name)
    end)
    if not ok or type(objects) ~= "table" then
        return nil
    end
    return objects
end

local entity_runtime = {
    reject_player_representation = reject_player_representation,
    classify_pal = classify_actor,
    probe_pal_fields = function(actor, component, parameter, save_parameter)
        if not config.FIELD_PROBES_ENABLED then
            return {}
        end
        return probe_candidate_fields(actor, component, parameter, save_parameter)
    end,
    distance_m = distance_m,
    find_all = find_all,
}

local function refresh_pipeline_output(source)
    local current = entity_snapshot.current(state.entity_store)

    local filter_started_at = os.clock()
    local matched = filter_engine.filter(current.records, config.ACTIVE_FILTERS)
    state.metrics.filter_seconds = state.metrics.filter_seconds + (os.clock() - filter_started_at)

    local order_started_at = os.clock()
    local ordered = filter_engine.order(matched)
    state.metrics.order_seconds = state.metrics.order_seconds + (os.clock() - order_started_at)

    local budget_started_at = os.clock()
    local configured_limit = math.max(0, tonumber(config.MAX_DISPLAY_TARGETS) or 0)
    local maximum_limit = math.max(0, tonumber(config.MAX_CONFIGURABLE_DISPLAY_TARGETS) or 0)
    local display_limit = math.min(configured_limit, maximum_limit)
    local displayed, truncated = filter_engine.budget(ordered, display_limit)
    state.metrics.budget_seconds = state.metrics.budget_seconds + (os.clock() - budget_started_at)

    state.candidates = current.lookup
    state.candidate_count = #current.records
    state.matched_records = ordered
    state.display_records = displayed
    state.metrics.admitted = state.candidate_count
    state.metrics.matched = #ordered
    state.metrics.displayed = #displayed
    state.metrics.display_truncated = truncated

    log_event("ENTITY_SNAPSHOT", string.format(
        "session=%d generation=%d source=%s admitted=%d",
        current.session_id,
        current.generation_id,
        source,
        state.metrics.admitted
    ))
    log_event("FILTER_RESULT", string.format(
        "session=%d generation=%d admitted=%d matched=%d displayed=%d",
        current.session_id,
        current.generation_id,
        state.metrics.admitted,
        state.metrics.matched,
        state.metrics.displayed
    ))
    if truncated > 0 then
        log_event("DISPLAY_BUDGET", string.format(
            "session=%d generation=%d limit=%d truncated=%d",
            current.session_id,
            current.generation_id,
            display_limit,
            truncated
        ))
    end
end

local function add_entity_candidate(actor, source, generation, context)
    if state.world_transitioning or not state.gameplay_active then
        return false
    end

    actor = unwrap(actor)
    if not config.ENABLED or not is_valid(actor) then
        return false
    end

    if #generation.records >= config.MAX_ADMITTED_ENTITIES then
        state.metrics.admission_rejected = state.metrics.admission_rejected + 1
        if state.metrics.admission_rejected == 1 then
            log_event("ADMISSION_BUDGET", string.format(
                "session=%d generation=%d limit=%d",
                generation.session_id,
                generation.generation_id,
                config.MAX_ADMITTED_ENTITIES
            ))
        end
        return false
    end

    context = context or {}
    context.source = source
    context.accepted_at = now()
    local normalized, rejection = state.entity_registry:admit(entity_runtime, actor, context)
    if normalized == nil then
        record_rejection(rejection and rejection.reason or "adapter_rejected")
        return false
    end

    local added, record = entity_snapshot.add(generation, normalized)
    if not added then
        return false
    end

    state.metrics.accepted = state.metrics.accepted + 1
    if not state.pal_accept_logged then
        state.pal_accept_logged = true
        local level_cell = record.fields.level
        local level = level_cell and level_cell.state == "known" and tostring(level_cell.value) or "<unknown>"
        log_event("PAL_ACCEPT", string.format(
            "source=%s level=%s generation=%d ordinal=%d",
            source,
            level,
            record.generation_id,
            record.ordinal
        ))
    end
    return true
end

local function select_target()
    local controller = find_player_controller()
    local camera_location = get_camera_location(controller)
    local selected = nil
    local selected_distance = nil

    for _, record in pairs(state.candidates) do
        if is_valid(record.actor) then
            local actor_location = get_actor_location(record.actor)
            if actor_location ~= nil then
                local candidate_distance = distance_squared(camera_location, actor_location)
                if selected == nil or (candidate_distance ~= nil and (
                    selected_distance == nil or candidate_distance < selected_distance
                )) then
                    selected = record
                    selected_distance = candidate_distance
                end
            end
        end
    end

    if selected ~= nil and selected_distance ~= nil then
        selected.distance_m = math.sqrt(selected_distance) / 100.0
    end
    state.selected = selected
end

local function gameplay_world_available()
    if state.world_transitioning then
        return false, "load_map_transition"
    end

    local ok, players = pcall(function()
        return FindAllOf("PalPlayerCharacter")
    end)
    if not ok or type(players) ~= "table" then
        return false, "local_player_query_failed"
    end
    if #players < 1 then
        return false, "no_local_player"
    end

    return true, nil
end

--[[ __DEPRECATED_20260716__ [reason: replaced by adapter admission into a generation-scoped snapshot]
local function scan_monsters_legacy(source)
    local started_at = os.clock()
    local ok, objects = pcall(function()
        return FindAllOf("PalMonsterCharacter")
    end)

    state.metrics.scan_count = state.metrics.scan_count + 1
    if not ok or objects == nil then
        state.metrics.scan_seconds = state.metrics.scan_seconds + (os.clock() - started_at)
        log_event("SCAN_EMPTY", string.format("source=%s", source))
        return 0
    end

    local raw_count = 0
    for _, actor in ipairs(objects) do
        raw_count = raw_count + 1
        local added_ok, added_error = pcall(function()
            add_candidate(actor, source)
        end)
        if not added_ok then
            log_event("ERROR_CANDIDATE", tostring(added_error))
        end
    end

    local elapsed = os.clock() - started_at
    state.metrics.raw_monsters = raw_count
    state.metrics.scan_seconds = state.metrics.scan_seconds + elapsed
    log_event("SCAN_DONE", string.format(
        "source=%s raw=%d candidates=%d elapsed_ms=%.3f",
        source,
        raw_count,
        state.candidate_count,
        elapsed * 1000.0
    ))
    return raw_count
end
]]

local function scan_monsters(source)
    local started_at = os.clock()
    local descriptor = state.entity_registry:get("pal")
    local generation = entity_snapshot.begin_generation(state.entity_store, source, now())
    state.metrics.scan_count = state.metrics.scan_count + 1
    state.metrics.admission_rejected = 0

    local discovery_started_at = os.clock()
    local ok, objects = pcall(function()
        return descriptor and descriptor.find_all(entity_runtime) or nil
    end)
    state.metrics.discovery_seconds = state.metrics.discovery_seconds + (os.clock() - discovery_started_at)
    if not ok or type(objects) ~= "table" then
        entity_snapshot.replace(state.entity_store, generation)
        refresh_pipeline_output(source)
        state.metrics.raw_monsters = 0
        state.metrics.scan_seconds = state.metrics.scan_seconds + (os.clock() - started_at)
        log_event("SCAN_EMPTY", string.format("source=%s", source))
        return 0
    end

    local context = {
        source = source,
        camera_location = get_camera_location(find_player_controller()),
    }
    local raw_count = 0
    local admission_started_at = os.clock()
    for _, actor in ipairs(objects) do
        raw_count = raw_count + 1
        local added_ok, added_error = pcall(function()
            add_entity_candidate(actor, source, generation, context)
        end)
        if not added_ok then
            log_event("ERROR_CANDIDATE", tostring(added_error))
        end
    end
    state.metrics.admission_seconds = state.metrics.admission_seconds + (os.clock() - admission_started_at)

    entity_snapshot.replace(state.entity_store, generation)
    refresh_pipeline_output(source)

    local elapsed = os.clock() - started_at
    state.metrics.raw_monsters = raw_count
    state.metrics.scan_seconds = state.metrics.scan_seconds + elapsed
    log_event("SCAN_DONE", string.format(
        "source=%s raw=%d admitted=%d matched=%d displayed=%d elapsed_ms=%.3f",
        source,
        raw_count,
        state.metrics.admitted,
        state.metrics.matched,
        state.metrics.displayed,
        elapsed * 1000.0
    ))
    return raw_count
end

--[[ __DEPRECATED_20260716__ [reason: audit now exercises the registry-owned hard player gate]
local function audit_player_boundary_legacy()
    local ok, players = pcall(function()
        return FindAllOf("PalPlayerCharacter")
    end)
    if not ok or players == nil then
        log_event("PLAYER_AUDIT", "status=no_player_instances")
        return
    end

    local audited = 0
    local boundary_failures = 0
    for _, player in ipairs(players) do
        audited = audited + 1
        local classification = classify_actor(player)
        if classification == "wild" then
            boundary_failures = boundary_failures + 1
            log_event("ERROR_PLAYER_BOUNDARY", "classification=wild")
        else
            record_rejection(classification)
        end
    end

    log_event("PLAYER_AUDIT", string.format(
        "audited=%d candidate_player_count=%d",
        audited,
        boundary_failures
    ))
end
]]

local function audit_player_boundary()
    local players = find_all("PalPlayerCharacter")
    if players == nil then
        log_event("PLAYER_AUDIT", "status=no_player_instances")
        return
    end

    local audited = 0
    local boundary_failures = 0
    for _, player in ipairs(players) do
        audited = audited + 1
        local normalized, rejection = state.entity_registry:admit(entity_runtime, player, {
            source = "player_audit",
            accepted_at = now(),
        })
        if normalized ~= nil then
            boundary_failures = boundary_failures + 1
            log_event("ERROR_PLAYER_BOUNDARY", "classification=admitted")
        else
            record_rejection(rejection and rejection.reason or "unknown_player_rejection")
        end
    end

    log_event("PLAYER_AUDIT", string.format(
        "audited=%d candidate_player_count=%d",
        audited,
        boundary_failures
    ))
end

local function emit_metrics(force)
    local current = now()
    if not force and current - state.metrics.last_emit_at < config.METRIC_INTERVAL_SECONDS then
        return
    end

    state.metrics.last_emit_at = current
    --[[ __DEPRECATED_20260716__ [reason: Entity Core exposes separate pipeline counts and timings]
    log_event("METRIC_LEGACY", string.format(
        "session=%d raw=%d candidates=%d accepted_total=%d rejected_player=%d rejected_owned=%d rejected_dead=%d rejected_unknown=%d invalidated=%d notifications=%d scans=%d scan_ms=%.3f draw_callbacks=%d draw_calls=%d draw_ms=%.3f bridge_syncs=%d bridge_set_calls=%d bridge_sync_ms=%.3f",
        state.session_index,
        state.metrics.raw_monsters,
        state.candidate_count,
        state.metrics.accepted,
        state.metrics.rejected_player,
        state.metrics.rejected_owned,
        state.metrics.rejected_dead,
        state.metrics.rejected_unknown,
        state.metrics.invalidated,
        state.metrics.notification_count,
        state.metrics.scan_count,
        state.metrics.scan_seconds * 1000.0,
        state.metrics.draw_callbacks,
        state.metrics.draw_calls,
        state.metrics.draw_seconds * 1000.0,
        state.metrics.bridge_syncs,
        state.metrics.bridge_set_calls,
        state.metrics.bridge_sync_seconds * 1000.0
    ))
    ]]
    local current = entity_snapshot.current(state.entity_store)
    log_event("METRIC", string.format(
        "session=%d generation=%d raw=%d admitted=%d matched=%d displayed=%d admission_rejected=%d display_truncated=%d accepted_total=%d rejected_player=%d rejected_owned=%d rejected_dead=%d rejected_unknown=%d notifications=%d scans=%d scan_ms=%.3f discovery_ms=%.3f admission_ms=%.3f filter_ms=%.3f order_ms=%.3f budget_ms=%.3f bridge_syncs=%d bridge_set_calls=%d bridge_sync_ms=%.3f",
        state.session_index,
        current.generation_id,
        state.metrics.raw_monsters,
        state.metrics.admitted,
        state.metrics.matched,
        state.metrics.displayed,
        state.metrics.admission_rejected,
        state.metrics.display_truncated,
        state.metrics.accepted,
        state.metrics.rejected_player,
        state.metrics.rejected_owned,
        state.metrics.rejected_dead,
        state.metrics.rejected_unknown,
        state.metrics.notification_count,
        state.metrics.scan_count,
        state.metrics.scan_seconds * 1000.0,
        state.metrics.discovery_seconds * 1000.0,
        state.metrics.admission_seconds * 1000.0,
        state.metrics.filter_seconds * 1000.0,
        state.metrics.order_seconds * 1000.0,
        state.metrics.budget_seconds * 1000.0,
        state.metrics.bridge_syncs,
        state.metrics.bridge_set_calls,
        state.metrics.bridge_sync_seconds * 1000.0
    ))
end

local sync_bridge_target

local function reconcile()
    if not config.ENABLED then
        return
    end

    local gameplay_active, inactive_reason = gameplay_world_available()
    state.gameplay_active = gameplay_active
    if not gameplay_active then
        clear_candidate_cache(inactive_reason)
        sync_bridge_target()
        state.metrics.raw_monsters = 0
        if state.last_scan_skip_reason ~= inactive_reason then
            state.last_scan_skip_reason = inactive_reason
            debug_event("SCAN_SKIPPED", string.format(
                "source=reconcile reason=%s",
                inactive_reason
            ))
        end
        emit_metrics(false)
        return
    end

    state.last_scan_skip_reason = nil
    -- __DEPRECATED_20260716__ [reason: scan_monsters atomically replaces the complete generation]
    -- clear_candidate_cache("reconcile_rebuild")
    scan_monsters("reconcile")
    -- __DEPRECATED_20260716__ [reason: Blueprint receives all candidates; nearest selection remains Lua-draw-only]
    -- if config.DRAW_ENABLED or config.BLUEPRINT_BRIDGE_ENABLED then
    if config.DRAW_ENABLED then
        select_target()
    end
    sync_bridge_target()
    emit_metrics(false)
end

local function reconcile_safely()
    local ok, err = pcall(reconcile)
    if not ok then
        log_event("ERROR_RECONCILE", tostring(err))
    end
end

local function run_on_game_thread(callback)
    if type(ExecuteInGameThread) == "function" then
        local ok, err = pcall(function()
            ExecuteInGameThread(callback)
        end)
        if ok then
            return true
        end
        log_event("ERROR_GAME_THREAD", tostring(err))
    end

    local ok, err = pcall(callback)
    if not ok then
        log_event("ERROR_CALLBACK", tostring(err))
        return false
    end
    return true
end

local function clear_bridge_cache(reason)
    local had_bridge = state.bridge_actor ~= nil or state.bridge_probe_actor ~= nil
    state.bridge_probe_actor = nil
    state.bridge_actor = nil
    state.bridge_session_sent = nil
    state.bridge_target_actor = nil
    state.bridge_target_session = nil
    state.bridge_target_count = 0
    state.bridge_gender_logged = false
    state.bridge_gender_pending_reason = nil

    if had_bridge then
        debug_event("BRIDGE_CLEARED", string.format("reason=%s", reason))
    end
end

local function register_blueprint_bridge_discovery()
    if not config.BLUEPRINT_BRIDGE_ENABLED or state.bridge_discovery_registered then
        return
    end
    if type(RegisterBeginPlayPostHook) ~= "function" then
        log_event("BRIDGE_DISCOVERY_UNAVAILABLE", "api=RegisterBeginPlayPostHook")
        return
    end

    local ok, err = pcall(function()
        RegisterBeginPlayPostHook(function(context_parameter)
            local actor = unwrap(context_parameter)
            if not is_valid(actor) then
                return
            end

            local class_ok, class_full_name = pcall(function()
                return actor:GetClass():GetFullName()
            end)
            if not class_ok or class_full_name ~= BRIDGE_CLASS_FULL_NAME then
                return
            end

            state.bridge_probe_actor = actor
            state.bridge_actor = actor
            state.bridge_session_sent = nil
            state.bridge_target_actor = nil
            state.bridge_target_session = nil
            state.bridge_target_count = 0
            state.bridge_gender_logged = false
            state.bridge_gender_pending_reason = nil
            log_event("BRIDGE_READY", "mode=lua_discovery class=ModActor_C")
            sync_bridge_target()
        end)
    end)
    if not ok then
        log_event("BRIDGE_DISCOVERY_FAILED", tostring(err))
        return
    end

    state.bridge_discovery_registered = true
    debug_event("BRIDGE_DISCOVERY_REGISTERED", "mode=lua_discovery")
end

local function call_bridge(method_name, ...)
    if not config.BLUEPRINT_BRIDGE_ENABLED then
        return false
    end

    local bridge_actor = state.bridge_actor
    if not is_valid(bridge_actor) then
        return false
    end

    local ok = safe_call_with_args(bridge_actor, method_name, ...)
    if not ok then
        log_event("BRIDGE_CALL_FAILED", string.format("method=%s", method_name))
        clear_bridge_cache("call_failed:" .. method_name)
        return false
    end

    return true
end

local function log_bridge_gender_diagnostic()
    if state.bridge_gender_logged or not is_valid(state.bridge_actor) then
        return
    end

    local function pending(reason)
        if state.bridge_gender_pending_reason ~= reason then
            state.bridge_gender_pending_reason = reason
            debug_event("BLUEPRINT_GENDER_PENDING", string.format("reason=%s", reason))
        end
    end

    local ok_value, value = safe_get_property(state.bridge_actor, "ESP_BridgeGenderDiagnosticCode")
    if not ok_value then
        pending("property_unavailable")
        return
    end
    if type(value) ~= "number" then
        pending("value_not_number")
        return
    end
    local gender_by_code = {
        [0] = "unknown",
        [1] = "male",
        [2] = "female",
    }
    local gender = gender_by_code[value]
    if gender == nil then
        pending("value_pending")
        return
    end

    state.bridge_gender_logged = true
    state.bridge_gender_pending_reason = nil
    log_event("BLUEPRINT_GENDER", string.format("value=%s", gender))
end

--[[ __DEPRECATED_20260716__ [reason: replaced by the all-candidate batch sync below]
sync_bridge_target = function()
    if not config.BLUEPRINT_BRIDGE_ENABLED or not is_valid(state.bridge_actor) then
        return
    end

    local session_index = state.session_index
    if state.bridge_session_sent ~= session_index then
        if not call_bridge(BRIDGE_METHOD_RESET_SESSION, session_index) then
            return
        end
        state.bridge_session_sent = session_index
        state.bridge_target_actor = nil
        state.bridge_target_session = nil
        debug_event("BRIDGE_SESSION", string.format("session=%d", session_index))
    end

    local selected = state.selected
    if selected ~= nil and is_valid(selected.actor) then
        if state.bridge_target_actor == selected.actor and state.bridge_target_session == session_index then
            return
        end

        if call_bridge(BRIDGE_METHOD_SET_TARGET, selected.actor, session_index) then
            state.bridge_target_actor = selected.actor
            state.bridge_target_session = session_index
            debug_event("BRIDGE_TARGET_SET", string.format("session=%d", session_index))
        end
        return
    end

    if state.bridge_target_actor ~= nil then
        if call_bridge(BRIDGE_METHOD_CLEAR_TARGET, session_index) then
            state.bridge_target_actor = nil
            state.bridge_target_session = nil
            debug_event("BRIDGE_TARGET_CLEARED", string.format("session=%d", session_index))
        end
    end
end
]]

sync_bridge_target = function()
    if not config.BLUEPRINT_BRIDGE_ENABLED or not is_valid(state.bridge_actor) then
        return
    end

    local session_index = state.session_index
    if state.bridge_session_sent ~= session_index then
        if not call_bridge(BRIDGE_METHOD_RESET_SESSION, session_index) then
            return
        end
        state.bridge_session_sent = session_index
        state.bridge_target_count = 0
        debug_event("BRIDGE_SESSION", string.format("session=%d", session_index))
    end

    -- __DEPRECATED_20260716__ [reason: admitted records may be removed by filters or display budget]
    -- if state.candidate_count == 0 and state.bridge_target_count == 0 then
    if #state.display_records == 0 and state.bridge_target_count == 0 then
        return
    end

    local started_at = os.clock()
    state.metrics.bridge_syncs = state.metrics.bridge_syncs + 1

    if not call_bridge(BRIDGE_METHOD_CLEAR_TARGET, session_index) then
        state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds + (os.clock() - started_at)
        return
    end

    local display_limit = math.max(0, tonumber(config.MAX_DISPLAY_TARGETS) or 0)
    local submitted = 0
    --[[ __DEPRECATED_20260716__ [reason: unordered admitted records bypassed filter ordering and display budgeting]
    for _, record in pairs(state.candidates) do
        if submitted >= display_limit then
            break
        end
        if record ~= nil and is_valid(record.actor) then
            if not call_bridge(BRIDGE_METHOD_SET_TARGET, record.actor, session_index) then
                state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds + (os.clock() - started_at)
                return
            end
            submitted = submitted + 1
            state.metrics.bridge_set_calls = state.metrics.bridge_set_calls + 1
        end
    end
    ]]
    for _, record in ipairs(state.display_records) do
        if submitted >= display_limit then
            break
        end
        if record ~= nil and is_valid(record.actor) then
            if not call_bridge(BRIDGE_METHOD_SET_TARGET, record.actor, session_index) then
                state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds + (os.clock() - started_at)
                return
            end
            submitted = submitted + 1
            state.metrics.bridge_set_calls = state.metrics.bridge_set_calls + 1
        end
    end

    state.bridge_target_count = submitted
    if submitted > 0 then
        log_bridge_gender_diagnostic()
    end
    state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds + (os.clock() - started_at)
    debug_event("BRIDGE_TARGETS_SYNCED", string.format(
        "session=%d count=%d candidates=%d limit=%d",
        session_index,
        submitted,
        state.candidate_count,
        display_limit
    ))
end

local function register_blueprint_bridge()
    if not config.BLUEPRINT_BRIDGE_ENABLED or state.bridge_event_registered then
        return
    end
    if type(RegisterCustomEvent) ~= "function" then
        log_event("BRIDGE_UNAVAILABLE", "api=RegisterCustomEvent")
        return
    end

    local ok, err = pcall(function()
        RegisterCustomEvent(BRIDGE_READY_EVENT, function(bridge_parameter)
            run_on_game_thread(function()
                local bridge_actor = unwrap(bridge_parameter)
                if not is_valid(bridge_actor) then
                    log_event("BRIDGE_REJECTED", "reason=invalid_actor")
                    return
                end

                state.bridge_actor = bridge_actor
                state.bridge_session_sent = nil
                state.bridge_target_actor = nil
                state.bridge_target_session = nil
                state.bridge_target_count = 0
                state.bridge_gender_logged = false
                state.bridge_gender_pending_reason = nil
                log_event("BRIDGE_READY", string.format("event=%s", BRIDGE_READY_EVENT))
                sync_bridge_target()
            end)
        end)
    end)
    if not ok then
        log_event("BRIDGE_REGISTER_FAILED", tostring(err))
        return
    end

    state.bridge_event_registered = true
    debug_event("BRIDGE_REGISTERED", string.format("event=%s", BRIDGE_READY_EVENT))
end

local function reset_session(reason)
    clear_candidate_cache("session_reset:" .. reason)
    state.bridge_session_sent = nil
    state.bridge_target_actor = nil
    state.bridge_target_session = nil
    state.bridge_target_count = 0
    state.bridge_gender_logged = false
    state.bridge_gender_pending_reason = nil
    state.draw_failure_logged = false
    state.draw_block_reason_logged = {}
    state.pal_accept_logged = false
    state.last_scan_skip_reason = nil
    state.session_index = state.session_index + 1
    entity_snapshot.reset_session(state.entity_store, state.session_index)
    log_event("SESSION_RESET", string.format("session=%d reason=%s", state.session_index, reason))
end

local function bootstrap(reason)
    state.bootstrap_pending = false
    state.world_transitioning = false
    reset_session(reason)
    resolve_required_classes()
    audit_player_boundary()

    local gameplay_active, inactive_reason = gameplay_world_available()
    state.gameplay_active = gameplay_active
    if gameplay_active then
        scan_monsters("bootstrap")
        -- __DEPRECATED_20260716__ [reason: Blueprint receives all candidates; nearest selection remains Lua-draw-only]
        -- if config.DRAW_ENABLED or config.BLUEPRINT_BRIDGE_ENABLED then
        if config.DRAW_ENABLED then
            select_target()
        end
        sync_bridge_target()
    else
        clear_candidate_cache(inactive_reason)
        sync_bridge_target()
        state.metrics.raw_monsters = 0
        debug_event("SCAN_SKIPPED", string.format(
            "source=bootstrap reason=%s",
            inactive_reason
        ))
    end

    emit_metrics(true)
    log_event("BOOT_OK", string.format("session=%d reason=%s", state.session_index, reason))
end

local function schedule_bootstrap(reason)
    if state.bootstrap_pending then
        return
    end
    state.bootstrap_pending = true
    local generation = state.lifecycle_generation

    local callback = function()
        if generation ~= state.lifecycle_generation then
            debug_event("BOOT_SKIPPED", string.format(
                "reason=stale_generation scheduled=%d current=%d",
                generation,
                state.lifecycle_generation
            ))
            return
        end
        run_on_game_thread(function()
            bootstrap(reason)
        end)
    end

    if type(ExecuteInGameThreadWithDelay) == "function" then
        local ok, err = pcall(function()
            ExecuteInGameThreadWithDelay(config.BOOTSTRAP_DELAY_MS, callback)
        end)
        if ok then
            return
        end
        log_event("ERROR_BOOT_DELAY", tostring(err))
    end

    if type(ExecuteWithDelay) == "function" then
        local ok, err = pcall(function()
            ExecuteWithDelay(config.BOOTSTRAP_DELAY_MS, callback)
        end)
        if ok then
            return
        end
        log_event("ERROR_BOOT_DELAY", tostring(err))
    end

    callback()
end

local function register_hook_safely(path, callback)
    if type(RegisterHook) ~= "function" then
        log_event("HOOK_UNAVAILABLE", string.format("path=%s", path))
        return nil, nil
    end

    local guarded_callback = function(...)
        local callback_ok, callback_error = pcall(callback, ...)
        if not callback_ok then
            log_event("ERROR_HOOK_CALLBACK", string.format("path=%s error=%s", path, tostring(callback_error)))
        end
    end

    local ok, pre_id, post_id = pcall(function()
        return RegisterHook(path, guarded_callback)
    end)
    if not ok then
        log_event("HOOK_FAILED", string.format("path=%s error=%s", path, tostring(pre_id)))
        return nil, nil
    end

    debug_event("HOOK_REGISTERED", string.format("path=%s", path))
    return pre_id, post_id
end

local function register_lifecycle_hooks()
    if state.lifecycle_hooks_registered then
        return
    end
    state.lifecycle_hooks_registered = true

    register_hook_safely("/Script/Engine.PlayerController:ServerAcknowledgePossession", function()
        schedule_bootstrap("server_acknowledge_possession")
    end)
    register_hook_safely("/Script/Engine.PlayerController:ClientRestart", function()
        schedule_bootstrap("client_restart")
    end)
end

local function register_load_map_hooks()
    if state.load_map_hooks_registered then
        return
    end
    state.load_map_hooks_registered = true

    if type(RegisterLoadMapPreHook) == "function" then
        local ok, err = pcall(function()
            RegisterLoadMapPreHook(function()
                state.lifecycle_generation = state.lifecycle_generation + 1
                state.bootstrap_pending = false
                state.world_transitioning = true
                state.gameplay_active = false
                clear_bridge_cache("load_map_pre")
                clear_candidate_cache("load_map_pre")
                debug_event("WORLD_TRANSITION", "phase=pre")
            end)
        end)
        if not ok then
            log_event("LOAD_MAP_PRE_FAILED", tostring(err))
        else
            debug_event("LOAD_MAP_PRE_REGISTERED", "status=ok")
        end
    else
        log_event("LOAD_MAP_PRE_UNAVAILABLE", "api=RegisterLoadMapPreHook")
    end

    if type(RegisterLoadMapPostHook) == "function" then
        local ok, err = pcall(function()
            RegisterLoadMapPostHook(function()
                state.world_transitioning = false
                state.gameplay_active = false
                debug_event("WORLD_TRANSITION", "phase=post")
                schedule_bootstrap("load_map_post")
            end)
        end)
        if not ok then
            log_event("LOAD_MAP_POST_FAILED", tostring(err))
        else
            debug_event("LOAD_MAP_POST_REGISTERED", "status=ok")
        end
    else
        log_event("LOAD_MAP_POST_UNAVAILABLE", "api=RegisterLoadMapPostHook")
    end
end

local function register_monster_notification()
    if state.notification_registered then
        return
    end
    state.notification_registered = true

    if type(NotifyOnNewObject) ~= "function" then
        log_event("NOTIFY_UNAVAILABLE", "class=/Script/Pal.PalMonsterCharacter")
        return
    end

    local ok, err = pcall(function()
        NotifyOnNewObject("/Script/Pal.PalMonsterCharacter", function(constructed_object)
            state.metrics.notification_count = state.metrics.notification_count + 1
            if state.world_transitioning or not state.gameplay_active then
                return
            end
            state.metrics.raw_monsters = state.metrics.raw_monsters + 1
            local actor = unwrap(constructed_object)
            run_on_game_thread(function()
                local generation = entity_snapshot.current(state.entity_store)
                local context = {
                    source = "notify",
                    camera_location = get_camera_location(find_player_controller()),
                }
                local added = add_entity_candidate(actor, "notify", generation, context)
                if not added then
                    return
                end
                refresh_pipeline_output("notify")
                -- __DEPRECATED_20260716__ [reason: Blueprint receives all candidates; nearest selection remains Lua-draw-only]
                -- if config.DRAW_ENABLED or config.BLUEPRINT_BRIDGE_ENABLED then
                if config.DRAW_ENABLED then
                    select_target()
                end
                sync_bridge_target()
            end)
        end)
    end)
    if not ok then
        log_event("NOTIFY_FAILED", tostring(err))
        return
    end

    debug_event("NOTIFY_REGISTERED", "class=/Script/Pal.PalMonsterCharacter")
end

local function start_reconcile_loop()
    if state.reconcile_started then
        return
    end
    state.reconcile_started = true

    if type(LoopInGameThreadWithDelay) == "function" then
        local ok, handle = pcall(function()
            return LoopInGameThreadWithDelay(config.RECONCILE_INTERVAL_MS, reconcile_safely)
        end)
        if ok then
            state.reconcile_handle = handle
            debug_event("RECONCILE_STARTED", "api=LoopInGameThreadWithDelay")
            return
        end
        log_event("RECONCILE_FAILED", tostring(handle))
    end

    if type(LoopAsync) == "function" then
        local ok, err = pcall(function()
            LoopAsync(config.RECONCILE_INTERVAL_MS, function()
                run_on_game_thread(reconcile_safely)
                return false
            end)
        end)
        if ok then
            debug_event("RECONCILE_STARTED", "api=LoopAsync_fallback")
            return
        end
        log_event("RECONCILE_FAILED", tostring(err))
    end

    log_event("RECONCILE_UNAVAILABLE", "no supported loop API")
end

local function make_vector2d(x, y)
    local constructor = rawget(_G, "FVector2D") or rawget(_G, "Vector2D")
    if type(constructor) == "function" then
        local ok, value = pcall(function()
            return constructor(x, y)
        end)
        if ok and value ~= nil then
            return value
        end

        ok, value = pcall(function()
            local result = constructor()
            result.X = x
            result.Y = y
            return result
        end)
        if ok and value ~= nil then
            return value
        end
    end

    return { X = x, Y = y }
end

local function make_linear_color(color)
    local constructor = rawget(_G, "FLinearColor") or rawget(_G, "LinearColor")
    if type(constructor) == "function" then
        local ok, value = pcall(function()
            return constructor(color.R, color.G, color.B, color.A)
        end)
        if ok and value ~= nil then
            return value
        end

        ok, value = pcall(function()
            local result = constructor()
            result.R = color.R
            result.G = color.G
            result.B = color.B
            result.A = color.A
            return result
        end)
        if ok and value ~= nil then
            return value
        end
    end

    return { R = color.R, G = color.G, B = color.B, A = color.A }
end

local function read_xy(value)
    value = unwrap(value)
    if value == nil then
        return nil, nil
    end

    local ok, x, y = pcall(function()
        return value.X, value.Y
    end)
    if not ok then
        return nil, nil
    end

    return read_number(x), read_number(y)
end

local function project_world_location(controller, world_location)
    if not is_valid(controller) or world_location == nil then
        return nil
    end

    local screen_location = make_vector2d(0.0, 0.0)
    local ok, projected, returned_location = pcall(function()
        return controller:ProjectWorldLocationToScreen(world_location, screen_location, false)
    end)
    if not ok or projected ~= true then
        return nil
    end

    local x, y = read_xy(returned_location)
    if x ~= nil and y ~= nil then
        return returned_location
    end

    x, y = read_xy(screen_location)
    if x ~= nil and y ~= nil then
        return screen_location
    end

    return nil
end

local function log_draw_block_once(reason)
    if state.draw_block_reason_logged[reason] then
        return
    end

    state.draw_block_reason_logged[reason] = true
    debug_event("DRAW_BLOCKED", string.format("reason=%s", reason))
end

local function on_draw_hud(context, size_x_parameter, size_y_parameter)
    if not config.ENABLED or not config.DRAW_ENABLED or config.MAX_DISPLAY_TARGETS < 1 then
        return
    end
    if state.world_transitioning or not state.gameplay_active then
        return
    end

    state.metrics.draw_callbacks = state.metrics.draw_callbacks + 1
    if not state.draw_callback_logged then
        state.draw_callback_logged = true
        log_event("DRAW_CALLBACK_READY", "path=/Script/Engine.HUD:ReceiveDrawHUD")
    end

    if config.DRAW_MODE == "callback_probe" then
        return
    end

    local record = state.selected
    if record == nil or not is_valid(record.actor) then
        log_draw_block_once("no_candidate")
        return
    end

    local started_at = os.clock()
    local hud = unwrap(context)
    local ok_canvas, canvas = safe_get_property(hud, "Canvas")
    if not ok_canvas or not is_valid(canvas) then
        log_draw_block_once("canvas_unavailable")
        return
    end

    local size_x = read_number(unwrap(size_x_parameter))
    if size_x == nil then
        local ok_clip, clip_x = safe_get_property(canvas, "ClipX")
        if ok_clip then
            size_x = read_number(clip_x)
        end
    end
    if size_x == nil or size_x <= 0 then
        log_draw_block_once("viewport_width_unavailable")
        return
    end

    local controller = find_player_controller()
    if not is_valid(controller) then
        log_draw_block_once("player_controller_unavailable")
        return
    end

    local world_location = get_actor_location(record.actor)
    if world_location == nil then
        log_draw_block_once("target_location_unavailable")
        return
    end

    local screen_location = project_world_location(controller, world_location)
    local screen_x, screen_y = read_xy(screen_location)
    if screen_x == nil or screen_y == nil then
        log_draw_block_once("projection_failed")
        return
    end

    local start_point = make_vector2d(size_x * 0.5, config.TOP_ANCHOR_Y)
    local end_point = make_vector2d(screen_x, screen_y)
    local color = make_linear_color(config.LINE_COLOR)
    local ok, err = pcall(function()
        canvas:K2_DrawLine(start_point, end_point, config.LINE_THICKNESS, color)
    end)
    if not ok then
        if not state.draw_failure_logged then
            state.draw_failure_logged = true
            log_event("DRAW_FAILED", tostring(err))
        end
        return
    end

    state.metrics.draw_calls = state.metrics.draw_calls + 1
    state.metrics.draw_seconds = state.metrics.draw_seconds + (os.clock() - started_at)
    if not state.draw_ready_logged then
        state.draw_ready_logged = true
        log_event("DRAW_READY", "anchor=top_center target_count=1")
    end
end

local function register_draw_hook()
    if state.draw_hook_registered then
        return
    end
    state.draw_hook_registered = true

    local pre_id, post_id = register_hook_safely("/Script/Engine.HUD:ReceiveDrawHUD", on_draw_hud)
    if pre_id == nil and post_id == nil then
        log_event("DRAW_HOOK_BLOCKED", "path=/Script/Engine.HUD:ReceiveDrawHUD")
    end
end

local function register_entity_adapters()
    if state.adapters_registered then
        return
    end
    local descriptor = state.entity_registry:register(pal_adapter.new())
    state.adapters_registered = true
    log_event("ADAPTER_REGISTERED", string.format(
        "id=%s notification_class=%s",
        descriptor.id,
        descriptor.notification_class
    ))
end

if not config.ENABLED then
    log_event("BOOT_DISABLED", "config.ENABLED=false")
    return
end

resolve_required_classes()
register_entity_adapters()
register_blueprint_bridge()
register_blueprint_bridge_discovery()
register_monster_notification()
register_lifecycle_hooks()
register_load_map_hooks()
register_draw_hook()
start_reconcile_loop()
schedule_bootstrap("mod_load")
log_event("BOOT_FILE_LOADED", "server_compatibility=community_untested")
