local config = require("config")

local MOD_NAME = "PalworldResourceESP"

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
    selected = nil,
    notification_registered = false,
    lifecycle_hooks_registered = false,
    load_map_hooks_registered = false,
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
        rejected_unknown = 0,
        invalidated = 0,
        notification_count = 0,
        scan_count = 0,
        scan_seconds = 0.0,
        draw_callbacks = 0,
        draw_calls = 0,
        draw_seconds = 0.0,
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
        return true
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
    state.candidates = {}
    state.candidate_count = 0
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

local function scan_monsters(source)
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

local function audit_player_boundary()
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

local function emit_metrics(force)
    local current = now()
    if not force and current - state.metrics.last_emit_at < config.METRIC_INTERVAL_SECONDS then
        return
    end

    state.metrics.last_emit_at = current
    log_event("METRIC", string.format(
        "session=%d raw=%d candidates=%d accepted_total=%d rejected_player=%d rejected_owned=%d rejected_unknown=%d invalidated=%d notifications=%d scans=%d scan_ms=%.3f draw_callbacks=%d draw_calls=%d draw_ms=%.3f",
        state.session_index,
        state.metrics.raw_monsters,
        state.candidate_count,
        state.metrics.accepted,
        state.metrics.rejected_player,
        state.metrics.rejected_owned,
        state.metrics.rejected_unknown,
        state.metrics.invalidated,
        state.metrics.notification_count,
        state.metrics.scan_count,
        state.metrics.scan_seconds * 1000.0,
        state.metrics.draw_callbacks,
        state.metrics.draw_calls,
        state.metrics.draw_seconds * 1000.0
    ))
end

local function reconcile()
    if not config.ENABLED then
        return
    end

    local gameplay_active, inactive_reason = gameplay_world_available()
    state.gameplay_active = gameplay_active
    if not gameplay_active then
        clear_candidate_cache(inactive_reason)
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
    clear_candidate_cache("reconcile_rebuild")
    scan_monsters("reconcile")
    if config.DRAW_ENABLED then
        select_target()
    end
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

local function reset_session(reason)
    clear_candidate_cache("session_reset:" .. reason)
    state.draw_failure_logged = false
    state.draw_block_reason_logged = {}
    state.pal_accept_logged = false
    state.last_scan_skip_reason = nil
    state.session_index = state.session_index + 1
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
        if config.DRAW_ENABLED then
            select_target()
        end
    else
        clear_candidate_cache(inactive_reason)
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
            local actor = unwrap(constructed_object)
            run_on_game_thread(function()
                add_candidate(actor, "notify")
                if config.DRAW_ENABLED then
                    select_target()
                end
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

if not config.ENABLED then
    log_event("BOOT_DISABLED", "config.ENABLED=false")
    return
end

resolve_required_classes()
register_monster_notification()
register_lifecycle_hooks()
register_load_map_hooks()
register_draw_hook()
start_reconcile_loop()
schedule_bootstrap("mod_load")
log_event("BOOT_FILE_LOADED", "server_compatibility=community_untested")
