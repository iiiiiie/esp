local config = {}

config.ENABLED = true
config.DEBUG = true

config.BOOTSTRAP_DELAY_MS = 3000
-- __DEPRECATED_20260720__ [reason: active profiles no longer perform periodic reconciliation]
-- config.RECONCILE_INTERVAL_MS = 5000
config.RECONCILE_BATCH_SIZE = 2
config.RECONCILE_BATCH_DELAY_MS = 16
config.RUNTIME_TICK_INTERVAL_MS = 250
config.NEAREST_TARGET_REFRESH_INTERVAL_MS = 250
config.ATOMIC_REBUILD_DEBOUNCE_MS = 500
config.ATOMIC_REBUILD_MOVEMENT_DEBOUNCE_MS = 250
config.ATOMIC_REBUILD_MAX_COALESCE_MS = 1500
config.ATOMIC_REBUILD_MIN_INTERVAL_MS = 2500
config.MAX_NEAREST_TARGET_RESOLVE_MISSES = 4
config.MAX_NEAREST_TARGET_PATH_RESOLVES_PER_TICK = 2
config.PANEL_TOGGLE_DELAY_MS = 50
config.EVENT_QUEUE_DELAY_MS = 16
config.EVENT_READINESS_RETRY_DELAY_MS = 250
config.MAX_EVENT_READINESS_ATTEMPTS = 8
config.MAX_EVENT_QUEUE = 512
config.STREAM_INTEGRITY_SCAN_DISTANCE_METERS = 120
config.STREAM_INTEGRITY_SCAN_MIN_INTERVAL_MS = 2000
config.METRIC_INTERVAL_SECONDS = 30

config.DEFAULT_RUNTIME_PROFILE_ID = 2
config.DEFAULT_PERFORMANCE_PRESET_ID = 1

-- __DEPRECATED_20260716__ [reason: admission and display budgets are now separate]
-- config.MAX_CANDIDATES = 512
config.MAX_ADMITTED_ENTITIES = 512
-- __DEPRECATED_20260716__ [reason: the Blueprint bridge now renders all accepted loaded Pals]
-- config.MAX_DISPLAY_TARGETS = 1
-- __DEPRECATED_20260716__ [reason: 512 remains the configurable ceiling, not the default]
-- config.MAX_DISPLAY_TARGETS = 512
config.DEFAULT_DISPLAY_TARGETS = 64
config.MAX_DISPLAY_TARGETS = config.DEFAULT_DISPLAY_TARGETS
-- __DEPRECATED_20260717__ [reason: the public panel now caps visible targets at 100]
-- config.MAX_CONFIGURABLE_DISPLAY_TARGETS = 512
config.MAX_CONFIGURABLE_DISPLAY_TARGETS = 100

-- __DEPRECATED_20260717__ [reason: distance now uses one bounded maximum-distance control]
-- config.DEFAULT_DISTANCE_M = 2000
config.DEFAULT_DISTANCE_M = 330
-- __DEPRECATED_20260717__ [reason: the distance range now has a fixed, non-configurable 0m lower bound]
-- config.MIN_DISTANCE_M = 10
config.MIN_DISTANCE_M = 0
-- __DEPRECATED_20260717__ [reason: values beyond the supported visible range are no longer exposed]
-- config.MAX_DISTANCE_M = 50000
config.MAX_DISTANCE_M = 330
config.ACTIVE_FILTERS = { fields = {} }

config.FIELD_PROBES_ENABLED = true
config.BLUEPRINT_BRIDGE_ENABLED = true
config.DRAW_ENABLED = false
config.DRAW_MODE = "disabled"
config.TOP_ANCHOR_Y = 28.0
config.LINE_THICKNESS = 1.5
config.SHOW_TOP_GUIDE_LINE = true
config.SHOW_NAME = true
config.SHOW_LEVEL = true
config.SHOW_DISTANCE = true
config.SHOW_IV = false
config.SHOW_PASSIVE_SKILLS = false
config.LINE_COLOR = {
    R = 0.20,
    G = 0.95,
    B = 0.55,
    A = 0.90,
}

return config
