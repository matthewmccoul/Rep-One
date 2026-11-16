# Creates /Game/Maps/DemoLevel from the engine's basic template (sky + light), removes the template floor
# (the demo builds its own floor at Play), and saves it.
import unreal

PATH = "/Game/Maps/DemoLevel"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(PATH):
    les.load_level(PATH)
else:
    les.new_level_from_template(PATH, "/Engine/Maps/Templates/Template_Default")

removed = []
for a in eas.get_all_level_actors():
    label = a.get_actor_label()
    if isinstance(a, unreal.StaticMeshActor) and "floor" in label.lower():
        removed.append(label)
        eas.destroy_actor(a)
les.save_current_level()
unreal.log("[DDSMOKE] DemoLevel saved; removed: %s; actors: %s" % (removed, [a.get_actor_label() for a in eas.get_all_level_actors()]))
