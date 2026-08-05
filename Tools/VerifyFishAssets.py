import unreal


ASSETS = [
    "/Game/Fish/Quaternius/Fish1.Fish1",
    "/Game/Fish/Quaternius/Fish2.Fish2",
    "/Game/Fish/Quaternius/Fish3.Fish3",
    "/Game/Fish/Quaternius/Manta_ray.Manta_ray",
    "/Game/Fish/Quaternius/Shark.Shark",
]

for asset_path in ASSETS:
    mesh = unreal.load_asset(asset_path)
    if not mesh:
        unreal.log_error("FISH_VERIFY load failed: " + asset_path)
        continue
    bounds = mesh.get_bounds()
    unreal.log(
        "FISH_VERIFY {} extent={} sphere_radius={} materials={}".format(
            asset_path,
            bounds.box_extent,
            bounds.sphere_radius,
            len(mesh.get_editor_property("static_materials")),
        )
    )
