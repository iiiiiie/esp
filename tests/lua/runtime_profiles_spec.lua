local helper = require("test_helper")
local runtime_profiles = require("core.runtime_profiles")

return {
    helper.case("runtime profile defaults are deterministic", function()
        local profile = runtime_profiles.resolve(nil, nil)
        helper.equal(profile.id, runtime_profiles.PROFILE.CHUNKED_CURRENT)
        helper.equal(profile.preset_id, runtime_profiles.PRESET.BALANCED)
        helper.equal(profile.reconcile_interval_ms, 5000)
        helper.equal(profile.max_display_targets, 64)
    end),

    helper.case("off disables discovery and display", function()
        local profile = runtime_profiles.resolve(runtime_profiles.PROFILE.OFF, runtime_profiles.PRESET.QUALITY)
        helper.falsy(profile.runtime_enabled)
        helper.falsy(profile.scan_on_enter)
        helper.equal(profile.reconcile_interval_ms, 0)
        helper.equal(profile.max_display_targets, 128)
    end),

    helper.case("snapshot scans once without periodic reconciliation", function()
        local profile = runtime_profiles.resolve(runtime_profiles.PROFILE.SNAPSHOT_ONCE, runtime_profiles.PRESET.LOW)
        helper.truthy(profile.runtime_enabled)
        helper.truthy(profile.scan_on_enter)
        helper.falsy(profile.event_admission)
        helper.equal(profile.reconcile_interval_ms, 0)
    end),

    helper.case("event-first fixes integrity interval while preset controls display budget", function()
        local low = runtime_profiles.resolve(runtime_profiles.PROFILE.EVENT_FIRST, runtime_profiles.PRESET.LOW)
        local quality = runtime_profiles.resolve(runtime_profiles.PROFILE.EVENT_FIRST, runtime_profiles.PRESET.QUALITY)
        helper.truthy(low.event_admission)
        helper.equal(low.reconcile_interval_ms, 30000)
        helper.equal(low.max_display_targets, 32)
        helper.equal(quality.reconcile_interval_ms, 30000)
        helper.equal(quality.max_display_targets, 128)
    end),

    helper.case("invalid identifiers fail closed to current balanced", function()
        local profile = runtime_profiles.resolve(99, -5)
        helper.equal(profile.id, runtime_profiles.PROFILE.CHUNKED_CURRENT)
        helper.equal(profile.preset_id, runtime_profiles.PRESET.BALANCED)
    end),
}
