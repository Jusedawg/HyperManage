import unreal
from pathlib import Path
out=[]
for path in ['/HyperManage/UI/MMToolsUI','/HyperManage/UI/MMInfoWidget']:
    a=unreal.load_asset(path)
    task=unreal.AssetExportTask()
    task.object=a
    task.filename=str(Path(unreal.Paths.project_saved_dir())/(path.rsplit('/',1)[-1]+'.copy'))
    task.automated=True
    task.prompt=False
    task.replace_identical=True
    out.append(str(unreal.Exporter.run_asset_export_task(task)))
unreal.log('VISUAL_INSPECT '+str(out))
