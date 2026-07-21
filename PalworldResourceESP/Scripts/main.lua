local config = require("config")
local adapter_registry = require("core.adapter_registry")
local entity_snapshot = require("core.entity_snapshot")
local filter_engine = require("core.filter_engine")
local runtime_profiles = require("core.runtime_profiles")
local user_settings = require("core.user_settings")
local pal_adapter = require("adapters.pal")

local MOD_NAME = "PalworldResourceESP"
local BRIDGE_READY_EVENT = "PalworldResourceESP_LuaBridgeReady"
local BRIDGE_METHOD_RESET_SESSION = "PalworldResourceESP_ResetSession"
local BRIDGE_METHOD_SET_TARGET = "PalworldResourceESP_SetTarget"
local BRIDGE_METHOD_REMOVE_TARGET = "PalworldResourceESP_RemoveTarget"
local BRIDGE_METHOD_CLEAR_TARGET = "PalworldResourceESP_ClearTarget"
local BRIDGE_METHOD_TOGGLE_PANEL = "PalworldResourceESP_TogglePanel"
local BRIDGE_METHOD_SET_DISPLAY_STYLE = "PalworldResourceESP_SetDisplayStyle"
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
        note = "provided by the typed Blueprint element-mask adapter after Lua admission",
    },
    {
        name = "capture_count",
        sources = {},
        note = "requires a separate local player-save collection adapter",
    },
}

local NORMALIZED_FIELD_NAMES = {
    "is_wild",
    "level",
    "distance_m",
    "species",
    "gender",
    "passive_skills",
    "iv_hp",
    "iv_attack",
    "iv_melee",
    "iv_defense",
    "lucky",
    "alpha_boss",
    "elements",
    "capture_count",
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
    notification_dirty = false,
    notification_deferred_logged = false,
    event_queue = {},
    event_queue_head = 1,
    event_queue_scheduled = false,
    event_queue_overflow_logged = false,
    event_queue_keys = {},
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
    bridge_target_keys = {},
    bridge_target_order = {},
    nearest_candidates = {},
    nearest_candidate_count = 0,
    nearest_candidate_order = {},
    nearest_refresh_cursor = 1,
    nearest_index_dirty = true,
    next_nearest_refresh_at = 0.0,
    atomic_rebuild_dirty = false,
    atomic_rebuild_reason = nil,
    atomic_rebuild_due_at = 0.0,
    atomic_rebuild_deadline_at = 0.0,
    atomic_rebuild_revision = 0,
    next_atomic_rebuild_at = 0.0,
    cached_bridge_resync_dirty = false,
    cached_bridge_resync_force = false,
    cached_bridge_resync_reason = nil,
    cached_bridge_resync_revision = 0,
    stream_integrity_last_location = nil,
    stream_integrity_distance_cm = 0.0,
    next_stream_integrity_scan_at = 0.0,
    bridge_repair_scheduled = false,
    bridge_gender_logged = false,
    bridge_gender_pending_reason = nil,
    rich_text_audit_consumed_bytes = 0,
    draw_hook_registered = false,
    reconcile_started = false,
    reconcile_handle = nil,
    reconcile_job = nil,
    next_reconcile_at = 0.0,
    runtime_profile = runtime_profiles.resolve(
        config.DEFAULT_RUNTIME_PROFILE_ID,
        config.DEFAULT_PERFORMANCE_PRESET_ID
    ),
    selected_profile_id = config.DEFAULT_RUNTIME_PROFILE_ID,
    selected_preset_id = config.DEFAULT_PERFORMANCE_PRESET_ID,
    runtime_master_enabled = true,
    profile_generation = 0,
    control_revision = -1,
    control_pending_logged = false,
    panel_filter_key = "level:nil:nil;distance:nil:nil",
    show_top_guide_line = config.SHOW_TOP_GUIDE_LINE,
    show_name = config.SHOW_NAME,
    show_level = config.SHOW_LEVEL,
    show_distance = config.SHOW_DISTANCE,
    show_iv = config.SHOW_IV,
    show_passive_skills = config.SHOW_PASSIVE_SKILLS,
    iv_min = 0,
    iv_hp_min = 0,
    iv_attack_min = 0,
    iv_defense_min = 0,
    passive_filter_revision = 0,
    gender_filter_id = 0,
    lucky_filter_id = 0,
    boss_filter_id = 0,
    collection_filter_id = 0,
    species_filter_text = "",
    panel_main_page = 0,
    panel_filter_page = 0,
    element_filter_mask = 0,
    language_id = 0,
    settings_path = nil,
    settings_io = nil,
    settings_loaded = nil,
    settings_loaded_version = nil,
    settings_last_serialized = nil,
    settings_pending = nil,
    settings_save_due_at = nil,
    settings_bridge_applied = false,
    settings_save_error_logged = false,
    panel_keybind_registered = false,
    panel_toggle_pending = false,
    panel_toggle_sequence = 0,
    panel_toggle_lifecycle_generation = 0,
    panel_toggle_deferred_logged = false,
    capture_requested = false,
    capture_active = false,
    capture_session_id = 0,
    bootstrap_pending = false,
    lifecycle_generation = 0,
    world_transitioning = false,
    gameplay_active = false,
    last_scan_skip_reason = nil,
    pal_accept_logged = false,
    session_index = 0,
    field_path_logged = {},
    field_states_logged = false,
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
        notifications_deferred = 0,
        event_queue_admitted = 0,
        event_queue_rejected = 0,
        event_queue_overflow = 0,
        event_callback_max_seconds = 0.0,
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
        reconcile_batches = 0,
        reconcile_batch_max_seconds = 0.0,
        reconcile_busy_skips = 0,
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

local function safe_set_property(object, property_name, value)
    object = unwrap(object)
    if not is_valid(object) then
        return false
    end
    local ok = pcall(function()
        object[property_name] = value
    end)
    return ok
end

local SETTINGS_PROPERTIES = {
    { name = "runtime_enabled", property = "ESP_RuntimeEnabled" },
    { name = "profile_id", property = "ESP_ProfileId" },
    { name = "preset_id", property = "ESP_PresetId" },
    { name = "language_id", property = "ESP_LanguageId" },
    { name = "level_min", property = "ESP_LevelMin" },
    { name = "level_max", property = "ESP_LevelMax" },
    { name = "distance_max", property = "ESP_DistanceMax" },
    { name = "display_limit", property = "ESP_DisplayTargetLimit" },
    { name = "show_top", property = "ESP_ShowTopGuideLine" },
    { name = "show_name", property = "ESP_ShowName" },
    { name = "show_level", property = "ESP_ShowLevel" },
    { name = "show_distance", property = "ESP_ShowDistance" },
    { name = "show_iv", property = "ESP_ShowIV" },
    { name = "show_passives", property = "ESP_ShowPassiveSkills" },
    { name = "iv_min", property = "ESP_IvMin" },
    { name = "iv_hp_min", property = "ESP_IvHpMin" },
    { name = "iv_attack_min", property = "ESP_IvAttackMin" },
    { name = "iv_defense_min", property = "ESP_IvDefenseMin" },
    { name = "passive_includes", property = "ESP_PassiveIncludeText" },
    { name = "passive_excludes", property = "ESP_PassiveExcludeText" },
    { name = "expand_rainbow", property = "ESP_PassiveRainbowExpanded" },
    { name = "expand_legend", property = "ESP_PassiveLegendExpanded" },
    { name = "expand_gold3", property = "ESP_PassiveGold3Expanded" },
    { name = "expand_gold2", property = "ESP_PassiveGold2Expanded" },
    { name = "expand_normal", property = "ESP_PassiveNormalExpanded" },
    { name = "expand_negative1", property = "ESP_PassiveNegative1Expanded" },
    { name = "expand_negative2", property = "ESP_PassiveNegative2Expanded" },
    { name = "expand_negative3", property = "ESP_PassiveNegative3Expanded" },
    { name = "gender", property = "ESP_GenderFilterId" },
    { name = "lucky", property = "ESP_LuckyFilterId" },
    { name = "boss", property = "ESP_BossFilterId" },
    { name = "collection", property = "ESP_CollectionFilterId" },
    { name = "species_filters", property = "ESP_SpeciesFilterText" },
    { name = "panel_main_page", property = "ESP_PanelMainPage" },
    { name = "panel_filter_page", property = "ESP_PanelFilterPage" },
    { name = "element_normal", property = "ESP_ElementNormal" },
    { name = "element_fire", property = "ESP_ElementFire" },
    { name = "element_water", property = "ESP_ElementWater" },
    { name = "element_leaf", property = "ESP_ElementLeaf" },
    { name = "element_electricity", property = "ESP_ElementElectricity" },
    { name = "element_ice", property = "ESP_ElementIce" },
    { name = "element_earth", property = "ESP_ElementEarth" },
    { name = "element_dark", property = "ESP_ElementDark" },
    { name = "element_dragon", property = "ESP_ElementDragon" },
}

local function initialize_user_settings()
    local override_path = rawget(_G, "PalworldResourceESPSettingsPath")
    local override_io = rawget(_G, "PalworldResourceESPSettingsIO")
    state.settings_io = type(override_io) == "table" and override_io or io
    if override_path == false then
        debug_event("USER_SETTINGS_DISABLED", "reason=path_override")
        return
    end

    if type(override_path) == "string" and override_path ~= "" then
        state.settings_path = override_path
    else
        local source = nil
        if type(debug) == "table" and type(debug.getinfo) == "function" then
            local ok, info = pcall(debug.getinfo, 1, "S")
            if ok and type(info) == "table" then
                source = info.source
            end
        end
        local package_path = type(package) == "table" and package.path or nil
        local package_searchpath = type(package) == "table" and package.searchpath or nil
        local resolution
        state.settings_path, resolution = user_settings.resolve_path(source, package_path, package_searchpath)
        state.settings_path_resolution = resolution
    end

    if state.settings_path == nil or type(state.settings_io) ~= "table" then
        log_event("USER_SETTINGS_UNAVAILABLE", string.format(
            "reason=%s",
            tostring(state.settings_path_resolution or "storage_path")
        ))
        return
    end

    debug_event("USER_SETTINGS_PATH", string.format(
        "resolution=%s",
        tostring(state.settings_path_resolution or "override")
    ))

    local loaded, load_reason, loaded_version = user_settings.load_latest(state.settings_path, state.settings_io)
    if loaded == nil then
        debug_event("USER_SETTINGS_DEFAULT", string.format("reason=%s", tostring(load_reason)))
        return
    end
    state.settings_loaded = loaded
    state.settings_loaded_version = loaded_version
    state.settings_last_serialized = user_settings.serialize(loaded)
    log_event("USER_SETTINGS_LOADED", string.format("version=%s", tostring(loaded_version)))
end

local function apply_loaded_settings_to_bridge()
    if state.settings_bridge_applied or not is_valid(state.bridge_actor) then
        return state.settings_bridge_applied
    end
    if state.settings_loaded == nil then
        state.settings_bridge_applied = true
        return true
    end

    local previous = {}
    for _, mapping in ipairs(SETTINGS_PROPERTIES) do
        local ok, value = safe_get_property(state.bridge_actor, mapping.property)
        if not ok then
            log_event("USER_SETTINGS_APPLY_FAILED", string.format("property=%s stage=read", mapping.property))
            return false
        end
        previous[mapping.property] = value
    end
    local revision_ok, revision = safe_get_property(state.bridge_actor, "ESP_ControlRevision")
    revision = revision_ok and tonumber(revision) or nil
    if revision == nil then
        log_event("USER_SETTINGS_APPLY_FAILED", "property=ESP_ControlRevision stage=read")
        return false
    end

    local applied = {}
    for _, mapping in ipairs(SETTINGS_PROPERTIES) do
        if not safe_set_property(state.bridge_actor, mapping.property, state.settings_loaded[mapping.name]) then
            for _, property_name in ipairs(applied) do
                safe_set_property(state.bridge_actor, property_name, previous[property_name])
            end
            log_event("USER_SETTINGS_APPLY_FAILED", string.format("property=%s stage=write", mapping.property))
            return false
        end
        applied[#applied + 1] = mapping.property
    end
    local passive_restore_ok = pcall(function()
        local restore = state.bridge_actor.PalworldResourceESP_ApplyPersistedPanelState
        if restore == nil then
            error("restore_event_unavailable")
        end
        restore(state.bridge_actor)
    end)
    if not passive_restore_ok then
        for _, property_name in ipairs(applied) do
            safe_set_property(state.bridge_actor, property_name, previous[property_name])
        end
        log_event("USER_SETTINGS_APPLY_FAILED", "event=PalworldResourceESP_ApplyPersistedPanelState")
        return false
    end
    if not safe_set_property(state.bridge_actor, "ESP_ControlRevision", math.floor(revision) + 1) then
        for _, property_name in ipairs(applied) do
            safe_set_property(state.bridge_actor, property_name, previous[property_name])
        end
        log_event("USER_SETTINGS_APPLY_FAILED", "property=ESP_ControlRevision stage=write")
        return false
    end

    state.settings_bridge_applied = true
    state.control_revision = -1
    log_event("USER_SETTINGS_APPLIED", string.format("version=%s", tostring(state.settings_loaded_version)))
    return true
end

local function schedule_user_settings_save(values)
    if state.settings_path == nil or type(state.settings_io) ~= "table" then
        return
    end
    local normalized = user_settings.normalize(values)
    state.settings_loaded = normalized
    state.settings_loaded_version = "v12"
    local serialized = user_settings.serialize(normalized)
    if serialized == state.settings_last_serialized then
        state.settings_pending = nil
        state.settings_save_due_at = nil
        return
    end
    if state.settings_pending ~= nil
        and serialized == user_settings.serialize(state.settings_pending) then
        return
    end
    state.settings_pending = normalized
    state.settings_save_due_at = now() + 0.75
end

local function flush_user_settings_if_due()
    if state.settings_pending == nil or state.settings_save_due_at == nil
        or now() < state.settings_save_due_at then
        return
    end
    local saved, save_error = user_settings.append(
        state.settings_path,
        state.settings_pending,
        state.settings_io
    )
    if not saved then
        if not state.settings_save_error_logged then
            state.settings_save_error_logged = true
            log_event("USER_SETTINGS_SAVE_FAILED", tostring(save_error))
        end
        state.settings_save_due_at = now() + 5.0
        return
    end
    state.settings_last_serialized = user_settings.serialize(state.settings_pending)
    state.settings_pending = nil
    state.settings_save_due_at = nil
    state.settings_save_error_logged = false
    debug_event("USER_SETTINGS_SAVED", "version=v12")
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

local function classify_actor(actor, player_gate_already_passed)
    actor = unwrap(actor)
    if not is_valid(actor) then
        return "invalid", nil, nil
    end

    if not player_gate_already_passed then
        if is_a(actor, resolve_class("player")) then
            return "player_class", nil, nil
        end
        if is_a(actor, resolve_class("player_controller")) then
            return "player_controller", nil, nil
        end
        if is_a(actor, resolve_class("player_state")) then
            return "player_state", nil, nil
        end
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

    local ok_initialized, initialized = safe_call_no_args(actor, "IsInitialized")
    if not ok_initialized then
        return "character_initialization_unavailable", component, nil
    end
    if initialized ~= true then
        return "character_not_initialized", component, nil
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

local function actor_full_name(actor)
    local ok, full_name = safe_call_no_args(actor, "GetFullName")
    if ok and type(full_name) == "string" and full_name ~= "" then
        return full_name
    end
    return nil
end

local function actor_key(actor)
    local full_name = actor_full_name(actor)
    if full_name == nil then
        return nil
    end
    local ok, address = safe_call_no_args(actor, "GetAddress")
    if not ok or type(address) ~= "number" or address == 0 then
        return nil
    end
    return string.format("%s @0x%X", full_name, address)
end

local function runtime_enabled()
    return config.ENABLED and state.runtime_profile.runtime_enabled
end

local function clear_event_queue(reason)
    local pending = math.max(0, #state.event_queue - state.event_queue_head + 1)
    state.event_queue = {}
    state.event_queue_head = 1
    state.event_queue_scheduled = false
    state.event_queue_overflow_logged = false
    state.event_queue_keys = {}
    if pending > 0 then
        debug_event("EVENT_QUEUE_CLEARED", string.format("reason=%s pending=%d", reason, pending))
    end
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

local function probe_entity_core_fields(parameter)
    local found, value, path = probe_source_value(parameter, {
        methods = { "GetLevel" },
    })
    local fields = {
        level = {
            available = found,
            value = value,
            path = found and ("parameter." .. path) or nil,
            note = "Lua-safe scalar required by Entity Core",
        },
    }

    if not state.field_path_logged.level then
        state.field_path_logged.level = true
        if found then
            log_event("FIELD_PATH", string.format(
                "field=level status=available path=%s value=%s",
                fields.level.path,
                value_to_text(value)
            ))
        else
            log_event("FIELD_PATH", "field=level status=unavailable note=Lua-safe scalar not available")
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
    local reconcile_job = state.reconcile_job
    if reconcile_job ~= nil then
        reconcile_job.cancelled = true
        reconcile_job.objects = {}
        reconcile_job.generation = nil
        state.reconcile_job = nil
        debug_event("RECONCILE_CANCELLED", string.format("reason=%s", reason))
    end
    entity_snapshot.clear(state.entity_store, reason)
    state.candidates = {}
    state.candidate_count = 0
    state.matched_records = {}
    state.display_records = {}
    state.nearest_candidates = {}
    state.nearest_candidate_count = 0
    state.nearest_candidate_order = {}
    state.nearest_refresh_cursor = 1
    state.nearest_index_dirty = true
    state.next_nearest_refresh_at = 0.0
    state.atomic_rebuild_dirty = false
    state.atomic_rebuild_reason = nil
    state.atomic_rebuild_due_at = 0.0
    state.atomic_rebuild_deadline_at = 0.0
    state.atomic_rebuild_revision = state.atomic_rebuild_revision + 1
    state.next_atomic_rebuild_at = 0.0
    state.cached_bridge_resync_dirty = false
    state.cached_bridge_resync_force = false
    state.cached_bridge_resync_reason = nil
    state.cached_bridge_resync_revision = state.cached_bridge_resync_revision + 1
    state.stream_integrity_last_location = nil
    state.stream_integrity_distance_cm = 0.0
    state.next_stream_integrity_scan_at = 0.0
    state.metrics.admitted = 0
    state.metrics.matched = 0
    state.metrics.displayed = 0
    state.metrics.display_truncated = 0
    state.notification_dirty = false
    state.selected = nil
    clear_event_queue(reason)

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

local function copy_location_scalars(location)
    local x, y, z = read_xyz(location)
    if x == nil then
        return nil
    end
    return { X = x, Y = y, Z = z }
end

local function reset_stream_integrity_tracking(camera_location)
    state.stream_integrity_last_location = copy_location_scalars(camera_location)
    state.stream_integrity_distance_cm = 0.0
    state.next_stream_integrity_scan_at = now()
        + (math.max(0, tonumber(config.STREAM_INTEGRITY_SCAN_MIN_INTERVAL_MS) or 0) / 1000.0)
end

local function track_stream_integrity_movement(camera_location)
    local current_location = copy_location_scalars(camera_location)
    if current_location == nil then
        return false
    end
    if state.stream_integrity_last_location == nil then
        reset_stream_integrity_tracking(current_location)
        return false
    end

    local segment_squared = distance_squared(state.stream_integrity_last_location, current_location)
    state.stream_integrity_last_location = current_location
    if segment_squared ~= nil and segment_squared > 0 then
        state.stream_integrity_distance_cm = state.stream_integrity_distance_cm
            + math.sqrt(segment_squared)
    end
    local threshold_cm = math.max(
        0,
        tonumber(config.STREAM_INTEGRITY_SCAN_DISTANCE_METERS) or 0
    ) * 100.0
    return threshold_cm > 0 and state.stream_integrity_distance_cm >= threshold_cm
end

local function request_atomic_bridge_rebuild(reason, debounce_ms)
    if not runtime_enabled() or state.world_transitioning or not state.gameplay_active then
        return false
    end

    local was_dirty = state.atomic_rebuild_dirty
    local delay = math.max(
        0,
        tonumber(debounce_ms) or tonumber(config.ATOMIC_REBUILD_DEBOUNCE_MS) or 0
    ) / 1000.0
    local due_at = now() + delay
    state.atomic_rebuild_dirty = true
    state.atomic_rebuild_reason = reason or state.atomic_rebuild_reason or "lifecycle"
    if was_dirty then
        state.atomic_rebuild_due_at = math.min(
            state.atomic_rebuild_deadline_at,
            math.max(state.atomic_rebuild_due_at, due_at)
        )
    else
        local max_coalesce = math.max(
            delay * 1000.0,
            tonumber(config.ATOMIC_REBUILD_MAX_COALESCE_MS) or 0
        ) / 1000.0
        state.atomic_rebuild_due_at = due_at
        state.atomic_rebuild_deadline_at = now() + max_coalesce
    end
    state.atomic_rebuild_revision = state.atomic_rebuild_revision + 1
    if not was_dirty then
        debug_event("ATOMIC_REBUILD_REQUESTED", string.format(
            "reason=%s debounce_ms=%d",
            state.atomic_rebuild_reason,
            math.floor(delay * 1000.0 + 0.5)
        ))
    end
    return true
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
    classify_pal = function(actor)
        return classify_actor(actor, true)
    end,
    probe_pal_fields = function(_actor, _component, parameter, _save_parameter)
        if not config.FIELD_PROBES_ENABLED then
            return {}
        end
        return probe_entity_core_fields(parameter)
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
        return false, state.world_transitioning and "world_transitioning" or "gameplay_inactive"
    end

    actor = unwrap(actor)
    if not runtime_enabled() then
        return false, "runtime_inactive"
    end
    if not is_valid(actor) then
        return false, "actor_invalid"
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
        return false, "admission_budget"
    end

    context = context or {}
    context.source = source
    context.accepted_at = now()
    local normalized, rejection = state.entity_registry:admit(entity_runtime, actor, context)
    if normalized == nil then
        local rejection_reason = rejection and rejection.reason or "adapter_rejected"
        record_rejection(rejection_reason)
        return false, rejection_reason
    end

    local added, record = entity_snapshot.add(generation, normalized)
    if not added then
        return false, "duplicate"
    end

    state.metrics.accepted = state.metrics.accepted + 1
    if not state.field_states_logged then
        state.field_states_logged = true
        for _, field_name in ipairs(NORMALIZED_FIELD_NAMES) do
            local cell = record.fields[field_name]
            if cell ~= nil then
                log_event("FIELD_STATE", string.format(
                    "field=%s state=%s detail=%s",
                    field_name,
                    cell.state,
                    cell.source or cell.reason or "unspecified"
                ))
            end
        end
    end
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
    return true, nil
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
        state.notification_dirty = false
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
    state.notification_dirty = false
    refresh_pipeline_output(source)

    local elapsed = os.clock() - started_at
    state.metrics.raw_monsters = raw_count
    state.metrics.scan_seconds = state.metrics.scan_seconds + elapsed
    reset_stream_integrity_tracking(context.camera_location)
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
        "session=%d generation=%d profile=%s preset=%s raw=%d admitted=%d matched=%d displayed=%d admission_rejected=%d display_truncated=%d accepted_total=%d rejected_player=%d rejected_owned=%d rejected_dead=%d rejected_unknown=%d notifications=%d notifications_deferred=%d event_admitted=%d event_rejected=%d event_overflow=%d event_max_ms=%.3f scans=%d reconcile_batches=%d max_batch_ms=%.3f busy_skips=%d scan_ms=%.3f discovery_ms=%.3f admission_ms=%.3f filter_ms=%.3f order_ms=%.3f budget_ms=%.3f bridge_syncs=%d bridge_set_calls=%d bridge_sync_ms=%.3f",
        state.session_index,
        current.generation_id,
        state.runtime_profile.name,
        state.runtime_profile.preset_name,
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
        state.metrics.notifications_deferred,
        state.metrics.event_queue_admitted,
        state.metrics.event_queue_rejected,
        state.metrics.event_queue_overflow,
        state.metrics.event_callback_max_seconds * 1000.0,
        state.metrics.scan_count,
        state.metrics.reconcile_batches,
        state.metrics.reconcile_batch_max_seconds * 1000.0,
        state.metrics.reconcile_busy_skips,
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

local admit_begin_play_target

local refresh_bridge_target

local remove_bridge_target

local refresh_nearest_bridge_targets

local process_reconcile_batch

local process_event_queue

--[[ __DEPRECATED_20260720__ [reason: delayed callbacks must not retain UObject wrappers]
local function event_queue_has_pending()
    return state.event_queue_head <= #state.event_queue
end

local function schedule_event_queue_process()
    if state.event_queue_scheduled or not event_queue_has_pending() then
        return
    end
    if type(ExecuteInGameThreadWithDelay) ~= "function" then
        state.notification_dirty = true
        return
    end

    state.event_queue_scheduled = true
    local profile_generation = state.profile_generation
    local ok, err = pcall(function()
        ExecuteInGameThreadWithDelay(config.EVENT_QUEUE_DELAY_MS, function()
            process_event_queue(profile_generation)
        end)
    end)
    if not ok then
        state.event_queue_scheduled = false
        state.notification_dirty = true
        log_event("EVENT_QUEUE_SCHEDULE_FAILED", tostring(err))
    end
end

process_event_queue = function(profile_generation)
    state.event_queue_scheduled = false
    if profile_generation ~= state.profile_generation
        or not runtime_enabled()
        or not state.runtime_profile.event_admission
        or state.world_transitioning
        or not state.gameplay_active then
        clear_event_queue("stale_event_callback")
        return
    end

    if state.reconcile_job ~= nil then
        schedule_event_queue_process()
        return
    end

    local actor = state.event_queue[state.event_queue_head]
    state.event_queue_head = state.event_queue_head + 1

    local started_at = os.clock()
    local added = false
    local ok, err = pcall(function()
        local generation = entity_snapshot.current(state.entity_store)
        added = add_entity_candidate(actor, "notify_queue", generation, {
            source = "notify_queue",
            camera_location = get_camera_location(find_player_controller()),
        })
        if added then
            refresh_pipeline_output("notify_queue")
            sync_bridge_target()
        end
    end)
    local elapsed = os.clock() - started_at
    state.metrics.event_callback_max_seconds = math.max(
        state.metrics.event_callback_max_seconds,
        elapsed
    )
    if ok and added then
        state.metrics.event_queue_admitted = state.metrics.event_queue_admitted + 1
    else
        state.metrics.event_queue_rejected = state.metrics.event_queue_rejected + 1
        if not ok then
            log_event("ERROR_EVENT_CANDIDATE", tostring(err))
        end
    end

    if not event_queue_has_pending() then
        state.event_queue = {}
        state.event_queue_head = 1
        return
    end
    schedule_event_queue_process()
end
]]

local RETRYABLE_EVENT_REJECTIONS = {
    actor_not_resolved = true,
    bridge_unavailable = true,
    gameplay_inactive = true,
    world_transitioning = true,
    parameter_component_unavailable = true,
    character_initialization_pending = true,
    character_initialization_unavailable = true,
    character_not_initialized = true,
    individual_parameter_unavailable = true,
    save_parameter_unavailable = true,
    owner_uid_unavailable = true,
    stream_integrity_discovered = true,
    trainer_unavailable = true,
}

local function event_rejection_is_retryable(reason)
    return RETRYABLE_EVENT_REJECTIONS[reason] == true
end

local function event_queue_has_pending()
    return state.event_queue_head <= #state.event_queue
end

local function event_queue_pending_count()
    return math.max(0, #state.event_queue - state.event_queue_head + 1)
end

local function actor_lookup_identity(actor)
    local full_name = actor_full_name(actor)
    local key = actor_key(actor)
    if key == nil or full_name == nil then
        return nil, nil
    end
    local object_path = full_name:match("^%S+%s+(.+)$")
    if object_path == nil or object_path == "" then
        return nil, nil
    end
    return key, object_path
end

local function remember_bridge_target(actor, key)
    local _, object_path = actor_lookup_identity(actor)
    state.bridge_target_keys[key] = object_path or true
    state.bridge_target_order[#state.bridge_target_order + 1] = key
end

local function forget_bridge_target(key)
    state.bridge_target_keys[key] = nil
    for index, ordered_key in ipairs(state.bridge_target_order) do
        if ordered_key == key then
            table.remove(state.bridge_target_order, index)
            break
        end
    end
end

local function resolve_event_actor(object_path)
    if type(StaticFindObject) ~= "function" or type(object_path) ~= "string" then
        return nil
    end
    local ok, actor = pcall(StaticFindObject, object_path)
    actor = ok and unwrap(actor) or nil
    if is_valid(actor) then
        return actor
    end
    return nil
end

local function schedule_event_queue_process(delay_ms)
    if state.event_queue_scheduled or not event_queue_has_pending() then
        return
    end
    if type(ExecuteInGameThreadWithDelay) ~= "function" then
        log_event("EVENT_QUEUE_UNAVAILABLE", "api=ExecuteInGameThreadWithDelay")
        return
    end

    local delay = math.max(
        config.EVENT_QUEUE_DELAY_MS,
        math.floor(tonumber(delay_ms) or config.EVENT_QUEUE_DELAY_MS)
    )
    state.event_queue_scheduled = true
    local profile_generation = state.profile_generation
    local ok, err = pcall(function()
        ExecuteInGameThreadWithDelay(delay, function()
            process_event_queue(profile_generation)
        end)
    end)
    if not ok then
        state.event_queue_scheduled = false
        log_event("EVENT_QUEUE_SCHEDULE_FAILED", tostring(err))
    end
end

local function enqueue_event_retry(actor, rejection_reason, mode)
    actor = unwrap(actor)
    if not is_valid(actor) or not event_rejection_is_retryable(rejection_reason) then
        return false
    end

    mode = mode == "refresh" and "refresh" or "admit"
    local key, object_path = actor_lookup_identity(actor)
    local already_eligible = key ~= nil and state.nearest_candidates[key] ~= nil
    if key == nil or state.event_queue_keys[key]
        or (mode == "admit" and already_eligible)
        or (mode == "refresh" and not already_eligible) then
        return false
    end
    if event_queue_pending_count() >= config.MAX_EVENT_QUEUE then
        state.metrics.event_queue_overflow = state.metrics.event_queue_overflow + 1
        if not state.event_queue_overflow_logged then
            state.event_queue_overflow_logged = true
            log_event("EVENT_QUEUE_OVERFLOW", string.format("limit=%d", config.MAX_EVENT_QUEUE))
        end
        return false
    end

    local queue_was_empty = not event_queue_has_pending()
    state.event_queue[#state.event_queue + 1] = {
        key = key,
        object_path = object_path,
        attempts = 0,
        mode = mode,
        lifecycle_generation = state.lifecycle_generation,
    }
    state.event_queue_keys[key] = true
    state.metrics.notifications_deferred = state.metrics.notifications_deferred + 1
    if queue_was_empty then
        log_event("EVENT_READINESS_QUEUE_STARTED", string.format(
            "reason=%s mode=%s retry_delay_ms=%d max_attempts=%d",
            rejection_reason,
            mode,
            config.EVENT_READINESS_RETRY_DELAY_MS,
            config.MAX_EVENT_READINESS_ATTEMPTS
        ))
    end
    schedule_event_queue_process(config.EVENT_READINESS_RETRY_DELAY_MS)
    return true
end

local function finish_event_queue_entry(entry)
    if entry ~= nil and entry.key ~= nil then
        state.event_queue_keys[entry.key] = nil
    end
end

process_event_queue = function(profile_generation)
    state.event_queue_scheduled = false
    if profile_generation ~= state.profile_generation
        or not runtime_enabled()
        or not state.runtime_profile.event_admission then
        clear_event_queue("stale_event_callback")
        return
    end
    if not event_queue_has_pending() then
        clear_event_queue("event_queue_empty")
        return
    end
    if state.world_transitioning or not state.gameplay_active then
        schedule_event_queue_process(config.EVENT_READINESS_RETRY_DELAY_MS)
        return
    end
    if state.reconcile_job ~= nil then
        schedule_event_queue_process(config.EVENT_QUEUE_DELAY_MS)
        return
    end

    local entry = state.event_queue[state.event_queue_head]
    state.event_queue_head = state.event_queue_head + 1
    if entry == nil or entry.key == nil or not state.event_queue_keys[entry.key]
        or entry.lifecycle_generation ~= state.lifecycle_generation then
        finish_event_queue_entry(entry)
    else
        local started_at = os.clock()
        local actor = resolve_event_actor(entry.object_path)
        local added = false
        local rejection_reason = "actor_not_resolved"
        local ok, err = true, nil
        if actor ~= nil and actor_key(actor) == entry.key then
            ok, added, rejection_reason = pcall(function()
                if entry.mode == "refresh" then
                    return refresh_bridge_target(actor, "initialization_retry")
                end
                return admit_begin_play_target(actor, "begin_play_retry")
            end)
        elseif actor ~= nil then
            rejection_reason = "actor_instance_replaced"
        end
        local elapsed = os.clock() - started_at
        state.metrics.event_callback_max_seconds = math.max(
            state.metrics.event_callback_max_seconds,
            elapsed
        )

        if ok and added then
            finish_event_queue_entry(entry)
        elseif ok and event_rejection_is_retryable(rejection_reason)
            and entry.attempts + 1 < config.MAX_EVENT_READINESS_ATTEMPTS then
            entry.attempts = entry.attempts + 1
            state.event_queue[#state.event_queue + 1] = entry
        else
            finish_event_queue_entry(entry)
            state.metrics.event_queue_rejected = state.metrics.event_queue_rejected + 1
            if not ok then
                log_event("ERROR_EVENT_CANDIDATE", tostring(err))
            else
                debug_event("EVENT_TARGET_DROPPED", string.format(
                    "reason=%s mode=%s attempts=%d",
                    rejection_reason or "rejected",
                    entry.mode or "admit",
                    entry.attempts + 1
                ))
            end
        end
    end

    if not event_queue_has_pending() then
        state.event_queue = {}
        state.event_queue_head = 1
        return
    end
    local next_delay = config.EVENT_QUEUE_DELAY_MS
    if event_queue_pending_count() == 1 then
        next_delay = config.EVENT_READINESS_RETRY_DELAY_MS
    end
    schedule_event_queue_process(next_delay)
end

local function abandon_reconcile_job(job, reason)
    if state.reconcile_job ~= job then
        return
    end
    job.cancelled = true
    job.objects = {}
    job.generation = nil
    state.reconcile_job = nil
    debug_event("RECONCILE_CANCELLED", string.format("reason=%s", reason))
end

local function reconcile_job_is_current(job)
    return state.reconcile_job == job
        and not job.cancelled
        and runtime_enabled()
        and not state.world_transitioning
        and state.gameplay_active
        and job.lifecycle_generation == state.lifecycle_generation
        and job.profile_generation == state.profile_generation
        and job.session_id == state.entity_store.session_id
end

local function finish_reconcile_job(job)
    if not reconcile_job_is_current(job) then
        abandon_reconcile_job(job, "stale_before_finish")
        return
    end

    state.metrics.raw_monsters = #job.objects
    entity_snapshot.replace(state.entity_store, job.generation)
    state.notification_dirty = state.metrics.notifications_deferred > job.notification_version

    local pipeline_started_at = os.clock()
    refresh_pipeline_output(job.source)
    state.nearest_index_dirty = true
    local pipeline_seconds = os.clock() - pipeline_started_at
    local scan_seconds = job.discovery_seconds + job.admission_seconds + pipeline_seconds
    state.metrics.scan_seconds = state.metrics.scan_seconds + scan_seconds

    local raw_count = #job.objects
    local batch_count = job.batch_count
    local max_batch_seconds = job.max_batch_seconds
    job.objects = {}
    job.generation = nil
    state.reconcile_job = nil
    reset_stream_integrity_tracking(get_camera_location(find_player_controller()))

    log_event("SCAN_DONE", string.format(
        "source=%s raw=%d admitted=%d matched=%d displayed=%d elapsed_ms=%.3f batches=%d max_batch_ms=%.3f",
        job.source,
        raw_count,
        state.metrics.admitted,
        state.metrics.matched,
        state.metrics.displayed,
        scan_seconds * 1000.0,
        batch_count,
        max_batch_seconds * 1000.0
    ))

    if config.DRAW_ENABLED then
        select_target()
    end
    sync_bridge_target()
    emit_metrics(false)
end

local function process_reconcile_immediately(job)
    if not reconcile_job_is_current(job) then
        abandon_reconcile_job(job, "stale_immediate_scan")
        return false
    end

    local admission_started_at = os.clock()
    for _, actor in ipairs(job.objects) do
        local added_ok, added_error = pcall(function()
            add_entity_candidate(actor, job.source, job.generation, job.context)
        end)
        if not added_ok then
            log_event("ERROR_CANDIDATE", tostring(added_error))
        end
    end
    local admission_seconds = os.clock() - admission_started_at
    job.admission_seconds = admission_seconds
    job.batch_count = #job.objects > 0 and 1 or 0
    job.max_batch_seconds = admission_seconds
    job.index = #job.objects + 1
    state.metrics.admission_seconds = state.metrics.admission_seconds + admission_seconds
    state.metrics.reconcile_batches = state.metrics.reconcile_batches + job.batch_count
    state.metrics.reconcile_batch_max_seconds = math.max(
        state.metrics.reconcile_batch_max_seconds,
        admission_seconds
    )
    finish_reconcile_job(job)
    return true
end

--[[ __DEPRECATED_20260717__ [reason: retaining UE4SS UObject wrappers across delayed callbacks caused GameThread access violations]
local function schedule_reconcile_batch(job)
    if not reconcile_job_is_current(job) then
        abandon_reconcile_job(job, "stale_before_schedule")
        return false
    end
    if type(ExecuteInGameThreadWithDelay) ~= "function" then
        log_event("ERROR_RECONCILE_BATCH", "reason=scheduler_unavailable")
        abandon_reconcile_job(job, "scheduler_unavailable")
        return false
    end

    local delay_ms = job.batch_delay_ms
    local ok, err = pcall(function()
        ExecuteInGameThreadWithDelay(delay_ms, function()
            local batch_ok, batch_error = pcall(function()
                process_reconcile_batch(job)
            end)
            if not batch_ok then
                log_event("ERROR_RECONCILE_BATCH", tostring(batch_error))
                abandon_reconcile_job(job, "batch_callback_error")
            end
        end)
    end)
    if not ok then
        log_event("ERROR_RECONCILE_BATCH", tostring(err))
        abandon_reconcile_job(job, "batch_schedule_error")
        return false
    end
    return true
end

process_reconcile_batch = function(job)
    if not reconcile_job_is_current(job) then
        abandon_reconcile_job(job, "stale_batch")
        return
    end

    local batch_started_at = os.clock()
    local batch_size = job.batch_size
    local last_index = math.min(#job.objects, job.index + batch_size - 1)
    for index = job.index, last_index do
        local actor = job.objects[index]
        local added_ok, added_error = pcall(function()
            add_entity_candidate(actor, job.source, job.generation, job.context)
        end)
        if not added_ok then
            log_event("ERROR_CANDIDATE", tostring(added_error))
        end
    end
    job.index = last_index + 1

    local batch_seconds = os.clock() - batch_started_at
    job.admission_seconds = job.admission_seconds + batch_seconds
    job.batch_count = job.batch_count + 1
    job.max_batch_seconds = math.max(job.max_batch_seconds, batch_seconds)
    state.metrics.admission_seconds = state.metrics.admission_seconds + batch_seconds
    state.metrics.reconcile_batches = state.metrics.reconcile_batches + 1
    state.metrics.reconcile_batch_max_seconds = math.max(
        state.metrics.reconcile_batch_max_seconds,
        batch_seconds
    )

    if job.index > #job.objects then
        finish_reconcile_job(job)
        return
    end
    schedule_reconcile_batch(job)
end
]]

local function start_safe_reconcile(source)
    if state.reconcile_job ~= nil then
        state.metrics.reconcile_busy_skips = state.metrics.reconcile_busy_skips + 1
        debug_event("RECONCILE_SKIPPED", "reason=job_in_progress")
        return false
    end

    local descriptor = state.entity_registry:get("pal")
    source = source or "reconcile"
    local generation = entity_snapshot.begin_generation(state.entity_store, source, now())
    state.metrics.scan_count = state.metrics.scan_count + 1
    state.metrics.admission_rejected = 0

    local discovery_started_at = os.clock()
    local ok, objects = pcall(function()
        return descriptor and descriptor.find_all(entity_runtime) or nil
    end)
    local discovery_seconds = os.clock() - discovery_started_at
    state.metrics.discovery_seconds = state.metrics.discovery_seconds + discovery_seconds
    if not ok or type(objects) ~= "table" then
        objects = {}
    end

    local job = {
        source = source,
        objects = objects,
        generation = generation,
        context = {
            source = source,
            camera_location = get_camera_location(find_player_controller()),
        },
        index = 1,
        session_id = state.entity_store.session_id,
        lifecycle_generation = state.lifecycle_generation,
        profile_generation = state.profile_generation,
        notification_version = state.metrics.notifications_deferred,
        discovery_seconds = discovery_seconds,
        admission_seconds = 0.0,
        batch_count = 0,
        max_batch_seconds = 0.0,
        batch_size = math.max(1, math.floor(tonumber(state.runtime_profile.batch_size) or 1)),
        batch_delay_ms = math.max(0, tonumber(state.runtime_profile.batch_delay_ms) or 0),
        cancelled = false,
    }
    state.reconcile_job = job

    if #objects == 0 then
        finish_reconcile_job(job)
        return true
    end
    debug_event("RECONCILE_IMMEDIATE", string.format(
        "raw=%d reason=wrapper_lifetime_safety",
        #objects
    ))
    return process_reconcile_immediately(job)
end

local function process_atomic_bridge_rebuild(current_time)
    if not state.atomic_rebuild_dirty
        or state.reconcile_job ~= nil
        or current_time < state.atomic_rebuild_due_at
        or current_time < state.next_atomic_rebuild_at then
        return false
    end

    local reason = state.atomic_rebuild_reason or "lifecycle"
    local revision = state.atomic_rebuild_revision
    state.atomic_rebuild_dirty = false
    state.atomic_rebuild_reason = nil
    state.atomic_rebuild_due_at = 0.0
    state.atomic_rebuild_deadline_at = 0.0

    local started_at = os.clock()
    local rebuilt = start_safe_reconcile("atomic_" .. reason)
    if not rebuilt then
        state.atomic_rebuild_dirty = true
        state.atomic_rebuild_reason = reason
        state.atomic_rebuild_due_at = current_time
        return false
    end

    state.next_atomic_rebuild_at = now()
        + (math.max(0, tonumber(config.ATOMIC_REBUILD_MIN_INTERVAL_MS) or 0) / 1000.0)
    log_event("ATOMIC_REBUILD_COMPLETED", string.format(
        "reason=%s revision=%d active=%d elapsed_ms=%.3f",
        reason,
        revision,
        state.bridge_target_count,
        (os.clock() - started_at) * 1000.0
    ))
    return true
end

-- __DEPRECATED_20260717__ [reason: delayed chunking was replaced by same-callback wrapper-safe admission]
local start_chunked_reconcile = start_safe_reconcile

local function reconcile()
    if not runtime_enabled() then
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
    --[[ __DEPRECATED_20260716__ [reason: synchronous full admission caused 200-300 ms GameThread stalls]
    scan_monsters("reconcile")
    if config.DRAW_ENABLED then
        select_target()
    end
    sync_bridge_target()
    emit_metrics(false)
    ]]
    start_safe_reconcile()
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
    state.bridge_target_keys = {}
    state.bridge_target_order = {}
    state.bridge_target_order = {}
    state.bridge_gender_logged = false
    state.bridge_gender_pending_reason = nil
    state.rich_text_audit_consumed_bytes = 0
    state.settings_bridge_applied = false
    state.control_revision = -1
    state.control_pending_logged = false

    if had_bridge then
        debug_event("BRIDGE_CLEARED", string.format("reason=%s", reason))
    end
end

-- __DEPRECATED_20260721__ [reason: nearest-candidate repair rebuilds from resolvable paths without FindAllOf]
local function schedule_bridge_repair(reason)
    if state.bridge_repair_scheduled then
        return
    end

    state.bridge_repair_scheduled = true
    local lifecycle_generation = state.lifecycle_generation
    local profile_generation = state.profile_generation
    local callback = function()
        state.bridge_repair_scheduled = false
        if lifecycle_generation ~= state.lifecycle_generation
            or profile_generation ~= state.profile_generation
            or not runtime_enabled()
            or state.world_transitioning
            or not state.gameplay_active then
            debug_event("BRIDGE_REPAIR_SKIPPED", string.format("reason=%s", reason))
            return
        end
        if not start_safe_reconcile("bridge_identity_repair") then
            debug_event("BRIDGE_REPAIR_DEFERRED", string.format("reason=%s", reason))
        end
    end

    if type(ExecuteInGameThreadWithDelay) == "function" then
        local ok, err = pcall(function()
            ExecuteInGameThreadWithDelay(config.EVENT_READINESS_RETRY_DELAY_MS, callback)
        end)
        if ok then
            debug_event("BRIDGE_REPAIR_SCHEDULED", string.format("reason=%s", reason))
            return
        end
        state.bridge_repair_scheduled = false
        log_event("BRIDGE_REPAIR_SCHEDULE_FAILED", tostring(err))
    end
    callback()
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

            if is_a(actor, resolve_class("monster")) then
                -- __DEPRECATED_20260721__ [reason: BeginPlay no longer promotes one Pal to a full world snapshot]
                -- request_atomic_bridge_rebuild("begin_play")
                local key = actor_key(actor)
                local mode = key ~= nil and state.nearest_candidates[key] ~= nil
                    and "refresh"
                    or "admit"
                enqueue_event_retry(actor, "character_initialization_pending", mode)
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
            state.bridge_target_keys = {}
            state.bridge_target_order = {}
            state.bridge_gender_logged = false
            state.bridge_gender_pending_reason = nil
            state.rich_text_audit_consumed_bytes = 0
            state.settings_bridge_applied = false
            state.control_revision = -1
            state.control_pending_logged = false
            log_event("BRIDGE_READY", "mode=lua_discovery class=ModActor_C")
            apply_loaded_settings_to_bridge()
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

-- __DEPRECATED_20260721__ [reason: recency-based rolling eviction was replaced by nearest-N membership]
local function fail_closed_bridge_targets(reason)
    local cleared = call_bridge(BRIDGE_METHOD_CLEAR_TARGET, state.session_index)
    state.bridge_target_count = 0
    state.bridge_target_keys = {}
    state.bridge_target_order = {}
    log_event("BRIDGE_TARGET_REPAIR", string.format(
        "session=%d reason=%s bridge_cleared=%s",
        state.session_index,
        reason,
        tostring(cleared)
    ))
    schedule_bridge_repair(reason)
end

-- __DEPRECATED_20260721__ [reason: display membership is now ordered by current distance]
local function evict_oldest_bridge_target()
    while #state.bridge_target_order > 0 do
        local key = table.remove(state.bridge_target_order, 1)
        local object_path = state.bridge_target_keys[key]
        if object_path ~= nil then
            local actor = type(object_path) == "string" and resolve_event_actor(object_path) or nil
            if actor == nil then
                fail_closed_bridge_targets("unresolved_display_window_target")
                return false
            end
            if not call_bridge(BRIDGE_METHOD_REMOVE_TARGET, actor, state.session_index) then
                table.insert(state.bridge_target_order, 1, key)
                return false
            end
            state.bridge_target_keys[key] = nil
            state.bridge_target_count = math.max(0, state.bridge_target_count - 1)
            log_event("EVENT_TARGET_EVICTED", string.format(
                "session=%d active=%d reason=display_window_rollover",
                state.session_index,
                state.bridge_target_count
            ))
            return true
        end
    end
    fail_closed_bridge_targets("inconsistent_display_window_state")
    return false
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

local function display_snapshot_integer(record, field_name)
    local cell = record and record.fields and record.fields[field_name] or nil
    local value = cell and cell.state == "known" and cell.value or nil
    if type(value) ~= "number" or value ~= value or value == math.huge or value == -math.huge then
        return -1
    end
    if value < 0 then
        return -1
    end
    return math.floor(value + 0.5)
end

local function filters_without_current_distance()
    local filters = { fields = {} }
    if type(config.ACTIVE_FILTERS.kinds) == "table" then
        filters.kinds = config.ACTIVE_FILTERS.kinds
    end
    for field_name, condition in pairs(config.ACTIVE_FILTERS.fields or {}) do
        if field_name ~= "distance_m" then
            filters.fields[field_name] = condition
        end
    end
    return filters
end

local function remember_nearest_candidate(actor, record)
    local key, object_path = actor_lookup_identity(actor)
    if key == nil or object_path == nil then
        return nil, "actor_identity_unavailable"
    end

    local candidate = state.nearest_candidates[key]
    if candidate == nil then
        if state.nearest_candidate_count >= config.MAX_ADMITTED_ENTITIES then
            return nil, "admission_budget"
        end
        candidate = {}
        state.nearest_candidates[key] = candidate
        state.nearest_candidate_count = state.nearest_candidate_count + 1
        state.nearest_candidate_order[#state.nearest_candidate_order + 1] = key
    end
    candidate.object_path = object_path
    candidate.level = display_snapshot_integer(record, "level")
    local location = get_actor_location(actor)
    local x, y, z = read_xyz(location)
    if x ~= nil then
        candidate.location_x = x
        candidate.location_y = y
        candidate.location_z = z
    end
    candidate.resolve_misses = 0
    return key, nil
end

local function forget_nearest_candidate(key)
    if key == nil or state.nearest_candidates[key] == nil then
        return false
    end
    state.nearest_candidates[key] = nil
    state.nearest_candidate_count = math.max(0, state.nearest_candidate_count - 1)
    for index, ordered_key in ipairs(state.nearest_candidate_order) do
        if ordered_key == key then
            table.remove(state.nearest_candidate_order, index)
            if index < state.nearest_refresh_cursor then
                state.nearest_refresh_cursor = state.nearest_refresh_cursor - 1
            end
            break
        end
    end
    if state.nearest_refresh_cursor > #state.nearest_candidate_order then
        state.nearest_refresh_cursor = 1
    end
    return true
end

local function rebuild_nearest_candidate_index(records)
    state.nearest_candidates = {}
    state.nearest_candidate_count = 0
    state.nearest_candidate_order = {}
    state.nearest_refresh_cursor = 1
    local eligible = filter_engine.filter(records or {}, filters_without_current_distance())
    for _, record in ipairs(eligible) do
        if record ~= nil and is_valid(record.actor) then
            remember_nearest_candidate(record.actor, record)
        end
    end
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

    if state.nearest_index_dirty then
        local current = entity_snapshot.current(state.entity_store)
        rebuild_nearest_candidate_index(current.records)
        state.nearest_index_dirty = false
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
        state.bridge_target_keys = {}
        state.bridge_target_order = {}
        debug_event("BRIDGE_SESSION", string.format("session=%d", session_index))
    end

    if state.nearest_index_dirty then
        local current = entity_snapshot.current(state.entity_store)
        rebuild_nearest_candidate_index(current.records)
        state.nearest_index_dirty = false
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
    state.bridge_target_count = 0
    state.bridge_target_keys = {}
    state.bridge_target_order = {}

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
            local level = display_snapshot_integer(record, "level")
            local distance = display_snapshot_integer(record, "distance_m")
            if not call_bridge(BRIDGE_METHOD_SET_TARGET, record.actor, session_index, level, distance) then
                state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds + (os.clock() - started_at)
                return
            end
            local key = actor_key(record.actor)
            if key ~= nil then
                remember_bridge_target(record.actor, key)
            end
            submitted = submitted + 1
            state.metrics.bridge_set_calls = state.metrics.bridge_set_calls + 1
        end
    end

    state.bridge_target_count = submitted
    state.next_nearest_refresh_at = now() + (config.NEAREST_TARGET_REFRESH_INTERVAL_MS / 1000.0)
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

local function ensure_bridge_session()
    if not config.BLUEPRINT_BRIDGE_ENABLED or not is_valid(state.bridge_actor) then
        return false
    end
    if state.bridge_session_sent == state.session_index then
        return true
    end
    if not call_bridge(BRIDGE_METHOD_RESET_SESSION, state.session_index) then
        return false
    end
    state.bridge_session_sent = state.session_index
    state.bridge_target_count = 0
    state.bridge_target_keys = {}
    state.bridge_target_order = {}
    debug_event("BRIDGE_SESSION", string.format("session=%d", state.session_index))
    return true
end

local function current_distance_matches(distance)
    local condition = config.ACTIVE_FILTERS.fields
        and config.ACTIVE_FILTERS.fields.distance_m
        or nil
    if condition == nil or condition.active == false then
        return true
    end
    return filter_engine.match({
        kind = "pal",
        fields = {
            distance_m = { state = "known", value = distance },
        },
    }, {
        fields = { distance_m = condition },
    })
end

local function configured_display_limit()
    local configured_limit = math.max(0, math.floor(tonumber(config.MAX_DISPLAY_TARGETS) or 0))
    local maximum_limit = math.max(0, math.floor(
        tonumber(config.MAX_CONFIGURABLE_DISPLAY_TARGETS) or 0
    ))
    return math.min(configured_limit, maximum_limit)
end

local function rank_cached_nearest_candidates(camera_location)
    local camera_x, camera_y, camera_z = read_xyz(camera_location)
    if camera_x == nil then
        return nil
    end

    local ranked = {}
    for key, candidate in pairs(state.nearest_candidates) do
        if candidate.location_x ~= nil then
            local dx = candidate.location_x - camera_x
            local dy = candidate.location_y - camera_y
            local dz = candidate.location_z - camera_z
            local distance = math.sqrt(dx * dx + dy * dy + dz * dz) / 100.0
            candidate.cached_distance = distance
            if current_distance_matches(distance) then
                ranked[#ranked + 1] = {
                    key = key,
                    candidate = candidate,
                    distance = distance,
                }
            end
        end
    end
    table.sort(ranked, function(first, second)
        if first.distance ~= second.distance then
            return first.distance < second.distance
        end
        return first.key < second.key
    end)
    return ranked
end

local function desired_nearest_targets(camera_location)
    local ranked = rank_cached_nearest_candidates(camera_location)
    if ranked == nil then
        return nil, nil
    end
    local desired = {}
    local desired_order = {}
    for index = 1, math.min(#ranked, configured_display_limit()) do
        local entry = ranked[index]
        desired[entry.key] = entry
        desired_order[#desired_order + 1] = entry.key
    end
    return desired, desired_order
end

local function cached_nearest_membership_changed(camera_location)
    local desired, desired_order = desired_nearest_targets(camera_location)
    if desired == nil then
        return false
    end
    if #desired_order ~= state.bridge_target_count then
        return true
    end
    for _, key in ipairs(desired_order) do
        if state.bridge_target_keys[key] == nil then
            return true
        end
    end
    return false
end

local function cached_bridge_membership_matches(desired_order)
    if #desired_order ~= state.bridge_target_count then
        return false
    end
    for _, key in ipairs(desired_order) do
        if state.bridge_target_keys[key] == nil then
            return false
        end
    end
    return true
end

local function request_cached_bridge_resync(reason, force)
    if not runtime_enabled() or state.world_transitioning or not state.gameplay_active then
        return false
    end
    local was_dirty = state.cached_bridge_resync_dirty
    state.cached_bridge_resync_dirty = true
    state.cached_bridge_resync_force = state.cached_bridge_resync_force or force == true
    state.cached_bridge_resync_reason = reason
        or state.cached_bridge_resync_reason
        or "cached_membership"
    state.cached_bridge_resync_revision = state.cached_bridge_resync_revision + 1
    if not was_dirty then
        debug_event("CACHED_BRIDGE_RESYNC_REQUESTED", string.format(
            "reason=%s force=%s",
            state.cached_bridge_resync_reason,
            tostring(state.cached_bridge_resync_force)
        ))
    end
    return true
end

local function resolve_cached_candidate_actor(key, candidate)
    local actor = resolve_event_actor(candidate and candidate.object_path or nil)
    if actor ~= nil and actor_key(actor) == key then
        candidate.resolve_misses = 0
        local x, y, z = read_xyz(get_actor_location(actor))
        if x ~= nil then
            candidate.location_x = x
            candidate.location_y = y
            candidate.location_z = z
        end
        return actor
    end

    if candidate ~= nil then
        candidate.resolve_misses = (candidate.resolve_misses or 0) + 1
    end
    return nil
end

local function atomic_sync_cached_nearest_targets(camera_location, reason, force)
    if not ensure_bridge_session() then
        return false
    end
    camera_location = camera_location or get_camera_location(find_player_controller())
    if camera_location == nil then
        return false
    end

    local max_misses = math.max(1, math.floor(
        tonumber(config.MAX_NEAREST_TARGET_RESOLVE_MISSES) or 1
    ))
    local started_at = os.clock()
    for _ = 1, 2 do
        local desired, desired_order = desired_nearest_targets(camera_location)
        if desired == nil then
            return false
        end
        if not force and cached_bridge_membership_matches(desired_order) then
            return true
        end

        local submissions = {}
        local removed_stale_candidate = false
        local transient_unresolved = false
        for _, key in ipairs(desired_order) do
            local entry = desired[key]
            local candidate = entry and entry.candidate or nil
            local actor = resolve_cached_candidate_actor(key, candidate)
            if actor ~= nil then
                submissions[#submissions + 1] = {
                    key = key,
                    actor = actor,
                    candidate = candidate,
                    distance = entry.distance,
                }
            elseif candidate ~= nil and candidate.resolve_misses >= max_misses then
                forget_nearest_candidate(key)
                removed_stale_candidate = true
            else
                transient_unresolved = true
            end
        end

        if removed_stale_candidate then
            force = true
        elseif transient_unresolved then
            debug_event("CACHED_BRIDGE_RESYNC_DEFERRED", string.format(
                "reason=%s unresolved=transient",
                reason or "cached_membership"
            ))
            return false
        else
            state.metrics.bridge_syncs = state.metrics.bridge_syncs + 1
            if not call_bridge(BRIDGE_METHOD_CLEAR_TARGET, state.session_index) then
                state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds
                    + (os.clock() - started_at)
                return false
            end
            state.bridge_target_count = 0
            state.bridge_target_keys = {}
            state.bridge_target_order = {}

            for _, submission in ipairs(submissions) do
                if not call_bridge(
                    BRIDGE_METHOD_SET_TARGET,
                    submission.actor,
                    state.session_index,
                    submission.candidate.level,
                    math.floor(submission.distance + 0.5)
                ) then
                    state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds
                        + (os.clock() - started_at)
                    return false
                end
                state.bridge_target_keys[submission.key] = submission.candidate.object_path
                state.bridge_target_order[#state.bridge_target_order + 1] = submission.key
                state.bridge_target_count = state.bridge_target_count + 1
                state.metrics.bridge_set_calls = state.metrics.bridge_set_calls + 1
            end

            state.metrics.displayed = state.bridge_target_count
            state.next_nearest_refresh_at = now()
                + (config.NEAREST_TARGET_REFRESH_INTERVAL_MS / 1000.0)
            local elapsed = os.clock() - started_at
            state.metrics.bridge_sync_seconds = state.metrics.bridge_sync_seconds + elapsed
            log_event("CACHED_TARGETS_ATOMIC_SYNCED", string.format(
                "session=%d active=%d eligible=%d reason=%s resolved=%d elapsed_ms=%.3f",
                state.session_index,
                state.bridge_target_count,
                state.nearest_candidate_count,
                reason or "cached_membership",
                #submissions,
                elapsed * 1000.0
            ))
            return true
        end
    end
    return false
end

local function process_cached_bridge_resync(camera_location)
    if not state.cached_bridge_resync_dirty then
        return false
    end
    local reason = state.cached_bridge_resync_reason or "cached_membership"
    local force = state.cached_bridge_resync_force
    if not atomic_sync_cached_nearest_targets(camera_location, reason, force) then
        return false
    end
    state.cached_bridge_resync_dirty = false
    state.cached_bridge_resync_force = false
    state.cached_bridge_resync_reason = nil
    return true
end

local function next_nearest_refresh_key()
    local count = #state.nearest_candidate_order
    if count == 0 then
        state.nearest_refresh_cursor = 1
        return nil
    end
    if state.nearest_refresh_cursor < 1 or state.nearest_refresh_cursor > count then
        state.nearest_refresh_cursor = 1
    end
    local key = state.nearest_candidate_order[state.nearest_refresh_cursor]
    state.nearest_refresh_cursor = state.nearest_refresh_cursor + 1
    if state.nearest_refresh_cursor > count then
        state.nearest_refresh_cursor = 1
    end
    return key
end

local function clear_unresolved_display_target(key)
    if state.bridge_target_keys[key] == nil then
        return
    end
    if call_bridge(BRIDGE_METHOD_CLEAR_TARGET, state.session_index) then
        state.bridge_target_count = 0
        state.bridge_target_keys = {}
        state.bridge_target_order = {}
        log_event("NEAREST_TARGETS_FAIL_CLOSED", string.format(
            "session=%d reason=display_path_unresolved",
            state.session_index
        ))
    end
end

local function resolve_nearest_candidate(key, direct_key, direct_actor, budget)
    local candidate = state.nearest_candidates[key]
    if candidate == nil then
        return nil
    end
    if key == direct_key and is_valid(direct_actor) then
        return direct_actor
    end
    if budget.remaining <= 0 then
        return nil
    end

    budget.remaining = budget.remaining - 1
    budget.used = budget.used + 1
    local actor = resolve_event_actor(candidate.object_path)
    if actor ~= nil and actor_key(actor) == key then
        candidate.resolve_misses = 0
        local location = get_actor_location(actor)
        local x, y, z = read_xyz(location)
        if x ~= nil then
            candidate.location_x = x
            candidate.location_y = y
            candidate.location_z = z
        end
        return actor
    end

    candidate.resolve_misses = (candidate.resolve_misses or 0) + 1
    local max_misses = math.max(1, math.floor(
        tonumber(config.MAX_NEAREST_TARGET_RESOLVE_MISSES) or 1
    ))
    if candidate.resolve_misses >= max_misses then
        local was_displayed = state.bridge_target_keys[key] ~= nil
        forget_nearest_candidate(key)
        if was_displayed then
            clear_unresolved_display_target(key)
        end
    end
    return nil
end

local function apply_nearest_membership(camera_location, direct_key, direct_actor, budget, reason)
    local desired, desired_order = desired_nearest_targets(camera_location)
    if desired == nil then
        return false
    end

    local leaving_key = nil
    for _, key in ipairs(state.bridge_target_order) do
        if state.bridge_target_keys[key] ~= nil and desired[key] == nil then
            leaving_key = key
            break
        end
    end
    if leaving_key == nil then
        for key in pairs(state.bridge_target_keys) do
            if desired[key] == nil then
                leaving_key = key
                break
            end
        end
    end

    local entering_key = nil
    for _, key in ipairs(desired_order) do
        if state.bridge_target_keys[key] == nil then
            entering_key = key
            break
        end
    end
    if leaving_key == nil and entering_key == nil then
        return false
    end

    local leaving_actor = nil
    if leaving_key ~= nil then
        leaving_actor = resolve_nearest_candidate(
            leaving_key, direct_key, direct_actor, budget
        )
        if leaving_actor == nil then
            return false
        end
    end
    local entering_actor = nil
    if entering_key ~= nil then
        entering_actor = resolve_nearest_candidate(
            entering_key, direct_key, direct_actor, budget
        )
        if entering_actor == nil then
            return false
        end
    end

    if leaving_key ~= nil then
        if not call_bridge(BRIDGE_METHOD_REMOVE_TARGET, leaving_actor, state.session_index) then
            return false
        end
        forget_bridge_target(leaving_key)
        state.bridge_target_count = math.max(0, state.bridge_target_count - 1)
    end
    if entering_key ~= nil then
        local entry = desired[entering_key]
        if not call_bridge(
            BRIDGE_METHOD_SET_TARGET,
            entering_actor,
            state.session_index,
            entry.candidate.level,
            math.floor(entry.distance + 0.5)
        ) then
            return false
        end
        remember_bridge_target(entering_actor, entering_key)
        state.bridge_target_count = state.bridge_target_count + 1
        state.metrics.bridge_set_calls = state.metrics.bridge_set_calls + 1
    end

    log_event("NEAREST_TARGETS_UPDATED", string.format(
        "session=%d active=%d eligible=%d reason=%s path_resolves=%d",
        state.session_index,
        state.bridge_target_count,
        state.nearest_candidate_count,
        reason or "runtime_tick",
        budget.used
    ))
    return true
end

-- __DEPRECATED_20260721__ [reason: single-target remove/set could desynchronize Blueprint parallel arrays]
local function deprecated_refresh_nearest_bridge_targets_incrementally(reason, direct_actor, camera_location)
    if not runtime_enabled() or not state.runtime_profile.event_admission
        or state.world_transitioning or not state.gameplay_active then
        return false
    end
    if not ensure_bridge_session() then
        return false
    end

    camera_location = camera_location or get_camera_location(find_player_controller())
    if camera_location == nil then
        return false
    end

    local path_budget = math.max(0, math.floor(
        tonumber(config.MAX_NEAREST_TARGET_PATH_RESOLVES_PER_TICK) or 0
    ))
    local budget = { remaining = path_budget, used = 0 }
    direct_actor = unwrap(direct_actor)
    local direct_key = is_valid(direct_actor) and actor_key(direct_actor) or nil
    if direct_key ~= nil and state.nearest_candidates[direct_key] ~= nil then
        local candidate = state.nearest_candidates[direct_key]
        local location = get_actor_location(direct_actor)
        local x, y, z = read_xyz(location)
        if x ~= nil then
            candidate.location_x = x
            candidate.location_y = y
            candidate.location_z = z
            candidate.resolve_misses = 0
        end
    end

    local changed = apply_nearest_membership(
        camera_location, direct_key, direct_actor, budget, reason
    )
    if reason == "runtime_tick" and budget.remaining > 0
        and state.nearest_candidate_count > configured_display_limit() then
        local refresh_key = next_nearest_refresh_key()
        if refresh_key ~= nil then
            local refresh_actor = resolve_nearest_candidate(
                refresh_key, nil, nil, budget
            )
            changed = apply_nearest_membership(
                camera_location,
                refresh_key,
                refresh_actor,
                budget,
                reason
            ) or changed
        end
    end
    return true
end

local function refresh_cached_candidate_positions()
    local distance_condition = config.ACTIVE_FILTERS.fields
        and config.ACTIVE_FILTERS.fields.distance_m
        or nil
    local needs_position_refresh = state.nearest_candidate_count > configured_display_limit()
        or (distance_condition ~= nil and distance_condition.active ~= false)
    if not needs_position_refresh then
        return 0
    end

    local budget = math.max(0, math.floor(
        tonumber(config.MAX_NEAREST_TARGET_PATH_RESOLVES_PER_TICK) or 0
    ))
    local used = 0
    local removed_stale_candidate = false
    local max_misses = math.max(1, math.floor(
        tonumber(config.MAX_NEAREST_TARGET_RESOLVE_MISSES) or 1
    ))
    for _ = 1, budget do
        local key = next_nearest_refresh_key()
        if key == nil then
            break
        end
        local candidate = state.nearest_candidates[key]
        if candidate ~= nil then
            used = used + 1
            local actor = resolve_cached_candidate_actor(key, candidate)
            if actor == nil and candidate.resolve_misses >= max_misses then
                forget_nearest_candidate(key)
                removed_stale_candidate = true
            end
        end
    end
    if removed_stale_candidate then
        request_cached_bridge_resync("stale_candidate", true)
    end
    return used
end

refresh_nearest_bridge_targets = function(reason, direct_actor, camera_location)
    if not runtime_enabled() or not state.runtime_profile.event_admission
        or state.world_transitioning or not state.gameplay_active then
        return false
    end
    if not ensure_bridge_session() then
        return false
    end

    camera_location = camera_location or get_camera_location(find_player_controller())
    if camera_location == nil then
        return false
    end

    direct_actor = unwrap(direct_actor)
    local direct_key = is_valid(direct_actor) and actor_key(direct_actor) or nil
    local direct_candidate = direct_key ~= nil and state.nearest_candidates[direct_key] or nil
    if direct_candidate ~= nil then
        local x, y, z = read_xyz(get_actor_location(direct_actor))
        if x ~= nil then
            direct_candidate.location_x = x
            direct_candidate.location_y = y
            direct_candidate.location_z = z
            direct_candidate.resolve_misses = 0
        end
    end

    local path_resolves = 0
    if reason == "runtime_tick" then
        path_resolves = refresh_cached_candidate_positions()
    end
    if cached_nearest_membership_changed(camera_location) then
        request_cached_bridge_resync(reason or "cached_membership", false)
    end
    debug_event("CACHED_NEAREST_REFRESHED", string.format(
        "reason=%s eligible=%d path_resolves=%d",
        reason or "cached_membership",
        state.nearest_candidate_count,
        path_resolves
    ))
    return true
end

local function run_stream_integrity_scan(camera_location)
    local started_at = os.clock()
    local descriptor = state.entity_registry:get("pal")
    local ok, objects = pcall(function()
        return descriptor and descriptor.find_all(entity_runtime) or nil
    end)
    if not ok or type(objects) ~= "table" then
        state.next_stream_integrity_scan_at = now()
            + (math.max(0, tonumber(config.STREAM_INTEGRITY_SCAN_MIN_INTERVAL_MS) or 0) / 1000.0)
        log_event("STREAM_INTEGRITY_SCAN_FAILED", "reason=discovery_unavailable")
        return false
    end

    local raw_count = 0
    local existing_count = 0
    local discovered_count = 0
    local queued_count = 0
    for _, wrapped_actor in ipairs(objects) do
        raw_count = raw_count + 1
        local actor = unwrap(wrapped_actor)
        if is_valid(actor) and is_a(actor, resolve_class("monster")) then
            local key, object_path = actor_lookup_identity(actor)
            local candidate = key ~= nil and state.nearest_candidates[key] or nil
            if candidate ~= nil then
                existing_count = existing_count + 1
                candidate.object_path = object_path or candidate.object_path
                local x, y, z = read_xyz(get_actor_location(actor))
                if x ~= nil then
                    candidate.location_x = x
                    candidate.location_y = y
                    candidate.location_z = z
                    candidate.resolve_misses = 0
                end
            elseif key ~= nil and not state.event_queue_keys[key] then
                discovered_count = discovered_count + 1
                if enqueue_event_retry(actor, "stream_integrity_discovered", "admit") then
                    queued_count = queued_count + 1
                end
            end
        end
    end

    reset_stream_integrity_tracking(camera_location)
    refresh_nearest_bridge_targets("stream_integrity_scan", nil, camera_location)
    log_event("STREAM_INTEGRITY_SCAN", string.format(
        "raw=%d existing=%d discovered=%d queued=%d elapsed_ms=%.3f",
        raw_count,
        existing_count,
        discovered_count,
        queued_count,
        (os.clock() - started_at) * 1000.0
    ))
    return true
end

admit_begin_play_target = function(actor, source)
    actor = unwrap(actor)
    if not is_valid(actor) then
        return false, "actor_invalid"
    end
    if not is_a(actor, resolve_class("monster")) then
        return false, "not_monster"
    end
    if not runtime_enabled() or not state.runtime_profile.event_admission then
        return false, "runtime_inactive"
    end
    if state.world_transitioning then
        return false, "world_transitioning"
    end
    if not state.gameplay_active then
        return false, "gameplay_inactive"
    end

    local key = actor_key(actor)
    if key == nil or state.nearest_candidates[key] then
        return false, "duplicate"
    end
    if not ensure_bridge_session() then
        return false, "bridge_unavailable"
    end

    source = source or "begin_play"
    local started_at = os.clock()
    local generation = entity_snapshot.begin_generation(state.entity_store, source, now())
    local added, rejection_reason = add_entity_candidate(actor, source, generation, {
        source = source,
        camera_location = get_camera_location(find_player_controller()),
    })
    if not added then
        return false, rejection_reason
    end
    local matched = added and filter_engine.filter(
        generation.records,
        filters_without_current_distance()
    ) or {}
    local record = matched[1]
    if record == nil then
        return false, "filter_rejected"
    end
    local remembered_key, remember_error = remember_nearest_candidate(actor, record)
    if remembered_key == nil then
        return false, remember_error
    end
    refresh_nearest_bridge_targets(source, actor)
    state.metrics.event_queue_admitted = state.metrics.event_queue_admitted + 1
    local elapsed = os.clock() - started_at
    state.metrics.event_callback_max_seconds = math.max(state.metrics.event_callback_max_seconds, elapsed)
    log_event("EVENT_CANDIDATE_REGISTERED", string.format(
        "session=%d active=%d eligible=%d displayed=%s elapsed_ms=%.3f source=%s",
        state.session_index,
        state.bridge_target_count,
        state.nearest_candidate_count,
        tostring(state.bridge_target_keys[key] ~= nil),
        elapsed * 1000.0,
        source
    ))
    if state.bridge_target_keys[key] ~= nil then
        log_event("EVENT_TARGET_ADDED", string.format(
            "session=%d active=%d elapsed_ms=%.3f source=%s",
            state.session_index,
            state.bridge_target_count,
            elapsed * 1000.0,
            source
        ))
    end
    return true, nil
end

-- __DEPRECATED_20260721__ [reason: displayed metadata refresh used RemoveTarget followed by SetTarget]
local function deprecated_refresh_bridge_target_incrementally(actor, source)
    actor = unwrap(actor)
    if not is_valid(actor) then
        return false, "actor_invalid"
    end
    if not runtime_enabled() or not state.runtime_profile.event_admission then
        return false, "runtime_inactive"
    end
    if not ensure_bridge_session() then
        return false, "bridge_unavailable"
    end

    local key = actor_key(actor)
    if key == nil or not state.nearest_candidates[key] then
        return false, "target_not_eligible"
    end
    local was_displayed = state.bridge_target_keys[key] ~= nil

    source = source or "initialization_complete"
    local started_at = os.clock()
    local generation = entity_snapshot.begin_generation(state.entity_store, source, now())
    local added, rejection_reason = add_entity_candidate(actor, source, generation, {
        source = source,
        camera_location = get_camera_location(find_player_controller()),
    })
    if not added then
        if event_rejection_is_retryable(rejection_reason) then
            return false, rejection_reason
        end
        if not remove_bridge_target(actor, "initialization_reclassified:" .. tostring(rejection_reason)) then
            return false, "bridge_unavailable"
        end
        log_event("EVENT_TARGET_REFRESH_DROPPED", string.format(
            "session=%d reason=%s",
            state.session_index,
            rejection_reason or "rejected"
        ))
        return true, nil
    end

    local matched = filter_engine.filter(generation.records, filters_without_current_distance())
    local record = matched[1]
    if record == nil then
        if not remove_bridge_target(actor, "initialization_filter_rejected") then
            return false, "bridge_unavailable"
        end
        log_event("EVENT_TARGET_REFRESH_DROPPED", string.format(
            "session=%d reason=filter_rejected",
            state.session_index
        ))
        return true, nil
    end

    local remembered_key, remember_error = remember_nearest_candidate(actor, record)
    if remembered_key == nil then
        return false, remember_error
    end
    refresh_nearest_bridge_targets(source, actor)

    if was_displayed and state.bridge_target_keys[key] ~= nil then
        local level = display_snapshot_integer(record, "level")
        local current_distance = distance_m(actor, {
            camera_location = get_camera_location(find_player_controller()),
        })
        if not call_bridge(BRIDGE_METHOD_REMOVE_TARGET, actor, state.session_index) then
            return false, "bridge_unavailable"
        end
        if not call_bridge(
            BRIDGE_METHOD_SET_TARGET,
            actor,
            state.session_index,
            level,
            current_distance and math.floor(current_distance + 0.5) or -1
        ) then
            return false, "bridge_unavailable"
        end
        state.metrics.bridge_set_calls = state.metrics.bridge_set_calls + 1
    end

    local elapsed = os.clock() - started_at
    state.metrics.event_callback_max_seconds = math.max(state.metrics.event_callback_max_seconds, elapsed)
    log_event("EVENT_TARGET_REFRESHED", string.format(
        "session=%d active=%d displayed=%s elapsed_ms=%.3f source=%s",
        state.session_index,
        state.bridge_target_count,
        tostring(state.bridge_target_keys[key] ~= nil),
        elapsed * 1000.0,
        source
    ))
    return true, nil
end

refresh_bridge_target = function(actor, source)
    actor = unwrap(actor)
    if not is_valid(actor) then
        return false, "actor_invalid"
    end
    if not runtime_enabled() or not state.runtime_profile.event_admission then
        return false, "runtime_inactive"
    end
    if not ensure_bridge_session() then
        return false, "bridge_unavailable"
    end

    local key = actor_key(actor)
    if key == nil or not state.nearest_candidates[key] then
        return false, "target_not_eligible"
    end
    local was_displayed = state.bridge_target_keys[key] ~= nil
    source = source or "initialization_complete"
    local started_at = os.clock()
    local generation = entity_snapshot.begin_generation(state.entity_store, source, now())
    local added, rejection_reason = add_entity_candidate(actor, source, generation, {
        source = source,
        camera_location = get_camera_location(find_player_controller()),
    })
    if not added then
        if event_rejection_is_retryable(rejection_reason) then
            return false, rejection_reason
        end
        remove_bridge_target(actor, "initialization_reclassified:" .. tostring(rejection_reason))
        return true, nil
    end

    local matched = filter_engine.filter(generation.records, filters_without_current_distance())
    local record = matched[1]
    if record == nil then
        remove_bridge_target(actor, "initialization_filter_rejected")
        return true, nil
    end
    local remembered_key, remember_error = remember_nearest_candidate(actor, record)
    if remembered_key == nil then
        return false, remember_error
    end

    refresh_nearest_bridge_targets(source, actor)
    request_cached_bridge_resync(source, was_displayed)
    local elapsed = os.clock() - started_at
    state.metrics.event_callback_max_seconds = math.max(
        state.metrics.event_callback_max_seconds,
        elapsed
    )
    log_event("EVENT_TARGET_REFRESH_QUEUED", string.format(
        "session=%d displayed=%s elapsed_ms=%.3f source=%s",
        state.session_index,
        tostring(was_displayed),
        elapsed * 1000.0,
        source
    ))
    return true, nil
end

-- __DEPRECATED_20260721__ [reason: single-target removal could shift Blueprint parallel arrays]
local function deprecated_remove_bridge_target_incrementally(actor, reason)
    actor = unwrap(actor)
    local key = actor_key(actor)
    local pending_cancelled = key ~= nil and state.event_queue_keys[key] == true
    if pending_cancelled then
        state.event_queue_keys[key] = nil
        debug_event("EVENT_TARGET_RETRY_CANCELLED", string.format(
            "reason=%s",
            reason or "lifecycle"
        ))
    end
    if not is_valid(actor) then
        return pending_cancelled
    end
    if key == nil then
        return pending_cancelled
    end
    local was_displayed = state.bridge_target_keys[key] ~= nil
    if was_displayed then
        if not call_bridge(BRIDGE_METHOD_REMOVE_TARGET, actor, state.session_index) then
            return false
        end
        forget_bridge_target(key)
        state.bridge_target_count = math.max(0, state.bridge_target_count - 1)
    end
    local candidate_removed = forget_nearest_candidate(key)
    if candidate_removed then
        refresh_nearest_bridge_targets(reason or "lifecycle")
    end
    log_event("EVENT_CANDIDATE_REMOVED", string.format(
        "session=%d active=%d eligible=%d displayed=%s reason=%s",
        state.session_index,
        state.bridge_target_count,
        state.nearest_candidate_count,
        tostring(was_displayed),
        reason or "lifecycle"
    ))
    if was_displayed then
        log_event("EVENT_TARGET_REMOVED", string.format(
            "session=%d active=%d reason=%s",
            state.session_index,
            state.bridge_target_count,
            reason or "lifecycle"
        ))
    end
    return pending_cancelled or candidate_removed or was_displayed
end

remove_bridge_target = function(actor, reason)
    actor = unwrap(actor)
    local key = is_valid(actor) and actor_key(actor) or nil
    local pending_cancelled = key ~= nil and state.event_queue_keys[key] == true
    if pending_cancelled then
        state.event_queue_keys[key] = nil
        debug_event("EVENT_TARGET_RETRY_CANCELLED", string.format(
            "reason=%s",
            reason or "lifecycle"
        ))
    end
    if key == nil then
        return pending_cancelled
    end

    local was_displayed = state.bridge_target_keys[key] ~= nil
    local candidate_removed = forget_nearest_candidate(key)
    if candidate_removed or was_displayed then
        request_cached_bridge_resync(reason or "lifecycle", true)
    end
    log_event("EVENT_CANDIDATE_REMOVED", string.format(
        "session=%d active=%d eligible=%d displayed=%s reason=%s mode=atomic_cached",
        state.session_index,
        state.bridge_target_count,
        state.nearest_candidate_count,
        tostring(was_displayed),
        reason or "lifecycle"
    ))
    return pending_cancelled or candidate_removed or was_displayed
end

local function handle_character_initialization_complete(actor, source)
    actor = unwrap(actor)
    if not is_valid(actor) or not is_a(actor, resolve_class("monster")) then
        return
    end

    local key = actor_key(actor)
    local mode = key ~= nil and state.nearest_candidates[key] ~= nil and "refresh" or "admit"
    local ok, handled, rejection_reason = pcall(function()
        if mode == "refresh" then
            return refresh_bridge_target(actor, source)
        end
        return admit_begin_play_target(actor, source)
    end)
    if not ok then
        state.metrics.event_queue_rejected = state.metrics.event_queue_rejected + 1
        log_event("ERROR_INITIALIZATION_ADMISSION", tostring(handled))
        return
    end
    if handled then
        if key ~= nil and state.event_queue_keys[key] then
            state.event_queue_keys[key] = nil
            debug_event("EVENT_TARGET_RETRY_CANCELLED", "reason=initialization_complete")
        end
        return
    end

    local deferred = enqueue_event_retry(actor, rejection_reason, mode)
    local already_pending = key ~= nil and state.event_queue_keys[key] ~= nil
    if not deferred and not already_pending then
        debug_event("EVENT_TARGET_INITIALIZATION_DROPPED", string.format(
            "reason=%s mode=%s",
            rejection_reason or "rejected",
            mode
        ))
    end
end

local function emit_capture_mode_marker(reason)
    log_event("PERF_MODE_CHANGED", string.format(
        "capture_session=%d profile=%s preset=%s reason=%s",
        state.capture_session_id,
        state.runtime_profile.name,
        state.runtime_profile.preset_name,
        reason
    ))
end

local function set_capture_active(requested, reason)
    requested = requested == true
    if requested == state.capture_active then
        return
    end

    state.capture_requested = requested
    state.capture_active = requested
    if requested then
        state.capture_session_id = state.capture_session_id + 1
        log_event("PERF_SESSION_START", string.format(
            "capture_session=%d reason=%s",
            state.capture_session_id,
            reason
        ))
        emit_capture_mode_marker("capture_start")
        return
    end

    log_event("PERF_SESSION_STOP", string.format(
        "capture_session=%d profile=%s reason=%s",
        state.capture_session_id,
        state.runtime_profile.name,
        reason
    ))
end

local function apply_runtime_profile(master_enabled, profile_id, preset_id, reason)
    master_enabled = master_enabled == true
    profile_id = runtime_profiles.normalize_profile_id(profile_id)
    preset_id = runtime_profiles.normalize_preset_id(preset_id)
    local effective_profile_id = master_enabled and profile_id or runtime_profiles.PROFILE.OFF
    local next_profile = runtime_profiles.resolve(effective_profile_id, preset_id)
    local changed = state.runtime_master_enabled ~= master_enabled
        or state.selected_profile_id ~= profile_id
        or state.selected_preset_id ~= preset_id
        or state.runtime_profile.id ~= next_profile.id
        or state.runtime_profile.max_display_targets ~= next_profile.max_display_targets
        or state.runtime_profile.reconcile_interval_ms ~= next_profile.reconcile_interval_ms
    if not changed then
        return false
    end

    state.runtime_master_enabled = master_enabled
    state.selected_profile_id = profile_id
    state.selected_preset_id = preset_id
    state.profile_generation = state.profile_generation + 1
    state.runtime_profile = next_profile
    state.next_reconcile_at = 0.0
    config.MAX_DISPLAY_TARGETS = math.min(
        next_profile.max_display_targets,
        config.MAX_CONFIGURABLE_DISPLAY_TARGETS
    )

    clear_candidate_cache("profile_change:" .. reason)
    sync_bridge_target()
    log_event("RUNTIME_PROFILE", string.format(
        "generation=%d master_enabled=%s profile=%s preset=%s interval_ms=%d max_display=%d reason=%s",
        state.profile_generation,
        tostring(master_enabled),
        next_profile.name,
        next_profile.preset_name,
        next_profile.reconcile_interval_ms,
        config.MAX_DISPLAY_TARGETS,
        reason
    ))
    if state.capture_active then
        emit_capture_mode_marker("profile_change")
    end

    if runtime_enabled() and state.gameplay_active and not state.world_transitioning then
        start_safe_reconcile("profile_enter")
        if next_profile.reconcile_interval_ms > 0 then
            state.next_reconcile_at = now() + (next_profile.reconcile_interval_ms / 1000.0)
        end
    end
    return true
end

local function read_panel_number(property_name)
    local ok, value = safe_get_property(state.bridge_actor, property_name)
    if not ok then
        return nil
    end
    return read_number(value)
end

local function read_panel_boolean(property_name)
    local ok, value = safe_get_property(state.bridge_actor, property_name)
    if not ok or type(value) ~= "boolean" then
        return nil
    end
    return value
end

local function read_panel_string(property_name)
    local ok, value = safe_get_property(state.bridge_actor, property_name)
    value = unwrap(value)
    if not ok then
        return nil
    end
    if type(value) == "string" then
        return value
    end
    if type(value) ~= "userdata" and type(value) ~= "table" then
        return nil
    end

    local type_ok, reflected_type = pcall(function()
        return value:type()
    end)
    if not type_ok or reflected_type ~= "FString" then
        return nil
    end
    local string_ok, result = pcall(function()
        return value:ToString()
    end)
    if not string_ok or type(result) ~= "string" then
        return nil
    end
    return result
end

local function drain_rich_text_audit()
    if not is_valid(state.bridge_actor) then
        return
    end
    local buffer = read_panel_string("ESP_RichTextAuditBuffer")
    if buffer == nil then
        return
    end

    local consumed = state.rich_text_audit_consumed_bytes or 0
    if #buffer < consumed then
        consumed = 0
    end
    if #buffer <= consumed then
        return
    end

    local delta = buffer:sub(consumed + 1)
    state.rich_text_audit_consumed_bytes = #buffer
    for line in delta:gmatch("[^\r\n]+") do
        if line ~= "" then
            log_event("RICH_TEXT_AUDIT", line)
        end
    end
end

local function normalize_panel_filter_bound(value, minimum, maximum)
    if type(value) ~= "number" or value ~= value or value == math.huge or value == -math.huge then
        return nil
    end
    value = math.floor(value)
    if value <= 0 then
        return nil
    end
    return math.max(minimum, math.min(maximum, value))
end

local function normalize_panel_distance_max(value)
    if type(value) ~= "number" or value ~= value or value == math.huge or value == -math.huge then
        return nil
    end
    return math.max(0, math.min(config.MAX_DISTANCE_M, math.floor(value)))
end

-- __DEPRECATED_20260717__ [reason: the panel no longer exposes or applies a minimum-distance bound]
local function apply_panel_filters_deprecated_20260717(level_min_raw, level_max_raw, distance_min_raw, distance_max_raw)
    local level_min = normalize_panel_filter_bound(level_min_raw, 1, 100)
    local level_max = normalize_panel_filter_bound(level_max_raw, 1, 100)
    local distance_min = normalize_panel_filter_bound(
        distance_min_raw,
        config.MIN_DISTANCE_M,
        config.MAX_DISTANCE_M
    )
    local distance_max = normalize_panel_distance_max(distance_max_raw)
    local key = string.format(
        "level:%s:%s;distance:%s:%s",
        tostring(level_min),
        tostring(level_max),
        tostring(distance_min),
        tostring(distance_max)
    )
    if key == state.panel_filter_key then
        return false
    end

    local fields = {}
    if level_min ~= nil or level_max ~= nil then
        fields.level = { mode = "range", min = level_min, max = level_max }
    end
    if distance_min ~= nil or distance_max ~= nil then
        fields.distance_m = { mode = "range", min = distance_min, max = distance_max }
    end
    config.ACTIVE_FILTERS = { fields = fields }
    state.panel_filter_key = key
    log_event("FILTER_CONFIG", string.format(
        "level_min=%s level_max=%s distance_min=%s distance_max=%s",
        tostring(level_min),
        tostring(level_max),
        tostring(distance_min),
        tostring(distance_max)
    ))
    return true
end

local function apply_panel_filters(level_min_raw, level_max_raw, distance_max_raw)
    local level_min = normalize_panel_filter_bound(level_min_raw, 1, 100)
    local level_max = normalize_panel_filter_bound(level_max_raw, 1, 100)
    local distance_max = normalize_panel_distance_max(distance_max_raw)
    local key = string.format(
        "level:%s:%s;distance_max:%s",
        tostring(level_min),
        tostring(level_max),
        tostring(distance_max)
    )
    if key == state.panel_filter_key then
        return false
    end

    local fields = {}
    if level_min ~= nil or level_max ~= nil then
        fields.level = { mode = "range", min = level_min, max = level_max }
    end
    if distance_max ~= nil then
        fields.distance_m = { mode = "range", min = 0, max = distance_max }
    end
    config.ACTIVE_FILTERS = { fields = fields }
    state.panel_filter_key = key
    log_event("FILTER_CONFIG", string.format(
        "level_min=%s level_max=%s distance_min=0 distance_max=%s",
        tostring(level_min),
        tostring(level_max),
        tostring(distance_max)
    ))
    return true
end

local function normalize_gender_filter_id(raw_id)
    if type(raw_id) ~= "number" or raw_id ~= raw_id or raw_id == math.huge or raw_id == -math.huge then
        return 0
    end
    return math.max(0, math.min(2, math.floor(raw_id)))
end

local function normalize_lucky_filter_id(raw_id)
    if type(raw_id) ~= "number" or raw_id ~= raw_id or raw_id == math.huge or raw_id == -math.huge then
        return 0
    end
    return math.max(0, math.min(2, math.floor(raw_id)))
end

local function normalize_boss_filter_id(raw_id)
    if type(raw_id) ~= "number" or raw_id ~= raw_id or raw_id == math.huge or raw_id == -math.huge then
        return 0
    end
    return math.max(0, math.min(2, math.floor(raw_id)))
end

local function normalize_collection_filter_id(raw_id)
    if type(raw_id) ~= "number" or raw_id ~= raw_id or raw_id == math.huge or raw_id == -math.huge then
        return 0
    end
    return math.max(0, math.min(2, math.floor(raw_id)))
end

local function build_element_filter_mask(
    normal, fire, water, leaf, electricity, ice, earth, dark, dragon
)
    return (normal and 1 or 0)
        + (fire and 2 or 0)
        + (water and 4 or 0)
        + (leaf and 8 or 0)
        + (electricity and 16 or 0)
        + (ice and 32 or 0)
        + (earth and 64 or 0)
        + (dark and 128 or 0)
        + (dragon and 256 or 0)
end

local function apply_display_styles(
    show_top_guide_line,
    show_name,
    show_level,
    show_distance,
    show_iv,
    show_passive_skills,
    raw_iv_min,
    raw_iv_hp_min,
    raw_iv_attack_min,
    raw_iv_defense_min,
    raw_passive_filter_revision,
    raw_gender_filter_id,
    raw_lucky_filter_id,
    raw_boss_filter_id,
    raw_collection_filter_id,
    raw_species_filter_text,
    raw_element_filter_mask,
    raw_language_id
)
    local gender_filter_id = normalize_gender_filter_id(raw_gender_filter_id)
    local lucky_filter_id = normalize_lucky_filter_id(raw_lucky_filter_id)
    local boss_filter_id = normalize_boss_filter_id(raw_boss_filter_id)
    local collection_filter_id = normalize_collection_filter_id(raw_collection_filter_id)
    local species_filter_text = user_settings.normalize({
        species_filters = raw_species_filter_text,
    }).species_filters
    local iv_min = math.max(0, math.min(100, math.floor(tonumber(raw_iv_min) or 0)))
    local iv_hp_min = math.max(0, math.min(100, math.floor(tonumber(raw_iv_hp_min) or iv_min)))
    local iv_attack_min = math.max(0, math.min(100, math.floor(tonumber(raw_iv_attack_min) or iv_min)))
    local iv_defense_min = math.max(0, math.min(100, math.floor(tonumber(raw_iv_defense_min) or iv_min)))
    local passive_filter_revision = math.max(0, math.floor(tonumber(raw_passive_filter_revision) or 0))
    local element_filter_mask = math.max(0, math.min(511, math.floor(tonumber(raw_element_filter_mask) or 0)))
    local language_id = math.max(0, math.min(1, math.floor(tonumber(raw_language_id) or 0)))
    if show_top_guide_line == state.show_top_guide_line
        and show_name == state.show_name
        and show_level == state.show_level
        and show_distance == state.show_distance
        and show_iv == state.show_iv
        and show_passive_skills == state.show_passive_skills
        and iv_min == state.iv_min
        and iv_hp_min == state.iv_hp_min
        and iv_attack_min == state.iv_attack_min
        and iv_defense_min == state.iv_defense_min
        and passive_filter_revision == state.passive_filter_revision
        and gender_filter_id == state.gender_filter_id
        and lucky_filter_id == state.lucky_filter_id
        and boss_filter_id == state.boss_filter_id
        and collection_filter_id == state.collection_filter_id
        and species_filter_text == state.species_filter_text
        and element_filter_mask == state.element_filter_mask
        and language_id == state.language_id then
        return false
    end
    state.show_top_guide_line = show_top_guide_line
    state.show_name = show_name
    state.show_level = show_level
    state.show_distance = show_distance
    state.show_iv = show_iv
    state.show_passive_skills = show_passive_skills
    state.iv_min = iv_min
    state.iv_hp_min = iv_hp_min
    state.iv_attack_min = iv_attack_min
    state.iv_defense_min = iv_defense_min
    state.passive_filter_revision = passive_filter_revision
    state.gender_filter_id = gender_filter_id
    state.lucky_filter_id = lucky_filter_id
    state.boss_filter_id = boss_filter_id
    state.collection_filter_id = collection_filter_id
    state.species_filter_text = species_filter_text
    state.element_filter_mask = element_filter_mask
    state.language_id = language_id
    if not safe_set_property(state.bridge_actor, "ESP_ElementFilterMask", element_filter_mask) then
        log_event("ELEMENT_FILTER_BRIDGE_FAILED", string.format("mask=%d", element_filter_mask))
    end
    call_bridge(
        BRIDGE_METHOD_SET_DISPLAY_STYLE,
        show_top_guide_line,
        show_name,
        show_level,
        show_distance,
        show_iv,
        show_passive_skills,
        iv_min,
        iv_hp_min,
        iv_attack_min,
        iv_defense_min,
        passive_filter_revision,
        gender_filter_id,
        lucky_filter_id,
        boss_filter_id,
        collection_filter_id,
        species_filter_text,
        element_filter_mask,
        language_id
    )
    local gender_filter_names = { [0] = "all", [1] = "male", [2] = "female" }
    local lucky_filter_names = { [0] = "all", [1] = "only_lucky", [2] = "exclude_lucky" }
    local boss_filter_names = { [0] = "all", [1] = "only_boss", [2] = "exclude_boss" }
    local collection_filter_names = { [0] = "all", [1] = "incomplete", [2] = "complete" }
    log_event("DISPLAY_STYLE", string.format(
        "top_guide_line=%s show_name=%s show_level=%s show_distance=%s show_iv=%s show_passives=%s iv_hp_min=%d iv_attack_min=%d iv_defense_min=%d passive_filter_revision=%d gender_filter=%s lucky_filter=%s boss_filter=%s collection_filter=%s species_filter_count=%d element_filter_mask=%d",
        tostring(show_top_guide_line),
        tostring(show_name),
        tostring(show_level),
        tostring(show_distance),
        tostring(show_iv),
        tostring(show_passive_skills),
        iv_hp_min,
        iv_attack_min,
        iv_defense_min,
        passive_filter_revision,
        gender_filter_names[gender_filter_id],
        lucky_filter_names[lucky_filter_id],
        boss_filter_names[boss_filter_id],
        collection_filter_names[collection_filter_id],
        select(2, species_filter_text:gsub("|", "")) > 0
            and math.max(0, select(2, species_filter_text:gsub("|", "")) - 1)
            or 0,
        element_filter_mask
    ))
    return true
end

local function apply_display_target_limit(raw_limit)
    if type(raw_limit) ~= "number" or raw_limit ~= raw_limit
        or raw_limit == math.huge or raw_limit == -math.huge then
        return false
    end
    local limit = math.max(1, math.min(
        config.MAX_CONFIGURABLE_DISPLAY_TARGETS,
        math.floor(raw_limit)
    ))
    if config.MAX_DISPLAY_TARGETS == limit then
        return false
    end
    config.MAX_DISPLAY_TARGETS = limit
    log_event("DISPLAY_TARGET_LIMIT", string.format("value=%d", limit))
    return true
end

local function poll_panel_controls()
    if not is_valid(state.bridge_actor) then
        return
    end

    local revision = read_panel_number("ESP_ControlRevision")
    if revision == nil then
        if not state.control_pending_logged then
            state.control_pending_logged = true
            debug_event("PANEL_CONTROL_PENDING", "reason=revision_unavailable")
        end
        return
    end
    revision = math.floor(revision)
    if revision == state.control_revision then
        return
    end

    local master_enabled = read_panel_boolean("ESP_RuntimeEnabled")
    local profile_id = read_panel_number("ESP_ProfileId")
    local preset_id = read_panel_number("ESP_PresetId")
    local capture_requested = read_panel_boolean("ESP_CaptureRequested")
    local level_min = read_panel_number("ESP_LevelMin")
    local level_max = read_panel_number("ESP_LevelMax")
    -- __DEPRECATED_20260717__ [reason: minimum distance is intentionally absent from the V2 panel and runtime]
    -- local distance_min = read_panel_number("ESP_DistanceMin")
    local distance_max = read_panel_number("ESP_DistanceMax")
    local show_top_guide_line = read_panel_boolean("ESP_ShowTopGuideLine")
    local show_name = read_panel_boolean("ESP_ShowName")
    local show_level = read_panel_boolean("ESP_ShowLevel")
    local show_distance = read_panel_boolean("ESP_ShowDistance")
    local show_iv = read_panel_boolean("ESP_ShowIV")
    local show_passive_skills = read_panel_boolean("ESP_ShowPassiveSkills")
    local iv_min = read_panel_number("ESP_IvMin")
    local iv_hp_min = read_panel_number("ESP_IvHpMin")
    local iv_attack_min = read_panel_number("ESP_IvAttackMin")
    local iv_defense_min = read_panel_number("ESP_IvDefenseMin")
    local passive_includes = read_panel_string("ESP_PassiveIncludeText")
    local passive_excludes = read_panel_string("ESP_PassiveExcludeText")
    local expand_rainbow = read_panel_boolean("ESP_PassiveRainbowExpanded")
    local expand_legend = read_panel_boolean("ESP_PassiveLegendExpanded")
    local expand_gold3 = read_panel_boolean("ESP_PassiveGold3Expanded")
    local expand_gold2 = read_panel_boolean("ESP_PassiveGold2Expanded")
    local expand_normal = read_panel_boolean("ESP_PassiveNormalExpanded")
    local expand_negative1 = read_panel_boolean("ESP_PassiveNegative1Expanded")
    local expand_negative2 = read_panel_boolean("ESP_PassiveNegative2Expanded")
    local expand_negative3 = read_panel_boolean("ESP_PassiveNegative3Expanded")
    local passive_filter_revision = read_panel_number("ESP_PassiveFilterRevision")
    local gender_filter_id = read_panel_number("ESP_GenderFilterId")
    local lucky_filter_id = read_panel_number("ESP_LuckyFilterId")
    local boss_filter_id = read_panel_number("ESP_BossFilterId")
    local collection_filter_id = read_panel_number("ESP_CollectionFilterId")
    local species_filter_text = read_panel_string("ESP_SpeciesFilterText")
    local element_normal = read_panel_boolean("ESP_ElementNormal")
    local element_fire = read_panel_boolean("ESP_ElementFire")
    local element_water = read_panel_boolean("ESP_ElementWater")
    local element_leaf = read_panel_boolean("ESP_ElementLeaf")
    local element_electricity = read_panel_boolean("ESP_ElementElectricity")
    local element_ice = read_panel_boolean("ESP_ElementIce")
    local element_earth = read_panel_boolean("ESP_ElementEarth")
    local element_dark = read_panel_boolean("ESP_ElementDark")
    local element_dragon = read_panel_boolean("ESP_ElementDragon")
    local language_id = read_panel_number("ESP_LanguageId")
    local display_target_limit = read_panel_number("ESP_DisplayTargetLimit")
    local panel_main_page = read_panel_number("ESP_PanelMainPage")
    local panel_filter_page = read_panel_number("ESP_PanelFilterPage")
    if master_enabled == nil or profile_id == nil or preset_id == nil or capture_requested == nil
        -- __DEPRECATED_20260717__ [reason: minimum distance is no longer required]
        -- or level_min == nil or level_max == nil or distance_min == nil or distance_max == nil
        or level_min == nil or level_max == nil or distance_max == nil
        or show_top_guide_line == nil or show_name == nil or show_level == nil or show_distance == nil
        or show_iv == nil or show_passive_skills == nil or iv_min == nil
        or iv_hp_min == nil or iv_attack_min == nil or iv_defense_min == nil
        or passive_includes == nil or passive_excludes == nil
        or expand_rainbow == nil or expand_legend == nil or expand_gold3 == nil or expand_gold2 == nil
        or expand_normal == nil or expand_negative1 == nil or expand_negative2 == nil or expand_negative3 == nil
        or passive_filter_revision == nil
        or gender_filter_id == nil or lucky_filter_id == nil or boss_filter_id == nil
        or collection_filter_id == nil or species_filter_text == nil
        or element_normal == nil or element_fire == nil or element_water == nil or element_leaf == nil
        or element_electricity == nil or element_ice == nil or element_earth == nil
        or element_dark == nil or element_dragon == nil
        or language_id == nil or display_target_limit == nil
        or panel_main_page == nil or panel_filter_page == nil then
        if not state.control_pending_logged then
            state.control_pending_logged = true
            debug_event("PANEL_CONTROL_PENDING", "reason=property_unavailable")
        end
        return
    end

    state.control_pending_logged = false
    panel_main_page = math.max(0, math.min(1, math.floor(panel_main_page)))
    panel_filter_page = math.max(0, math.min(1, math.floor(panel_filter_page)))
    apply_runtime_profile(master_enabled, profile_id, preset_id, "panel_revision")
    -- __DEPRECATED_20260717__ [reason: minimum distance is no longer applied]
    -- local filters_changed = apply_panel_filters(level_min, level_max, distance_min, distance_max)
    local filters_changed = apply_panel_filters(level_min, level_max, distance_max)
    local element_filter_mask = build_element_filter_mask(
        element_normal, element_fire, element_water, element_leaf, element_electricity,
        element_ice, element_earth, element_dark, element_dragon
    )
    apply_display_styles(
        show_top_guide_line,
        show_name,
        show_level,
        show_distance,
        show_iv,
        show_passive_skills,
        iv_min,
        iv_hp_min,
        iv_attack_min,
        iv_defense_min,
        passive_filter_revision,
        gender_filter_id,
        lucky_filter_id,
        boss_filter_id,
        collection_filter_id,
        species_filter_text,
        element_filter_mask,
        language_id
    )
    local limit_changed = apply_display_target_limit(display_target_limit)
    if runtime_enabled() and state.gameplay_active and (filters_changed or limit_changed) then
        clear_candidate_cache("panel_query_change")
        start_safe_reconcile("panel_query_change")
    end
    set_capture_active(capture_requested, "panel_revision")
    schedule_user_settings_save({
        runtime_enabled = master_enabled,
        profile_id = profile_id,
        preset_id = preset_id,
        language_id = state.language_id,
        level_min = level_min,
        level_max = level_max,
        distance_max = distance_max,
        display_limit = display_target_limit,
        show_top = show_top_guide_line,
        show_name = show_name,
        show_level = show_level,
        show_distance = show_distance,
        show_iv = show_iv,
        show_passives = show_passive_skills,
        iv_min = 0,
        iv_hp_min = iv_hp_min,
        iv_attack_min = iv_attack_min,
        iv_defense_min = iv_defense_min,
        passive_includes = passive_includes,
        passive_excludes = passive_excludes,
        expand_rainbow = expand_rainbow,
        expand_legend = expand_legend,
        expand_gold3 = expand_gold3,
        expand_gold2 = expand_gold2,
        expand_normal = expand_normal,
        expand_negative1 = expand_negative1,
        expand_negative2 = expand_negative2,
        expand_negative3 = expand_negative3,
        gender = gender_filter_id,
        lucky = lucky_filter_id,
        boss = boss_filter_id,
        collection = collection_filter_id,
        species_filters = species_filter_text,
        panel_main_page = panel_main_page,
        panel_filter_page = panel_filter_page,
        element_normal = element_normal,
        element_fire = element_fire,
        element_water = element_water,
        element_leaf = element_leaf,
        element_electricity = element_electricity,
        element_ice = element_ice,
        element_earth = element_earth,
        element_dark = element_dark,
        element_dragon = element_dragon,
    })
    state.control_revision = revision
    debug_event("PANEL_CONTROL_APPLIED", string.format("revision=%d", revision))
end

local function process_panel_toggle()
    if not state.panel_toggle_pending then
        return false
    end
    if state.reconcile_job ~= nil then
        if not state.panel_toggle_deferred_logged then
            state.panel_toggle_deferred_logged = true
            debug_event("PANEL_TOGGLE_DEFERRED", string.format(
                "sequence=%d reason=reconcile_active",
                state.panel_toggle_sequence
            ))
        end
        return false
    end

    state.panel_toggle_pending = false
    state.panel_toggle_deferred_logged = false
    local sequence = state.panel_toggle_sequence
    if state.panel_toggle_lifecycle_generation ~= state.lifecycle_generation then
        log_event("PANEL_TOGGLE_SKIPPED", string.format(
            "sequence=%d reason=stale_lifecycle",
            sequence
        ))
        return true
    end

    log_event("PANEL_TOGGLE_DISPATCHED", string.format("sequence=%d", sequence))
    local called = call_bridge(BRIDGE_METHOD_TOGGLE_PANEL)
    log_event("PANEL_TOGGLE_COMPLETED", string.format(
        "sequence=%d status=%s",
        sequence,
        called and "ok" or "bridge_unavailable"
    ))
    return true
end

local function runtime_tick()
    if state.panel_toggle_pending then
        local handled = process_panel_toggle()
        emit_metrics(false)
        if handled or state.panel_toggle_pending then
            return
        end
    end

    poll_panel_controls()
    -- __DEPRECATED_20260719__ [reason: the complete 300-species audit established the finite runtime tag contract]
    -- drain_rich_text_audit()
    flush_user_settings_if_due()

    --[[ __DEPRECATED_20260720__ [reason: BeginPlay admission replaced notification-triggered full snapshots]
    if state.runtime_profile.event_admission and state.notification_dirty
        and state.gameplay_active and state.reconcile_job == nil then
        state.notification_dirty = false
        start_safe_reconcile("notify_integrity")
    end
    ]]
    if not runtime_enabled() or state.world_transitioning then
        emit_metrics(false)
        return
    end

    local current = now()
    local camera_location = nil
    local integrity_due = false
    if state.runtime_profile.event_admission and state.gameplay_active
        and state.reconcile_job == nil then
        camera_location = get_camera_location(find_player_controller())
        integrity_due = track_stream_integrity_movement(camera_location)
        local integrity_min_interval_ms = math.max(
            0,
            tonumber(config.STREAM_INTEGRITY_SCAN_MIN_INTERVAL_MS) or 0
        )
        local integrity_interval_elapsed = integrity_min_interval_ms <= 0
            or current >= state.next_stream_integrity_scan_at
        if integrity_due and integrity_interval_elapsed then
            reset_stream_integrity_tracking(camera_location)
            --[[ __DEPRECATED_20260721__ [reason: traversal now performs identity/coordinate census only]
            request_atomic_bridge_rebuild("movement_integrity", config.ATOMIC_REBUILD_MOVEMENT_DEBOUNCE_MS)
            ]]
            run_stream_integrity_scan(camera_location)
            debug_event("STREAM_INTEGRITY_CENSUS_COMPLETED", string.format(
                "distance_m=%d",
                math.floor(tonumber(config.STREAM_INTEGRITY_SCAN_DISTANCE_METERS) or 0)
            ))
        elseif current >= state.next_nearest_refresh_at then
            state.next_nearest_refresh_at = current
                + (config.NEAREST_TARGET_REFRESH_INTERVAL_MS / 1000.0)
            -- __DEPRECATED_20260721__ [reason: scalar membership changes no longer trigger FindAllOf/admission]
            -- request_atomic_bridge_rebuild("nearest_membership")
            refresh_nearest_bridge_targets("runtime_tick", nil, camera_location)
        end
    end

    if process_cached_bridge_resync(camera_location) then
        emit_metrics(false)
        return
    end

    if process_atomic_bridge_rebuild(now()) then
        emit_metrics(false)
        return
    end

    local interval_ms = state.runtime_profile.reconcile_interval_ms
    if interval_ms > 0 and current >= state.next_reconcile_at then
        state.next_reconcile_at = current + (interval_ms / 1000.0)
        reconcile_safely()
    end
    emit_metrics(false)
end

local function runtime_tick_safely()
    local ok, err = pcall(runtime_tick)
    if not ok then
        log_event("ERROR_RUNTIME_TICK", tostring(err))
    end
end

local function register_panel_keybind()
    if state.panel_keybind_registered then
        return
    end
    if type(RegisterKeyBind) ~= "function" or type(Key) ~= "table" or type(ModifierKey) ~= "table" then
        log_event("PANEL_KEYBIND_UNAVAILABLE", "key=Shift+Y")
        return
    end

    local ok, err = pcall(function()
        RegisterKeyBind(Key.Y, { ModifierKey.SHIFT }, function()
            if state.panel_toggle_pending then
                log_event("PANEL_TOGGLE_IGNORED", "reason=dispatch_pending")
                return
            end

            state.panel_toggle_pending = true
            state.panel_toggle_sequence = state.panel_toggle_sequence + 1
            local sequence = state.panel_toggle_sequence
            local lifecycle_generation = state.lifecycle_generation
            state.panel_toggle_lifecycle_generation = lifecycle_generation
            state.panel_toggle_deferred_logged = false
            log_event("PANEL_TOGGLE_REQUESTED", string.format(
                "sequence=%d key=Shift+Y dispatch=runtime_tick",
                sequence
            ))

            -- __DEPRECATED_20260718__ [reason: scheduling a second GameThread callback could race synchronous reconciliation]
            if false then
            local dispatch = function()
                state.panel_toggle_pending = false
                if lifecycle_generation ~= state.lifecycle_generation then
                    log_event("PANEL_TOGGLE_SKIPPED", string.format(
                        "sequence=%d reason=stale_lifecycle",
                        sequence
                    ))
                    return
                end

                log_event("PANEL_TOGGLE_DISPATCHED", string.format("sequence=%d", sequence))
                local called = call_bridge(BRIDGE_METHOD_TOGGLE_PANEL)
                log_event("PANEL_TOGGLE_COMPLETED", string.format(
                    "sequence=%d status=%s",
                    sequence,
                    called and "ok" or "bridge_unavailable"
                ))
            end

            if type(ExecuteInGameThreadWithDelay) == "function" then
                local scheduled, schedule_error = pcall(function()
                    ExecuteInGameThreadWithDelay(config.PANEL_TOGGLE_DELAY_MS, dispatch)
                end)
                if scheduled then
                    return
                end
                log_event("PANEL_TOGGLE_DELAY_FAILED", tostring(schedule_error))
            end

            run_on_game_thread(dispatch)
            end
        end)
    end)
    if not ok then
        log_event("PANEL_KEYBIND_FAILED", tostring(err))
        return
    end
    state.panel_keybind_registered = true
    log_event("PANEL_KEYBIND_READY", "key=Shift+Y")
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
                state.bridge_target_keys = {}
                state.bridge_target_order = {}
                state.bridge_gender_logged = false
                state.bridge_gender_pending_reason = nil
                state.rich_text_audit_consumed_bytes = 0
                state.settings_bridge_applied = false
                state.control_revision = -1
                state.control_pending_logged = false
                log_event("BRIDGE_READY", string.format("event=%s", BRIDGE_READY_EVENT))
                apply_loaded_settings_to_bridge()
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
    state.bridge_target_keys = {}
    state.bridge_target_order = {}
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
    if gameplay_active and runtime_enabled() then
        start_safe_reconcile("bootstrap")
        if state.runtime_profile.reconcile_interval_ms > 0 then
            state.next_reconcile_at = now() + (state.runtime_profile.reconcile_interval_ms / 1000.0)
        end
        -- __DEPRECATED_20260716__ [reason: Blueprint receives all candidates; nearest selection remains Lua-draw-only]
        -- if config.DRAW_ENABLED or config.BLUEPRINT_BRIDGE_ENABLED then
        if config.DRAW_ENABLED then
            select_target()
        end
        sync_bridge_target()
    else
        local skip_reason = inactive_reason or "runtime_off"
        clear_candidate_cache(skip_reason)
        sync_bridge_target()
        state.metrics.raw_monsters = 0
        debug_event("SCAN_SKIPPED", string.format(
            "source=bootstrap reason=%s",
            skip_reason
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

local function register_hook_safely(path, callback, phase)
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
        if phase == "post" then
            return RegisterHook(path, function() end, guarded_callback)
        end
        return RegisterHook(path, guarded_callback)
    end)
    if not ok then
        log_event("HOOK_FAILED", string.format("path=%s error=%s", path, tostring(pre_id)))
        return nil, nil
    end

    debug_event("HOOK_REGISTERED", string.format("path=%s phase=%s", path, phase or "pre"))
    return pre_id, post_id
end

local function register_lifecycle_hooks()
    if state.lifecycle_hooks_registered then
        return
    end
    state.lifecycle_hooks_registered = true

    --[[ __DEPRECATED_20260721__ [reason: lifecycle invalidation promoted every event burst to a full world snapshot]
    local function request_parameter_rebuild(_, _, source)
        request_atomic_bridge_rebuild(source or "parameter_initialized")
    end

    local function request_tracked_lifecycle_rebuild(context_parameter, reason)
        local actor = unwrap(context_parameter)
        if not is_valid(actor) then
            return
        end
        local key = actor_key(actor)
        if key ~= nil and (state.nearest_candidates[key] ~= nil or state.bridge_target_keys[key] ~= nil) then
            request_atomic_bridge_rebuild(reason or "lifecycle")
        end
    end

    local function handle_active_actor_changed()
        request_atomic_bridge_rebuild("active_actor")
    end

    register_hook_safely("/Script/Engine.PlayerController:ServerAcknowledgePossession", function()
        schedule_bootstrap("server_acknowledge_possession")
    end)
    register_hook_safely("/Script/Engine.PlayerController:ClientRestart", function()
        schedule_bootstrap("client_restart")
    end)
    register_hook_safely("/Script/Engine.Actor:ReceiveEndPlay", function(context_parameter)
        request_tracked_lifecycle_rebuild(context_parameter, "end_play")
    end)
    register_hook_safely("/Script/Pal.PalCharacter:OnDeadCharacter", function()
        request_atomic_bridge_rebuild("dead")
    end)
    register_hook_safely("/Script/Pal.PalUtility:PalCaptureSuccess", function()
        request_atomic_bridge_rebuild("captured")
    end)
    register_hook_safely(
        "/Script/Pal.PalCharacter:OnRep_IsPalActiveActor",
        handle_active_actor_changed,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacter:LocalInitialized",
        function()
            request_atomic_bridge_rebuild("local_initialized")
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacterParameterComponent:OnRep_IndividualParameter",
        function()
            request_atomic_bridge_rebuild("parameter_replicated")
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacterParameterComponent:OnInitialize_AfterSetIndividualParameter",
        function(context_parameter, character_parameter)
            request_parameter_rebuild(
                context_parameter,
                character_parameter,
                "parameter_initialized"
            )
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacterParameterComponent:OnInitializedCharacter",
        function(context_parameter, character_parameter)
            request_parameter_rebuild(
                context_parameter,
                character_parameter,
                "component_initialized"
            )
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacter:BroadcastOnCompleteInitializeParameter",
        function()
            request_atomic_bridge_rebuild("initialization_complete")
        end,
        "post"
    )
    ]]

    local function actor_from_hook_arguments(...)
        for index = 1, select("#", ...) do
            local value = select(index, ...)
            local actor = unwrap(value)
            if is_valid(actor) and is_a(actor, resolve_class("monster")) then
                return actor
            end
        end
        for index = 1, select("#", ...) do
            local value = select(index, ...)
            local owner_source = unwrap(value)
            if is_valid(owner_source) then
                local ok, owner = safe_call_no_args(owner_source, "GetOwner")
                owner = ok and unwrap(owner) or nil
                if is_valid(owner) and is_a(owner, resolve_class("monster")) then
                    return owner
                end
            end
        end
        return nil
    end

    local function queue_lifecycle_actor(source, ...)
        local actor = actor_from_hook_arguments(...)
        if actor == nil then
            return false
        end
        local key = actor_key(actor)
        local mode = key ~= nil and state.nearest_candidates[key] ~= nil and "refresh" or "admit"
        return enqueue_event_retry(actor, "character_initialization_pending", mode)
    end

    local function remove_lifecycle_actor(reason, ...)
        local actor = actor_from_hook_arguments(...)
        if actor == nil then
            return false
        end
        return remove_bridge_target(actor, reason)
    end

    register_hook_safely("/Script/Engine.PlayerController:ServerAcknowledgePossession", function()
        schedule_bootstrap("server_acknowledge_possession")
    end)
    register_hook_safely("/Script/Engine.PlayerController:ClientRestart", function()
        schedule_bootstrap("client_restart")
    end)
    register_hook_safely("/Script/Engine.Actor:ReceiveEndPlay", function(...)
        remove_lifecycle_actor("end_play", ...)
    end)
    register_hook_safely("/Script/Pal.PalCharacter:OnDeadCharacter", function(...)
        remove_lifecycle_actor("dead", ...)
    end)
    register_hook_safely("/Script/Pal.PalUtility:PalCaptureSuccess", function(...)
        remove_lifecycle_actor("captured", ...)
    end)
    register_hook_safely(
        "/Script/Pal.PalCharacter:OnRep_IsPalActiveActor",
        function(...)
            local actor = actor_from_hook_arguments(...)
            if actor == nil then
                return
            end
            local ok, active = safe_call_no_args(actor, "GetActiveActorFlag")
            if ok and active == false then
                remove_bridge_target(actor, "active_actor_disabled")
                return
            end
            queue_lifecycle_actor("active_actor", actor)
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacter:LocalInitialized",
        function(...)
            queue_lifecycle_actor("local_initialized", ...)
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacterParameterComponent:OnRep_IndividualParameter",
        function(...)
            queue_lifecycle_actor("parameter_replicated", ...)
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacterParameterComponent:OnInitialize_AfterSetIndividualParameter",
        function(...)
            queue_lifecycle_actor("parameter_initialized", ...)
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacterParameterComponent:OnInitializedCharacter",
        function(...)
            queue_lifecycle_actor("component_initialized", ...)
        end,
        "post"
    )
    register_hook_safely(
        "/Script/Pal.PalCharacter:BroadcastOnCompleteInitializeParameter",
        function(...)
            queue_lifecycle_actor("initialization_complete", ...)
        end,
        "post"
    )
end

local function register_load_map_hooks()
    if state.load_map_hooks_registered then
        return
    end
    state.load_map_hooks_registered = true

    if type(RegisterLoadMapPreHook) == "function" then
        local ok, err = pcall(function()
            RegisterLoadMapPreHook(function()
                if state.capture_active then
                    set_capture_active(false, "map_transition")
                end
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
        NotifyOnNewObject("/Script/Pal.PalMonsterCharacter", function(_constructed_object)
            state.metrics.notification_count = state.metrics.notification_count + 1
            if state.world_transitioning or not state.gameplay_active or not runtime_enabled() then
                return
            end
            if not state.notification_deferred_logged then
                state.notification_deferred_logged = true
                log_event("NOTIFY_OBSERVED", "mode=begin_play_post")
            end
            --[[ __DEPRECATED_20260720__ [reason: construction notifications no longer request a full snapshot]
            state.notification_dirty = true
            state.metrics.notifications_deferred = state.metrics.notifications_deferred + 1
            if state.runtime_profile.event_admission then
                -- __DEPRECATED_20260717__ [reason: queued UObject wrappers can expire before the delayed callback]
                -- state.event_queue[#state.event_queue + 1] = constructed_object
                -- schedule_event_queue_process()
                if not state.notification_deferred_logged then
                    state.notification_deferred_logged = true
                    log_event("NOTIFY_DEFERRED", "mode=next_safe_snapshot")
                end
                return
            end
            if not state.notification_deferred_logged then
                state.notification_deferred_logged = true
                log_event("NOTIFY_DEFERRED", "mode=next_reconcile")
            end
            ]]
            --[[ __DEPRECATED_20260716__ [reason: construction-time classification caused load-time GameThread stalls]
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
            ]]
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
            return LoopInGameThreadWithDelay(config.RUNTIME_TICK_INTERVAL_MS, runtime_tick_safely)
        end)
        if ok then
            state.reconcile_handle = handle
            debug_event("RUNTIME_LOOP_STARTED", "api=LoopInGameThreadWithDelay")
            return
        end
        log_event("RECONCILE_FAILED", tostring(handle))
    end

    if type(LoopAsync) == "function" then
        local ok, err = pcall(function()
            LoopAsync(config.RUNTIME_TICK_INTERVAL_MS, function()
                run_on_game_thread(runtime_tick_safely)
                return false
            end)
        end)
        if ok then
            debug_event("RUNTIME_LOOP_STARTED", "api=LoopAsync_fallback")
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
    if not runtime_enabled() or not config.DRAW_ENABLED or config.MAX_DISPLAY_TARGETS < 1 then
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

initialize_user_settings()
resolve_required_classes()
register_entity_adapters()
register_blueprint_bridge()
register_blueprint_bridge_discovery()
register_monster_notification()
register_lifecycle_hooks()
register_load_map_hooks()
register_draw_hook()
register_panel_keybind()
start_reconcile_loop()
schedule_bootstrap("mod_load")
log_event("BOOT_FILE_LOADED", "server_compatibility=community_untested")
