import unreal
from pathlib import Path

root = Path(__file__).resolve().parent.parent
folder = '/HyperManage/Textures/Icons'
textures = {}
for filename, name in [('HyperManage-Tool.png', 'T_HyperManageTool'), ('HyperManage-Tool-Small.png', 'T_HyperManageToolSmall'), ('HyperManage-Milestone.png', 'T_HyperManageMilestone')]:
    task = unreal.AssetImportTask()
    task.factory = unreal.TextureFactory()
    task.filename = str(root / 'Art/Icons' / filename)
    task.destination_path = folder
    task.destination_name = name
    task.automated = True
    task.replace_existing = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(folder + '/' + name)
    assert isinstance(texture, unreal.Texture2D), name
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    import_data = texture.get_editor_property('asset_import_data')
    assert not isinstance(import_data, unreal.InterchangeAssetImportData), 'Disable Interchange PNG import for portable source metadata.'
    import_data.scripted_add_filename('../../../Art/Icons/' + filename, 0, 'HyperManage icon')
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
    textures[name] = texture

blueprints = ['/HyperManage/Equipment/Desc_HyperManager', '/HyperManage/Schematics/Schematic_HyperManage', '/HyperManage/Schematics/SC_MM']
for path in blueprints:
    asset = unreal.load_asset(path)
    cdo = unreal.get_default_object(asset.generated_class())
    if path == blueprints[0]:
        cdo.set_editor_property('mSmallIcon', textures['T_HyperManageToolSmall'])
        cdo.set_editor_property('mPersistentBigIcon', textures['T_HyperManageTool'])
    elif path == blueprints[2]:
        brush = cdo.get_editor_property('mCategoryIcon')
        brush.set_editor_property('resource_object', textures['T_HyperManageMilestone'])
        cdo.set_editor_property('mCategoryIcon', brush)
    else:
        brush = cdo.get_editor_property('mSchematicIcon')
        brush.set_editor_property('resource_object', textures['T_HyperManageMilestone'])
        cdo.set_editor_property('mSchematicIcon', brush)
        cdo.set_editor_property('mSmallSchematicIcon', textures['T_HyperManageMilestone'])
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
unreal.log('HYPERMANAGE_ICONS_IMPORTED')