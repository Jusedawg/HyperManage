import unreal
path = '/HyperManage/Materials/M_SelectionOutline'
material = unreal.load_asset(path)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_SelectionOutline', '/HyperManage/Materials', unreal.Material, unreal.MaterialFactoryNew())
lib = unreal.MaterialEditingLibrary
lib.delete_all_material_expressions(material)
material.set_editor_property('material_domain', unreal.MaterialDomain.MD_POST_PROCESS)
material.set_editor_property('blendable_location', unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
custom = lib.create_material_expression(material, unreal.MaterialExpressionCustom, 0, 0)
custom.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in ['Scene', 'Stencil', 'Depth', 'VisibleDepth', 'PixelSize']:
    item = unreal.CustomInput()
    item.set_editor_property('input_name', name)
    inputs.append(item)
custom.set_editor_property('inputs', inputs)
custom.set_editor_property('code', '''
float selected = step(9.5, Stencil.r) * step(Stencil.r, 12.5);
float visible = step(Depth.r, VisibleDepth.r + 2.0);
float2 uv = GetDefaultSceneTextureUV(Parameters, 25);
float edge = 0;
float2 offsets[4] = {float2(1,0), float2(-1,0), float2(0,1), float2(0,-1)};
[unroll] for (int i=0; i<4; ++i) {
    float neighbor = SceneTextureLookup(uv + offsets[i] * PixelSize.rg * 2, 25, false).r;
    edge = max(edge, step(0.5, abs(neighbor - Stencil.r)));
}
float3 ink = Stencil.r > 11.5 ? float3(1.0,0.18,0.13) : Stencil.r > 10.5 ? float3(0.1,0.75,1.0) : float3(0.12,1.0,0.28);
return lerp(Scene.rgb, ink, selected * visible * lerp(0.12, 0.90, edge));
''')
for name, texture in [('Scene', unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0), ('Stencil', unreal.SceneTextureId.PPI_CUSTOM_STENCIL), ('Depth', unreal.SceneTextureId.PPI_CUSTOM_DEPTH), ('VisibleDepth', unreal.SceneTextureId.PPI_SCENE_DEPTH)]:
    node = lib.create_material_expression(material, unreal.MaterialExpressionSceneTexture, -400, 0)
    node.set_editor_property('scene_texture_id', texture)
    lib.connect_material_expressions(node, 'Color', custom, name)
    if name == 'Stencil':
        lib.connect_material_expressions(node, 'InvSize', custom, 'PixelSize')
lib.connect_material_property(custom, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
unreal.log('HYPERMANAGE_SELECTION_OUTLINE_OK')
