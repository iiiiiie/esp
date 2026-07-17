local runtime_profiles = {}

runtime_profiles.PROFILE = {
    OFF = 0,
    SNAPSHOT_ONCE = 1,
    CHUNKED_CURRENT = 2,
    EVENT_FIRST = 3,
}

runtime_profiles.PRESET = {
    LOW = 0,
    BALANCED = 1,
    QUALITY = 2,
}

local profile_names = {
    [runtime_profiles.PROFILE.OFF] = "off",
    [runtime_profiles.PROFILE.SNAPSHOT_ONCE] = "snapshot_once",
    [runtime_profiles.PROFILE.CHUNKED_CURRENT] = "chunked_current",
    [runtime_profiles.PROFILE.EVENT_FIRST] = "event_first",
}

local presets = {
    [runtime_profiles.PRESET.LOW] = {
        name = "low",
        reconcile_interval_ms = 30000,
        max_display_targets = 32,
    },
    [runtime_profiles.PRESET.BALANCED] = {
        name = "balanced",
        reconcile_interval_ms = 15000,
        max_display_targets = 64,
    },
    [runtime_profiles.PRESET.QUALITY] = {
        name = "quality",
        reconcile_interval_ms = 5000,
        max_display_targets = 128,
    },
}

local function integer_or(value, fallback)
    value = tonumber(value)
    if value == nil then
        return fallback
    end
    return math.floor(value)
end

function runtime_profiles.normalize_profile_id(value)
    local profile_id = integer_or(value, runtime_profiles.PROFILE.CHUNKED_CURRENT)
    if profile_names[profile_id] == nil then
        return runtime_profiles.PROFILE.CHUNKED_CURRENT
    end
    return profile_id
end

function runtime_profiles.normalize_preset_id(value)
    local preset_id = integer_or(value, runtime_profiles.PRESET.BALANCED)
    if presets[preset_id] == nil then
        return runtime_profiles.PRESET.BALANCED
    end
    return preset_id
end

function runtime_profiles.resolve(profile_value, preset_value)
    local profile_id = runtime_profiles.normalize_profile_id(profile_value)
    local preset_id = runtime_profiles.normalize_preset_id(preset_value)
    local preset = presets[preset_id]
    local profile = {
        id = profile_id,
        name = profile_names[profile_id],
        preset_id = preset_id,
        preset_name = preset.name,
        runtime_enabled = profile_id ~= runtime_profiles.PROFILE.OFF,
        scan_on_enter = profile_id ~= runtime_profiles.PROFILE.OFF,
        event_admission = profile_id == runtime_profiles.PROFILE.EVENT_FIRST,
        reconcile_interval_ms = 0,
        batch_size = 2,
        batch_delay_ms = 16,
        max_display_targets = preset.max_display_targets,
    }

    if profile_id == runtime_profiles.PROFILE.CHUNKED_CURRENT then
        profile.reconcile_interval_ms = 5000
    elseif profile_id == runtime_profiles.PROFILE.EVENT_FIRST then
        profile.reconcile_interval_ms = 30000
        profile.batch_size = 1
    end

    return profile
end

return runtime_profiles
