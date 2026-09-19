# HyperManage icons

The isometric H block, movement arrows and selection brackets represent editing factory buildings. The colored version is used for the tool and mod listing. The single-color version is used for the milestone and category, where the game controls tint and locked-state styling.

SVG files are the editable source. Run `node Tools/Render-Icons.mjs` with the `sharp` package available to render the transparent PNGs and `Resources/Icon128.png`.

Run `Tools/Import-Icons.py` with the Unreal Python commandlet and `-ini:Engine:[ConsoleVariables]:Interchange.FeatureFlags.Import.PNG=0` to update textures and Blueprint icon references. Source metadata uses relative paths.