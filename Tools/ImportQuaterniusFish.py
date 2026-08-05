import os
import unreal


SOURCE_DIRECTORY = r"G:\boids\ThirdParty\QuaterniusAnimatedFish\OBJ"
DESTINATION_PATH = "/Game/Fish/Quaternius"
FISH_NAMES = ["Fish1", "Fish2", "Fish3", "Manta ray", "Shark"]


def import_fish(name):
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SOURCE_DIRECTORY, name + ".obj")
    task.destination_path = DESTINATION_PATH
    task.destination_name = name.replace(" ", "_")
    task.automated = True
    task.replace_existing = True
    task.replace_existing_settings = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = True
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.generate_lightmap_u_vs = False
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError("Import failed: " + task.filename)
    unreal.log("Imported {} -> {}".format(name, task.imported_object_paths))


for fish_name in FISH_NAMES:
    import_fish(fish_name)

unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH, only_if_is_dirty=False, recursive=True)
unreal.log("Quaternius fish import complete")
