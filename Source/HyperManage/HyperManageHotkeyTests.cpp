#include "HyperManageConfig.h"
#include "HyperManageInput.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageDismantleHotkeyTest, "HyperManage.Input.DismantleHotkey", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageDismantleHotkeyTest::RunTest(const FString& Parameters)
{
 FHyperManageKeyConfigs Keys;
 Keys.DoNotEditConfigFormatVersion = TEXT("1.0");
 Keys.ActionKeys.Add(FHyperManageKeyConfig(DeleteSelection, EKeys::Invalid, false, false, false, false));
 UHyperManageConfiguration::UpgradeDismantleBinding(Keys);
 TestTrue(TEXT("Old unbound default becomes Delete"), Keys.ActionKeys[0].Key == EKeys::Delete);
 TestFalse(TEXT("Dismantle cannot repeat"), Keys.ActionKeys[0].UseRepeats);
 Keys.ActionKeys[0] = FHyperManageKeyConfig(DeleteSelection, EKeys::G, true, false, false, true);
 Keys.DoNotEditConfigFormatVersion = TEXT("1.0");
 UHyperManageConfiguration::UpgradeDismantleBinding(Keys);
 TestTrue(TEXT("Existing custom shortcut retained"), Keys.ActionKeys[0].Key == EKeys::G && Keys.ActionKeys[0].Ctrl);
 TestFalse(TEXT("Custom shortcut repeats disabled"), Keys.ActionKeys[0].UseRepeats);
 Keys.ActionKeys[0] = FHyperManageKeyConfig(DeleteSelection, EKeys::Invalid, false, false, false, false);
 UHyperManageConfiguration::UpgradeDismantleBinding(Keys);
 TestTrue(TEXT("Explicit unbinding in new format retained"), Keys.ActionKeys[0].Key == EKeys::Invalid);
 Keys.DoNotEditConfigFormatVersion = TEXT("1.0");
 Keys.ActionKeys.Add(FHyperManageKeyConfig(Undo, EKeys::Delete, false, false, false, false));
 UHyperManageConfiguration::UpgradeDismantleBinding(Keys);
 TestTrue(TEXT("Existing Delete binding is not shadowed"), Keys.ActionKeys[0].Key == EKeys::Invalid);
 // No System is assigned: repeats/releases must return before any action dispatch.
 auto* Input = NewObject<UHyperManageInput>();
 Input->PerformIndexedAction(EKeys::Delete, DeleteSelection, IE_Repeat);
 Input->PerformIndexedAction(EKeys::Delete, DeleteSelection, IE_Released);
 for (const auto Action : {CopySelection, PlaceCopy, CancelCopy, ToggleCopyMode, ClickPlaceCopy})
 {
  Input->PerformIndexedAction(EKeys::C, Action, IE_Repeat);
  Input->PerformIndexedAction(EKeys::C, Action, IE_Released);
 }
 FHyperManageKeyConfigs CopyKeys;
 CopyKeys.DoNotEditConfigFormatVersion = TEXT("1.1");
 CopyKeys.ActionKeys.Add(FHyperManageKeyConfig(CopySelection, EKeys::Invalid, false, false, false, false));
 UHyperManageConfiguration::UpgradeCopyBindings(CopyKeys);
 TestEqual(TEXT("Copy actions are registered once"), CopyKeys.ActionKeys.Num(), 3);
 TestTrue(TEXT("Old copy placeholder gains Ctrl+C"), CopyKeys.ActionKeys[0].Key == EKeys::C && CopyKeys.ActionKeys[0].Ctrl);
 TestTrue(TEXT("Place defaults to Ctrl+V"), CopyKeys.ActionKeys[1].Key == EKeys::V && CopyKeys.ActionKeys[1].Ctrl);
 TestTrue(TEXT("Cancel defaults to Escape"), CopyKeys.ActionKeys[2].Key == EKeys::Escape && !CopyKeys.ActionKeys[2].Ctrl);
 CopyKeys.ActionKeys[0] = FHyperManageKeyConfig(CopySelection, EKeys::Invalid, false, false, false, false);
 CopyKeys.ActionKeys[1] = FHyperManageKeyConfig(PlaceCopy, EKeys::P, true, false, true, true);
 UHyperManageConfiguration::UpgradeDismantleBinding(CopyKeys);
 UHyperManageConfiguration::UpgradeCopyBindings(CopyKeys);
 TestTrue(TEXT("New-format explicit unbind survives reload migration"), CopyKeys.ActionKeys[0].Key == EKeys::Invalid);
 TestTrue(TEXT("Custom paste mapping survives with repeats disabled"), CopyKeys.ActionKeys[1].Key == EKeys::P && CopyKeys.ActionKeys[1].Shift && !CopyKeys.ActionKeys[1].UseRepeats);
 CopyKeys.DoNotEditConfigFormatVersion = TEXT("1.1");
 CopyKeys.ActionKeys.Add(FHyperManageKeyConfig(Undo, EKeys::C, true, false, false, false));
 UHyperManageConfiguration::UpgradeCopyBindings(CopyKeys);
 TestTrue(TEXT("Existing Ctrl+C binding is not shadowed"), CopyKeys.ActionKeys[0].Key == EKeys::Invalid);
 auto* Config = NewObject<UHyperManageConfiguration>();
 Config->MMKeyConfigs = CopyKeys;
 TestEqual(TEXT("Shortcut hint follows custom chord"), Config->GetShortcutLabel(PlaceCopy), FString(TEXT("Ctrl+Shift+P")));
 TestEqual(TEXT("Shortcut hint reflects unbinding"), Config->GetShortcutLabel(CopySelection), FString(TEXT("Unbound")));
 FHyperManageKeyConfigs MouseKeys;
 MouseKeys.DoNotEditConfigFormatVersion = TEXT("1.2");
 MouseKeys.ActionKeys.Add(FHyperManageKeyConfig(Undo, EKeys::LeftMouseButton, false, false, false, false));
 UHyperManageConfiguration::UpgradePlacementBindings(MouseKeys);
 TestTrue(TEXT("Middle click switches copy placement mode"), MouseKeys.ActionKeys[1].Key == EKeys::MiddleMouseButton);
 TestTrue(TEXT("Custom left click is not shadowed"), MouseKeys.ActionKeys[2].Key == EKeys::Invalid);
 MouseKeys.ActionKeys[1] = FHyperManageKeyConfig(ToggleCopyMode, EKeys::Invalid, false, false, false, false);
 UHyperManageConfiguration::UpgradeCopyBindings(MouseKeys);
 TestEqual(TEXT("Old migration cannot downgrade new format"), MouseKeys.DoNotEditConfigFormatVersion, FString(TEXT("1.3")));
 UHyperManageConfiguration::UpgradePlacementBindings(MouseKeys);
 TestTrue(TEXT("Unbound free middle click is not reassigned"), MouseKeys.ActionKeys[1].Key == EKeys::Invalid);
 TestTrue(TEXT("Explicit unbinding survives placement migration"), MouseKeys.ActionKeys[2].Key == EKeys::Invalid);
 return true;
}
#endif
