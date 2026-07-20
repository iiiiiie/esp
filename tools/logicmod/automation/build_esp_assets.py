import unreal


if not hasattr(unreal, "ESPBlueprintAutomationLibrary"):
    raise RuntimeError("ESPBlueprintAutomation plugin is not loaded")

ok = unreal.ESPBlueprintAutomationLibrary.build_palworld_resource_esp_assets()
unreal.log(f"[ESP_AUTOMATION] build_result={ok}")
if not ok:
    raise RuntimeError("PalworldResourceESP asset generation failed")
