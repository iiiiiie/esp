local config = {}

config.ENABLED = true
config.DEBUG = true

config.BOOTSTRAP_DELAY_MS = 3000
config.RECONCILE_INTERVAL_MS = 5000
config.METRIC_INTERVAL_SECONDS = 30

config.MAX_CANDIDATES = 512
-- __DEPRECATED_20260716__ [reason: the Blueprint bridge now renders all accepted loaded Pals]
-- config.MAX_DISPLAY_TARGETS = 1
config.MAX_DISPLAY_TARGETS = 512

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
