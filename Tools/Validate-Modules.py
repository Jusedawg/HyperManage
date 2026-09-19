import json
from pathlib import Path
import unreal

world_module = unreal.get_default_object(unreal.load_class(None, '/Script/HyperManage.HyperManageWorldModule'))
instance_module = unreal.get_default_object(unreal.load_class(None, '/Script/HyperManage.HyperManageInstanceModule'))
schematics = [item.get_path_name() for item in world_module.get_editor_property('mSchematics')]
rpcs = [item.get_path_name() for item in instance_module.get_editor_property('RemoteCallObjects')]
assert world_module.get_editor_property('bRootModule'), 'World module is not a root module'
assert instance_module.get_editor_property('bRootModule'), 'Game-instance module is not a root module'
assert schematics == ['/HyperManage/Schematics/Schematic_HyperManage.Schematic_HyperManage_C'], schematics
assert '/Script/HyperManage.HyperManageRCO' in rpcs, rpcs
cls = unreal.EditorAssetLibrary.load_blueprint_class('/HyperManage/Equipment/Desc_HyperManager')
equipment = unreal.get_default_object(cls).get_editor_property('mEquipmentClass')
assert equipment.get_path_name() == '/Script/HyperManage.HyperManageEquip', equipment
recipe_class = unreal.EditorAssetLibrary.load_blueprint_class('/HyperManage/Recipes/HyperManager/Recipe_HyperManager')
assert recipe_class, 'Tool recipe is missing'
recipe = unreal.get_default_object(recipe_class)
products = recipe.get_editor_property('mProduct')
assert len(products) == 1, products
assert products[0].get_editor_property('ItemClass') == cls, 'Recipe does not produce the tool descriptor'
schematic_class = unreal.EditorAssetLibrary.load_blueprint_class('/HyperManage/Schematics/Schematic_HyperManage')
schematic = unreal.get_default_object(schematic_class)
unlocked_recipes = []
for unlock in schematic.get_editor_property('mUnlocks'):
    if isinstance(unlock, unreal.FGUnlockRecipe):
        unlocked_recipes.extend(unlock.get_editor_property('mRecipes'))
assert recipe_class in unlocked_recipes, 'Schematic does not unlock the tool recipe'
result = {'schematics': schematics, 'remote_call_objects': rpcs, 'equipment_class': equipment.get_path_name()}
output = Path(unreal.Paths.project_saved_dir()) / 'HyperManage-ModuleValidation.json'
output.write_text(json.dumps(result, indent=2), encoding='utf-8')
unreal.log('HYPERMANAGE_MODULE_VALIDATION_OK')