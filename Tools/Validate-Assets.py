import json
from pathlib import Path
import unreal

unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(['/HyperManage'], force_rescan=True)
assets = sorted(unreal.EditorAssetLibrary.list_assets('/HyperManage', recursive=True, include_folder=False))
if len(assets) != 75:
    raise RuntimeError('Expected 75 assets, found ' + str(len(assets)))
report = {'assets': [], 'equipment_class': None}
for path in assets:
    asset = unreal.EditorAssetLibrary.load_asset(path)
    if asset is None:
        raise RuntimeError('Could not load ' + path)
    if isinstance(asset, unreal.Blueprint):
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    if path == '/HyperManage/Equipment/Desc_HyperManager.Desc_HyperManager':
        cls = unreal.EditorAssetLibrary.load_blueprint_class(path)
        equipment = unreal.get_default_object(cls).get_editor_property('mEquipmentClass')
        report['equipment_class'] = equipment.get_path_name() if equipment else None
        if report['equipment_class'] != '/Script/HyperManage.HyperManageEquip':
            raise RuntimeError('Unexpected equipment class: ' + str(report['equipment_class']))
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Could not save ' + path)
    report['assets'].append(path)
output = Path(unreal.Paths.project_saved_dir()) / 'HyperManage-AssetValidation.json'
output.write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('HYPERMANAGE_ASSET_VALIDATION_OK: ' + str(len(report['assets'])) + ' assets')