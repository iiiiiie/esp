import unreal


PACKAGE_PATH = "/Game/Mods/PalworldResourceESP"
ASSET_NAME = "PalworldResourceESP"
ASSET_PATH = f"{PACKAGE_PATH}/{ASSET_NAME}"


def set_property(asset, name, value):
    try:
        asset.set_editor_property(name, value)
        unreal.log(f"[ESP_LABEL] set {name}={value}")
        return True
    except Exception as exc:
        unreal.log_warning(f"[ESP_LABEL] property unavailable {name}: {exc}")
        return False


existing = unreal.load_object(None, ASSET_PATH)
if existing is not None:
    label = unreal.PrimaryAssetLabel.cast(existing)
    unreal.log(f"[ESP_LABEL] reusing {ASSET_PATH}")
else:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    label = tools.create_asset(ASSET_NAME, PACKAGE_PATH, unreal.PrimaryAssetLabel, None)
    if label is None:
        raise RuntimeError(f"Unable to create PrimaryAssetLabel at {ASSET_PATH}")
    unreal.log(f"[ESP_LABEL] created {ASSET_PATH}")

set_property(label, "label_assets_in_my_directory", True)
set_property(label, "is_runtime_label", True)

rules = label.get_editor_property("rules")
rules.set_editor_property("priority", 100)
rules.set_editor_property("chunk_id", 1001)
rules.set_editor_property("apply_recursively", True)
rules.set_editor_property("cook_rule", unreal.PrimaryAssetCookRule.ALWAYS_COOK)
label.set_editor_property("rules", rules)
unreal.log(f"[ESP_LABEL] rules={rules.export_text()}")

unreal.EditorAssetLibrary.save_asset(ASSET_PATH, only_if_is_dirty=False)
saved = unreal.load_object(None, ASSET_PATH)
if saved is None:
    raise RuntimeError(f"PrimaryAssetLabel did not reload after save: {ASSET_PATH}")

saved_rules = saved.get_editor_property("rules")
saved_chunk = saved_rules.get_editor_property("chunk_id")
saved_runtime = saved.get_editor_property("is_runtime_label")
saved_directory = saved.get_editor_property("label_assets_in_my_directory")
if saved_chunk != 1001 or not saved_runtime or not saved_directory:
    raise RuntimeError(
        "PrimaryAssetLabel verification failed: "
        f"chunk={saved_chunk} runtime={saved_runtime} directory={saved_directory}"
    )

unreal.log(
    f"[ESP_LABEL] saved path={saved.get_path_name()} "
    f"chunk={saved_chunk} runtime={saved_runtime} directory={saved_directory}"
)
