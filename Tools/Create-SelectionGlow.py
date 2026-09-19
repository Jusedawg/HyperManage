import unreal
path = '/HyperManage/Materials/M_SelectionGlow'
material = unreal.load_asset(path)
if material is None:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_SelectionGlow', '/HyperManage/Materials', unreal.Material, unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided', True)
lib = unreal.MaterialEditingLibrary
color = lib.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -500, 0)
color.set_editor_property('parameter_name', 'GlowColor')
color.set_editor_property('default_value', unreal.LinearColor(0.05, 1.0, 0.12, 1.0))
fresnel = lib.create_material_expression(material, unreal.MaterialExpressionFresnel, -500, 250)
fresnel.set_editor_property('exponent', 3.0)
fresnel.set_editor_property('base_reflect_fraction', 0.12)
lib.connect_material_property(color, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(fresnel, '', unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
unreal.log('HYPERMANAGE_SELECTION_GLOW_OK')
