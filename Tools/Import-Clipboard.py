import unreal
from pathlib import Path
root=Path(__file__).resolve().parent.parent/'Art/Clipboard'
tasks=[]
for file, name in [('clipboard-concept.png','T_Clipboard')]:
    task=unreal.AssetImportTask()
    task.factory=unreal.TextureFactory()
    task.filename=str(root/file)
    task.destination_path='/HyperManage/UI/Jusedawg'
    task.destination_name=name
    task.automated=True
    task.replace_existing=True
    task.save=False
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    unreal.log('CLIPBOARD_IMPORTED '+str(task.imported_object_paths))
texture=unreal.load_asset('/HyperManage/UI/Jusedawg/T_Clipboard')
assert isinstance(texture, unreal.Texture2D)
texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
# Source art is located relative to this script; do not retain machine-specific import metadata.
texture.get_editor_property('asset_import_data').scripted_add_filename('../../../Art/Clipboard/clipboard-concept.png', 0, 'Clipboard artwork')

import_data = texture.get_editor_property('asset_import_data')
assert not isinstance(import_data, unreal.InterchangeAssetImportData), 'Run with -ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.PNG=0 to avoid embedding absolute source paths.'
assert unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('CLIPBOARD_ASSETS_OK')
