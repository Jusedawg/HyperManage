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
 return true;
}
#endif
