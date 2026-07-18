import unreal


ROOT = "/Game/Mods/PalworldResourceESP"


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def create_blueprint(asset_tools, name, asset_class, factory_class, parent_class):
    package_path = f"{ROOT}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(package_path):
        asset = unreal.load_object(None, package_path)
        require(asset is not None, f"Existing asset failed to load: {package_path}")
        unreal.log(f"[ESP_ASSET_CREATE] reusing path={package_path}")
        return asset

    factory = factory_class()
    factory.set_editor_property("parent_class", parent_class)
    asset = asset_tools.create_asset(name, ROOT, asset_class, factory)
    require(asset is not None, f"Failed to create asset: {package_path}")

    if isinstance(asset, unreal.Blueprint):
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    require(
        unreal.EditorAssetLibrary.save_loaded_asset(asset),
        f"Failed to save asset: {package_path}",
    )
    unreal.log(
        f"[ESP_ASSET_CREATE] path={package_path} class={asset.get_class().get_name()}"
    )
    return asset


if unreal.EditorAssetLibrary.does_directory_exist(ROOT):
    unreal.log(f"[ESP_ASSET_CREATE] reusing directory={ROOT}")
else:
    require(unreal.EditorAssetLibrary.make_directory(ROOT), f"Failed to create directory: {ROOT}")
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

create_blueprint(
    asset_tools,
    "ModActor",
    unreal.Blueprint,
    unreal.BlueprintFactory,
    unreal.Actor,
)
create_blueprint(
    asset_tools,
    "BP_ESPBridge",
    unreal.Blueprint,
    unreal.BlueprintFactory,
    unreal.Actor,
)
create_blueprint(
    asset_tools,
    "WBP_ESPOverlay",
    unreal.WidgetBlueprint,
    unreal.WidgetBlueprintFactory,
    unreal.UserWidget,
)
create_blueprint(
    asset_tools,
    "WBP_ESPPanel",
    unreal.WidgetBlueprint,
    unreal.WidgetBlueprintFactory,
    unreal.UserWidget,
)
create_blueprint(
    asset_tools,
    "WBP_ESPPassiveEntry",
    unreal.WidgetBlueprint,
    unreal.WidgetBlueprintFactory,
    unreal.UserWidget,
)

unreal.log("[ESP_ASSET_CREATE] complete")
