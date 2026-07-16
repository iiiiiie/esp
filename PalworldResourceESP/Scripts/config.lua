local config = {}

config.ENABLED = true
config.DEBUG = true

config.BOOTSTRAP_DELAY_MS = 3000
config.RECONCILE_INTERVAL_MS = 5000
config.RECONCILE_BATCH_SIZE = 2
config.RECONCILE_BATCH_DELAY_MS = 16
config.METRIC_INTERVAL_SECONDS = 30

-- __DEPRECATED_20260716__ [reason: admission and display budgets are now separate]
-- config.MAX_CANDIDATES = 512
config.MAX_ADMITTED_ENTITIES = 512
-- __DEPRECATED_20260716__ [reason: the Blueprint bridge now renders all accepted loaded Pals]
-- config.MAX_DISPLAY_TARGETS = 1
-- __DEPRECATED_20260716__ [reason: 512 remains the configurable ceiling, not the default]
-- config.MAX_DISPLAY_TARGETS = 512
config.DEFAULT_DISPLAY_TARGETS = 64
config.MAX_DISPLAY_TARGETS = config.DEFAULT_DISPLAY_TARGETS
config.MAX_CONFIGURABLE_DISPLAY_TARGETS = 512

config.DEFAULT_DISTANCE_M = 2000
config.MIN_DISTANCE_M = 10
config.MAX_DISTANCE_M = 50000
config.ACTIVE_FILTERS = {}

config.FIELD_PROBES_ENABLED = true
config.BLUEPRINT_BRIDGE_ENABLED = true
config.DRAW_ENABLED = false
config.DRAW_MODE = "disabled"
config.TOP_ANCHOR_Y = 28.0
config.LINE_THICKNESS = 1.5
config.LINE_COLOR = {
    R = 0.20,
    G = 0.95,
    B = 0.55,
    A = 0.90,
}

return config
