#include "MaterialDomain.h"
#include "HyperManageLightweight.h"
#include "HyperManageTransform.h"
#include "HyperManageEquip.h"
#include "Components/SceneComponent.h"
#include "Misc/AutomationTest.h"
#include "HyperManageSelection.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "HyperManageToolWidget.h"
#include "HyperManageConfig.h"
#include "HyperManageUndo.h"
#include "Components/ExpandableArea.h"
#include "Components/BoxComponent.h"
#include "Components/CheckBox.h"
#include "WheeledVehicles/FGTargetPoint.h"
#include "JsonObjectConverter.h"
#include <limits>
#include "HyperManageClipboardWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Misc/Paths.h"
#include "Widgets/SWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/NamedSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageLightweightIdentityTest, "HyperManage.Lightweight.StaleIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageLightweightIdentityTest::RunTest(const FString& Parameters)
{
	FRuntimeBuildableInstanceData Data;
	Data.Transform = FTransform::Identity;
	Data.BuiltWithRecipe = UFGRecipe::StaticClass();
	// Identity checking only examines the runtime handle count; no instance is spawned here.
	Data.Handles.AddDefaulted();
	FHyperManageLightweightRef Ref;
	Ref.Recipe = Data.BuiltWithRecipe;
	TestTrue(TEXT("Matching live record"), Ref.Matches(&Data));
	Data.Transform.SetLocation(FVector(10, 0, 0));
	TestFalse(TEXT("Changed transform rejects stale selection"), Ref.Matches(&Data));
	Data.Transform = FTransform::Identity;
	Ref.Recipe = nullptr;
	TestFalse(TEXT("Different recipe rejects reused record"), Ref.Matches(&Data));
	Ref.Recipe = Data.BuiltWithRecipe;
	Data.Handles.Empty();
	TestFalse(TEXT("Removed instance rejected"), Ref.Matches(&Data));
	TestFalse(TEXT("Missing instance rejected"), Ref.Matches(nullptr));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageLightweightBoundsTest, "HyperManage.Lightweight.TransformBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageLightweightBoundsTest::RunTest(const FString& Parameters)
{
	FTransform Transform = FTransform::Identity;
	TestTrue(TEXT("Identity accepted"), HyperManageLightweight::IsValidTransform(Transform));
	Transform.SetScale3D(FVector(-1, 2, 3));
	TestTrue(TEXT("Mirroring accepted"), HyperManageLightweight::IsValidTransform(Transform));
	Transform.SetScale3D(FVector(0, 1, 1));
	TestFalse(TEXT("Degenerate scale rejected"), HyperManageLightweight::IsValidTransform(Transform));
	Transform = FTransform::Identity;
	Transform.SetLocation(FVector(1.e10, 0, 0));
	TestFalse(TEXT("Out of bounds location rejected"), HyperManageLightweight::IsValidTransform(Transform));
	Transform = FTransform::Identity;
	Transform.SetRotation(FQuat(0, 0, 0, 2));
	TestFalse(TEXT("Non-normalized rotation rejected"), HyperManageLightweight::IsValidTransform(Transform));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageLightweightPivotTest, "HyperManage.Lightweight.PivotTransforms", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageLightweightPivotTest::RunTest(const FString& Parameters)
{
	auto* Calculator = NewObject<UHyperManageTransform>();
	const FTransform Initial(FQuat::Identity, FVector(100, 0, 0));
	FHyperManageTransformData Data;
	Data.IsRot = true;
	Data.PivotAngle = 90;
	Data.PivotQuat = FQuat(FVector::UpVector, UE_HALF_PI);
	auto Result = Calculator->ComputeTransform(Initial, Data);
	TestTrue(TEXT("Group rotation orbits pivot"), Result.GetLocation().Equals(FVector(0, 100, 0), 0.001));
	TestTrue(TEXT("Group rotation rotates object"), Result.GetRotation().Equals(Data.PivotQuat, 0.001));
	Data.GroupMode = false;
	Data.ViewRelative = false;
	Data.TransformAxis = EAxis::Z;
	Data.Rot = FRotator(0, 90, 0);
	Result = Calculator->ComputeTransform(Initial, Data);
	TestTrue(TEXT("Individual rotation keeps position"), Result.GetLocation().Equals(Initial.GetLocation()));
	TestTrue(TEXT("Individual rotation uses own pivot"), Result.GetRotation().Equals(Data.PivotQuat, 0.001));
	Data = FHyperManageTransformData();
	Data.IsScale = true;
	Data.Scale = FVector(2);
	Result = Calculator->ComputeTransform(Initial, Data);
	TestTrue(TEXT("Group scale changes pivot distance"), Result.GetLocation().Equals(FVector(200, 0, 0)));
	TestTrue(TEXT("Group scale changes object size"), Result.GetScale3D().Equals(FVector(2)));
	Data.SetSame = true;
	Data.GroupMode = false;
	Data.TargetRotation = FQuat(FVector::UpVector, 0.5);
	Result = Calculator->ComputeTransform(Initial, Data);
	TestTrue(TEXT("Match rotation uses target snapshot"), Result.GetRotation().Equals(Data.TargetRotation));
	TestTrue(TEXT("Match rotation retains scale"), Result.GetScale3D().Equals(Initial.GetScale3D()));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageEquipmentRootTest, "HyperManage.Equipment.AttachmentRoot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageEquipmentRootTest::RunTest(const FString& Parameters)
{
	const auto* Equipment = GetDefault<AHyperManageEquip>();
	const auto* Root = Equipment->GetRootComponent();
	TestNotNull(TEXT("Native equipment provides the root required by AFGEquipment::Equip"), Root);
	if (Root) {
		TestTrue(TEXT("Root is a default subobject created for every equipment instance"), Root->HasAnyFlags(RF_DefaultSubObject));
		TestTrue(TEXT("Root can attach and move with the player's hand"), Root->Mobility == EComponentMobility::Movable);
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageSelectionOverlayTest, "HyperManage.Selection.PreservesMaterials", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageSelectionOverlayTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	auto* Actor = World->SpawnActor<AStaticMeshActor>();
	auto* Mesh = Actor->GetStaticMeshComponent();
	auto* Original = UMaterial::GetDefaultMaterial(MD_Surface);
	auto* PreviousOverlay = NewObject<UMaterial>();
	Mesh->SetMaterial(0, Original);
	Mesh->SetOverlayMaterial(PreviousOverlay);
	auto* Selection = NewObject<UHyperManageSelection>();
	Selection->Init();
	FSelectedActorInfo Info;
	Selection->ShowHologram(Actor, Info);
	TestTrue(TEXT("Selecting preserves the original surface"), Mesh->GetMaterial(0) == Original);
	TestTrue(TEXT("Selection never adds a coplanar overlay"), Mesh->GetOverlayMaterial() == PreviousOverlay);
	Selection->HideHologram(Actor, Info);
	TestTrue(TEXT("Deselecting restores the pre-existing overlay"), Mesh->GetOverlayMaterial() == PreviousOverlay);
	TestTrue(TEXT("Deselecting preserves the original surface"), Mesh->GetMaterial(0) == Original);
	TestFalse(TEXT("Outline reference cleared"), Info.Outline.IsValid());
	World->DestroyWorld(false);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageWorldAlignmentTest, "HyperManage.Transform.WorldAlignment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageWorldAlignmentTest::RunTest(const FString& Parameters)
{
	auto* Calculator = NewObject<UHyperManageTransform>();
	FHyperManageTransformData Data;
	Data.WorldAlignment = true;
	Data.GroupMode = false;
	Data.SnapWorldPosition = true;
	const FTransform Original(FRotator(7, 43, -4), FVector(910, -730, 123), FVector(2, 1, 0.5));
	auto Result = Calculator->ComputeTransform(Original, Data);
	TestTrue(TEXT("XY rounds to world origin grid, preserving Z"), Result.GetLocation().Equals(FVector(800, -800, 123)));
	TestTrue(TEXT("Position snap preserves rotation"), Result.GetRotation().Equals(Original.GetRotation(), 0.0001));
	TestTrue(TEXT("Scale stays unchanged"), Result.GetScale3D().Equals(Original.GetScale3D()));
	TestTrue(TEXT("Snapping twice is stable"), Calculator->ComputeTransform(Result, Data).Equals(Result, 0.0001));
	Data.SnapWorldPosition = false;
	Data.SnapWorldRotation = true;
	Data.AlignmentAngle = 15;
	Result = Calculator->ComputeTransform(Original, Data);
	TestTrue(TEXT("World rotation rounds independently of view"), Result.GetRotation().Equals(FRotator(0, 45, 0).Quaternion(), 0.0001));
	TestTrue(TEXT("Individual rotation retains position"), Result.GetLocation().Equals(Original.GetLocation()));
	Data.SnapWorldRotation = false;
	Data.LevelWorldRotation = true;
	Result = Calculator->ComputeTransform(Original, Data);
	TestTrue(TEXT("Level retains yaw"), Result.GetRotation().Equals(FRotator(0, 43, 0).Quaternion(), 0.0001));
	Data.LevelWorldRotation = false;
	Data.GroupMode = true;
	Data.SnapWorldPosition = true;
	Data.SnapWorldRotation = true;
	Data.AlignmentAngle = 90;
	Data.PivotLoc = FVector(910, -730, 123);
	Data.AnchorQuat = FRotator(0, 80, 0).Quaternion();
	const FTransform Reference(Data.AnchorQuat, Data.PivotLoc);
	const FTransform Member(Data.AnchorQuat, Data.PivotLoc + FVector(100, 0, 30), FVector(2));
	const auto AlignedReference = Calculator->ComputeTransform(Reference, Data);
	const auto AlignedMember = Calculator->ComputeTransform(Member, Data);
	TestTrue(TEXT("Group anchor lands on grid"), AlignedReference.GetLocation().Equals(FVector(800, -800, 123)));
	TestTrue(TEXT("Group offset rotates rigidly"), (AlignedMember.GetLocation() - AlignedReference.GetLocation()).Equals(FRotator(0, 10, 0).RotateVector(FVector(100, 0, 30)), 0.001));
	TestTrue(TEXT("Group member keeps scale"), AlignedMember.GetScale3D().Equals(FVector(2)));
	Data.AlignmentGridCm = 0;
	TestTrue(TEXT("Invalid spacing cannot alter objects"), Calculator->ComputeTransform(Member, Data).Equals(Member));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageToolbarLayoutTest, "HyperManage.UI.NestedToolbar", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageToolbarLayoutTest::RunTest(const FString& Parameters)
{
	auto* Tools = NewObject<UHyperManageToolWidget>();
	Tools->WidgetTree = NewObject<UWidgetTree>(Tools);
	auto* Tree = Tools->WidgetTree.Get();
	auto* Root = Tree->ConstructWidget<UCanvasPanel>();
	Tree->RootWidget = Root;
	auto* Window = Tree->ConstructWidget<UNamedSlot>(UNamedSlot::StaticClass(), TEXT("ToolbarWindow"));
	Root->AddChild(Window);
	auto* Content = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Content"));
	Tools->btnNewSelection = Tree->ConstructWidget<UButton>();
	Content->AddChild(Tools->btnNewSelection);
	Tools->btnCopySelection = Tree->ConstructWidget<UButton>();
	Content->AddChild(Tools->btnCopySelection);
	// Reproduce the real Blueprint: Content keeps its outer but is inserted into another widget's tree.
	auto* ForeignWindow = NewObject<UHyperManageToolWidget>(Tree);
	ForeignWindow->WidgetTree = NewObject<UWidgetTree>(ForeignWindow);
	auto* ForeignBody = ForeignWindow->WidgetTree->ConstructWidget<USizeBox>();
	ForeignWindow->WidgetTree->RootWidget = ForeignBody;
	ForeignBody->SetContent(Content);
	Window->AddChild(ForeignWindow);
	TestNull(TEXT("Old traversal cannot find reparented Content"), Tools->GetWidgetFromName(TEXT("Content")));
	Tools->RepairToolbarLayout();
	TestTrue(TEXT("Content is reachable after replacing the legacy window"), Tools->GetWidgetFromName(TEXT("Content")) == Content);
	TestNotNull(TEXT("World offset X field created"), Tools->OffsetX.Get());
	TestNotNull(TEXT("World offset Y field created"), Tools->OffsetY.Get());
	TestNotNull(TEXT("World offset Z field created"), Tools->OffsetZ.Get());
	TestFalse(TEXT("Offset apply starts disabled without a selection"), Tools->ApplyOffsetButton->GetIsEnabled());
	Tools->OffsetX->SetValue(0.375f); Tools->OffsetY->SetValue(-1.25f); Tools->OffsetZ->SetValue(2.f);
	Tools->ClearWorldOffset();
	TestEqual(TEXT("Zero fields resets X"), Tools->OffsetX->GetValue(), 0.f);
	TestEqual(TEXT("Zero fields resets Y"), Tools->OffsetY->GetValue(), 0.f);
	TestEqual(TEXT("Zero fields resets Z"), Tools->OffsetZ->GetValue(), 0.f);
	TestNotNull(TEXT("World position fields created"), Tools->PositionX.Get());
 TestEqual(TEXT("All position axes start enabled"), Tools->GetPositionAxisMask(), uint8(7));
 Tools->PositionAxes[0]->SetIsChecked(false); Tools->PositionAxes[1]->SetIsChecked(false);
 TestEqual(TEXT("Position switches can isolate height"), Tools->GetPositionAxisMask(), uint8(4));
 Tools->PositionAxes[0]->SetIsChecked(true); Tools->PositionAxes[1]->SetIsChecked(true);
	TestFalse(TEXT("Position apply requires a selection"), Tools->ApplyPositionButton->GetIsEnabled());
	TestFalse(TEXT("Read position requires a selection"), Tools->ReadPositionButton->GetIsEnabled());
	TestNotNull(TEXT("Absolute orientation input created"), Tools->OrientationYaw.Get());
 TestEqual(TEXT("All orientation axes start enabled"), Tools->GetOrientationAxisMask(), uint8(7));
 Tools->OrientationAxes[1]->SetIsChecked(false); Tools->OrientationAxes[2]->SetIsChecked(false);
 TestEqual(TEXT("Orientation switches can isolate heading"), Tools->GetOrientationAxisMask(), uint8(1));
 Tools->OrientationAxes[1]->SetIsChecked(true); Tools->OrientationAxes[2]->SetIsChecked(true);
	TestFalse(TEXT("Absolute orientation apply starts disabled"), Tools->ApplyOrientationButton->GetIsEnabled());
	TestNotNull(TEXT("Yaw input created"), Tools->OffsetYaw.Get());
	TestNotNull(TEXT("Pitch input created"), Tools->OffsetPitch.Get());
	TestNotNull(TEXT("Roll input created"), Tools->OffsetRoll.Get());
	TestFalse(TEXT("Rotation apply starts disabled"), Tools->ApplyRotationButton->GetIsEnabled());
	Tools->OffsetYaw->SetValue(90.f); Tools->OffsetPitch->SetValue(22.5f); Tools->OffsetRoll->SetValue(-15.f);
	Tools->ClearRotationOffset();
	TestEqual(TEXT("Clear yaw input"), Tools->OffsetYaw->GetValue(), 0.f);
	TestEqual(TEXT("Clear pitch input"), Tools->OffsetPitch->GetValue(), 0.f);
	TestEqual(TEXT("Clear roll input"), Tools->OffsetRoll->GetValue(), 0.f);
	TestNotNull(TEXT("Exact scale X created"), Tools->ScaleX.Get());
	TestFalse(TEXT("Scale apply starts disabled"), Tools->ApplyScaleButton->GetIsEnabled());
	auto ScaleCombo = Tools->ScalePreset->TakeWidget();
	Tools->ScalePreset->SetSelectedOption(TEXT("125"));
	TestEqual(TEXT("Uniform preset fills X"), Tools->ScaleX->GetValue(), 125.f);
	TestEqual(TEXT("Uniform preset fills Y"), Tools->ScaleY->GetValue(), 125.f);
	TestEqual(TEXT("Uniform preset fills Z"), Tools->ScaleZ->GetValue(), 125.f);
	Tools->ScaleY->SetValue(80.f);
	Tools->ScaleY->OnValueChanged.Broadcast(80.f); // Simulate typing in the headless widget.
	Tools->ScalePreset->SetSelectedOption(TEXT("125"));
	TestEqual(TEXT("Same preset can be reapplied after manual edits"), Tools->ScaleY->GetValue(), 125.f);
	Tools->ResetScaleFields();
	TestEqual(TEXT("Reset prepares original X size"), Tools->ScaleX->GetValue(), 100.f);
	TestEqual(TEXT("Reset prepares original Y size"), Tools->ScaleY->GetValue(), 100.f);
	TestEqual(TEXT("Reset prepares original Z size"), Tools->ScaleZ->GetValue(), 100.f);
	TestEqual(TEXT("Three anchor alignment controls created"), Tools->AnchorAlignmentButtons.Num(), 3);
	for (const auto& Button : Tools->AnchorAlignmentButtons) TestFalse(TEXT("Anchor alignment requires a reference and selection"), Button->GetIsEnabled());
	TestNotNull(TEXT("Recent history is present"), Tools->HistoryArea.Get());
	TestFalse(TEXT("History starts collapsed to preserve tray space"), Tools->HistoryArea->GetIsExpanded());
	TestNotNull(TEXT("Selection slot picker created"), Tools->SelectionSlotPicker.Get());
	TestEqual(TEXT("Ten slots are available"), Tools->SelectionSlotPicker->GetOptionCount(), 10);
	TestNotNull(TEXT("Undo control created"), Tools->UndoButton.Get());
	TestNotNull(TEXT("Redo control created"), Tools->RedoButton.Get());
	TestNotNull(TEXT("Movement presets created"), Tools->MovementPreset.Get());
	TestNotNull(TEXT("Rotation presets created"), Tools->RotationPreset.Get());
	TestNotNull(TEXT("Independent height-grid field created"), Tools->HeightGridValue.Get());
	TestNotNull(TEXT("Independent height-grid presets created"), Tools->HeightGridPreset.Get());
	TestNotNull(TEXT("Grid presets created"), Tools->GridPreset.Get());
	TestNotNull(TEXT("Exact movement field created"), Tools->MovementValue.Get());
	TestNotNull(TEXT("Exact rotation field created"), Tools->RotationValue.Get());
	TestNotNull(TEXT("Exact grid field created"), Tools->GridValue.Get());
	TestFalse(TEXT("Numeric fields cannot consume drag gestures as a slider"), Tools->MovementValue->GetEnableSlider());
	TestNotNull(TEXT("Toolbar body has an explicit size"), Cast<USizeBox>(Content->GetParent()));
	TArray<FString> Tips;
	TArray<FString> Labels;
	Tree->ForEachWidget([&](UWidget* Widget) {
		if (auto* Text = Cast<UTextBlock>(Widget)) Labels.Add(Text->GetText().ToString());
		if (auto* Button = Cast<UButton>(Widget)) Tips.Add(Button->GetToolTipText().ToString());
	});
	TestTrue(TEXT("Working actions get readable labels"), Labels.Contains(TEXT("Clear")));
	TestNull(TEXT("Unfinished copy action is removed from the visible layout"), Tools->btnCopySelection->GetParent());
	TestTrue(TEXT("Snap XY is in the visible hierarchy"), Tips.ContainsByPredicate([](const FString& Tip) { return Tip.StartsWith(TEXT("Snap XY\n")); }));
	TestTrue(TEXT("Snap rotation is in the visible hierarchy"), Tips.ContainsByPredicate([](const FString& Tip) { return Tip.StartsWith(TEXT("Snap angle\n")); }));
	TestTrue(TEXT("Snap Z is visible"), Tips.ContainsByPredicate([](const FString& Tip) { return Tip.StartsWith(TEXT("Snap Z\n")); }));
	TestTrue(TEXT("Icon-only level has an identifying tooltip"), Tips.ContainsByPredicate([](const FString& Tip) { return Tip.StartsWith(TEXT("Level\n")); }));
	TestTrue(TEXT("Version label identifies the repaired menu"), Labels.Contains(TEXT("HyperManage | dev.45")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageClipboardLayoutTest, "HyperManage.UI.OriginalClipboard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageClipboardLayoutTest::RunTest(const FString& Parameters)
{
	auto* Clipboard = NewObject<UHyperManageClipboardWidget>();
	auto Slate = Clipboard->RebuildWidget();
	 auto* Config = NewObject<UHyperManageConfiguration>();
	 Config->MMConfig.IncrementSettings.SetNum(4);
	 Config->MMConfig.IncrementSize = EIncrementSize::Medium;
	 Config->MMConfig.IsGrouped = false; Config->MMConfig.IsViewBased = false;
	 Config->MMKeyConfigs.ActionKeys = {
		 FHyperManageKeyConfig(ShowTools, EKeys::RightMouseButton, false, false, false, false),
		 FHyperManageKeyConfig(EActionNameIdx::SetAnchor, EKeys::LeftMouseButton, false, false, true, false),
		 FHyperManageKeyConfig(EActionNameIdx::SetTarget, EKeys::RightMouseButton, false, false, true, false),
		 FHyperManageKeyConfig(SelectTarget, EKeys::LeftMouseButton, true, false, false, false),
		 FHyperManageKeyConfig(DeselectTarget, EKeys::RightMouseButton, true, false, false, false),
		 FHyperManageKeyConfig(Undo, EKeys::Z, true, false, false, false),
		 FHyperManageKeyConfig(Redo, EKeys::Y, true, false, false, false),
		 FHyperManageKeyConfig(Shrink, EKeys::J, true, true, false, false),
		 FHyperManageKeyConfig(Grow, EKeys::L, true, true, false, false),
		 FHyperManageKeyConfig(ChangeIncSize, EKeys::I, true, true, false, false),
		 FHyperManageKeyConfig(KnowNotes, EKeys::K, true, true, false, false)
	 };
	 Clipboard->UpdateReference(*Config, 12345);
	 TestEqual(TEXT("Redo shortcut is visible"), Clipboard->RedoText->GetText().ToString(), FString(TEXT("Ctrl+Y  Redo")));
	 TestTrue(TEXT("Selection count has its own box"), Clipboard->CountText->GetText().ToString().Contains(TEXT("12345")));
	 TestTrue(TEXT("Shrink and grow are documented"), Clipboard->ScaleText->GetText().ToString().Contains(TEXT("Ctrl+Alt+L  Grow")));
	 TestTrue(TEXT("Increment shortcut and current profile are documented"), Clipboard->StatusText->GetText().ToString().Contains(TEXT("Ctrl+Alt+I  Increment: Medium")));
	 TestTrue(TEXT("Notes shortcut is documented"), Clipboard->NotesText->GetText().ToString().Contains(TEXT("Ctrl+Alt+K")));
	Slate->SlatePrepass(1.f);
	TestTrue(TEXT("Packaged handwriting font exists"), FPaths::FileExists(Clipboard->HandwrittenFont->CompositeFont.DefaultTypeface.Fonts[0].Font.GetFontFilename()));
	int32 Images = 0;
	Clipboard->WidgetTree->ForEachWidget([&](UWidget* Widget) {
		if (auto* Image = Cast<UImage>(Widget)) {
			++Images;
			const auto* Texture = Cast<UTexture2D>(Image->GetBrush().GetResourceObject());
			TestNotNull(TEXT("Original artwork loads"), Texture);
			if (Texture) TestTrue(TEXT("Uses new artwork rather than legacy clipboard"), Texture->GetPathName().Contains(TEXT("/UI/Jusedawg/T_Clipboard")));
		}
		if (auto* Text = Cast<UTextBlock>(Widget)) {
			const auto* TextSlot = Cast<UCanvasPanelSlot>(Text->Slot);
			if (TextSlot) TestTrue(*FString::Printf(TEXT("Text fits height (%g <= %g): %s"), Text->GetDesiredSize().Y, TextSlot->GetSize().Y, *Text->GetText().ToString()), Text->GetDesiredSize().Y <= TextSlot->GetSize().Y + 1.f);
		}
	});
	TestEqual(TEXT("One original clipboard background"), Images, 1);
	TestEqual(TEXT("Clipboard cannot block gameplay input"), Clipboard->GetVisibility(), ESlateVisibility::HitTestInvisible);
	Config->MMKeyConfigs.ActionKeys[6] = FHyperManageKeyConfig(Redo, EKeys::R, true, false, true, false);
	Clipboard->UpdateReference(*Config, 0);
	TestEqual(TEXT("Redo label follows remapping"), Clipboard->RedoText->GetText().ToString(), FString(TEXT("Ctrl+Shift+R  Redo")));
	Config->MMKeyConfigs.ActionKeys.RemoveAt(6);
	Clipboard->UpdateReference(*Config, 0);
	TestEqual(TEXT("Missing binding is honest"), Clipboard->RedoText->GetText().ToString(), FString(TEXT("Unbound  Redo")));
	Clipboard->ReleaseSlateResources(true);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManagePrecisionSettingsTest, "HyperManage.Settings.PrecisionValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManagePrecisionSettingsTest::RunTest(const FString& Parameters)
{
	auto* Config = NewObject<UHyperManageConfiguration>();
	Config->MMConfig.IncrementSettings = {
		FHyperManageIncrement(EIncrementSize::Tiny, 1.f, 1.f, 1.f), FHyperManageIncrement(EIncrementSize::Medium, 10.f, 5.f, 5.f),
		FHyperManageIncrement(EIncrementSize::Large, 25.f, 10.f, 10.f), FHyperManageIncrement(EIncrementSize::Huge, 100.f, 45.f, 20.f)
	};
	Config->MMConfig.IncrementSize = EIncrementSize::Medium;
	Config->MMConfig.CurrentIncrementSize = TEXT("Medium");
	TestTrue(TEXT("Accept custom movement in meters"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Movement, 0.375f));
	TestTrue(TEXT("Accept fractional degrees"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Rotation, 22.5f));
	TestTrue(TEXT("Accept independent height grid"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::HeightGrid, 2.5f));
	TestTrue(TEXT("Accept custom world grid in meters"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Grid, 1.25f));
	TestEqual(TEXT("Movement converts to centimeters"), Config->MMConfig.IncrementSettings[1].CentimetersToMove, 37.5f);
	TestEqual(TEXT("Other profiles remain unchanged"), Config->MMConfig.IncrementSettings[0].CentimetersToMove, 1.f);
	TestEqual(TEXT("Height grid converts independently"), Config->MMConfig.HeightGridCm, 250.f);
	TestEqual(TEXT("Grid converts to centimeters"), Config->MMConfig.AlignmentGridCm, 125.f);
	TestFalse(TEXT("Unchanged value does not request another write"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Movement, 0.375f));
	for (float Invalid : {-1.f, 0.f, 1001.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
		TestFalse(TEXT("Reject invalid movement"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Movement, Invalid));
		TestFalse(TEXT("Reject invalid height grid"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::HeightGrid, Invalid));
		TestFalse(TEXT("Reject invalid grid"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Grid, Invalid));
		TestFalse(TEXT("Reject invalid rotation"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Rotation, Invalid));
	}
	TestFalse(TEXT("Reject subminimum rotation"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Rotation, 0.01f));
	TestFalse(TEXT("Reject rotation above the supported maximum"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Rotation, 181.f));
	TestEqual(TEXT("Rejected values preserve previous movement"), Config->MMConfig.IncrementSettings[1].CentimetersToMove, 37.5f);
	FString Json;
	TestTrue(TEXT("Custom settings serialize"), FJsonObjectConverter::UStructToJsonObjectString(Config->MMConfig, Json));
	FHyperManageConfig Restored;
	TestTrue(TEXT("Custom settings deserialize"), FJsonObjectConverter::JsonObjectStringToUStruct(Json, &Restored));
	if (Restored.IncrementSettings.Num() != 4) { AddError(TEXT("Saved profiles did not round-trip")); return false; }
	TestEqual(TEXT("Saved custom movement survives round-trip"), Restored.IncrementSettings[1].CentimetersToMove, 37.5f);
	TestEqual(TEXT("Saved custom rotation survives round-trip"), Restored.IncrementSettings[1].DegreesToRotate, 22.5f);
	TestEqual(TEXT("Saved height grid survives round-trip"), Restored.HeightGridCm, 250.f);
	TestEqual(TEXT("Saved custom grid survives round-trip"), Restored.AlignmentGridCm, 125.f);
	Config->MMConfig.IncrementSettings.Empty();
	TestFalse(TEXT("Missing active profile cannot cause an out-of-bounds write"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Movement, 1.f));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageHistoryTest, "HyperManage.History.UndoRedo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageHistoryTest::RunTest(const FString& Parameters)
{
	auto* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Actor = World->SpawnActor<AStaticMeshActor>();
	Actor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
	auto* History = NewObject<UHyperManageUndo>();
	TArray<AActor*> Actors = {Actor, nullptr};
	const FTransform Before(FRotator(0, 15, 0), FVector(100, 200, 300));
	const FTransform After(FRotator(0, 45, 0), FVector(400, 500, 600));
	Actor->SetActorTransform(Before);
	History->PushNamedTransforms(Actors, TEXT("World offset"));
	TestEqual(TEXT("Undo preview names the next edit"), History->GetRecentDescriptions(false)[0], FString(TEXT("World offset")));
	Actor->SetActorTransform(After);
	FUndoInfo Info;
	for (int32 Index = 0; Index < 3; ++Index) {
		TestTrue(TEXT("Undo is available"), History->PopUndo(Info));
		if (Info.TransformActors.Num() != 1) { AddError(TEXT("Expected one valid transform record")); break; }
		TestTrue(TEXT("Undo restores the original transform"), Info.TransformActors[0].Transform.Equals(Before));
		Actor->SetActorTransform(Info.TransformActors[0].Transform);
		TestEqual(TEXT("Description survives transfer to redo"), History->GetRecentDescriptions(true)[0], FString(TEXT("World offset")));
		TestTrue(TEXT("Redo is available"), History->PopRedo(Info));
		if (Info.TransformActors.Num() != 1) { AddError(TEXT("Expected one redo transform")); break; }
		TestTrue(TEXT("Redo restores the edited transform"), Info.TransformActors[0].Transform.Equals(After));
		Actor->SetActorTransform(Info.TransformActors[0].Transform);
	}
	History->PopUndo(Info);
	TArray<AActor*> Empty;
	History->PushUndoTransforms(Empty);
	TestEqual(TEXT("Empty edit preserves redo"), History->GetRedoCount(), 1);
	History->PushUndoTransforms(Actors);
	TestEqual(TEXT("New edit discards redo"), History->GetRedoCount(), 0);
 History->PushNamedTransforms(Actors, TEXT("Rotate"));
 History->PushNamedTransforms(Actors, TEXT("Snap Z"));
 const auto Preview = History->GetRecentDescriptions(false, 2);
 TestEqual(TEXT("History preview respects its limit"), Preview.Num(), 2);
 TestEqual(TEXT("Newest operation appears first"), Preview[0], FString(TEXT("Snap Z")));
 TestEqual(TEXT("Previous operation follows"), Preview[1], FString(TEXT("Rotate")));
 TestTrue(TEXT("Negative preview limit returns no entries"), History->GetRecentDescriptions(false, -1).IsEmpty());
 TestTrue(TEXT("Branch removes redo preview"), History->GetRecentDescriptions(true).IsEmpty());
	for (int32 Index = 0; Index < 1005; ++Index) History->PushUndoTransforms(Actors);
	TestEqual(TEXT("History stays bounded"), History->GetUndoCount(), 1000);
	const uint64 PreviousRevision = History->GetRevision();
	History->ClearUndoStack();
	TestTrue(TEXT("Clear invalidates cached history display"), History->GetRevision() > PreviousRevision);
	TestTrue(TEXT("Clear removes preview entries"), History->GetRecentDescriptions(false).IsEmpty());
	TestFalse(TEXT("Clear removes undo"), History->CanUndo());
	TestFalse(TEXT("Clear removes redo"), History->CanRedo());
	History->PushUndoTransforms(Actors);
	Actor->Destroy();
	TestFalse(TEXT("Destroyed objects cannot be replayed"), History->PopUndo(Info));

	auto* System = NewObject<UHyperManageSystem>();
	System->Selection = InitComponent<UHyperManageSelection>(System);
	auto* SelectionHistory = InitComponent<UHyperManageUndo>(System);
	auto* Proxy = World->SpawnActor<AHyperManageLightweightProxy>();
	Actors = {Proxy}; SelectionHistory->PushUndoSelection(Actors);
	Proxy->BeginRequest();
	TestFalse(TEXT("Pending selection proxy disables undo"), SelectionHistory->CanUndo());
	TestFalse(TEXT("Pending record cannot be popped"), SelectionHistory->PopUndo(Info));
	TestEqual(TEXT("Pending record stays in history"), SelectionHistory->GetUndoCount(), 1);
	Proxy->ApplyAcknowledgement(FHyperManageLightweightRef(), FFactoryCustomizationData());
	TestTrue(TEXT("Acknowledged record can be undone"), SelectionHistory->PopUndo(Info));
	Proxy->BeginRequest();
	TestFalse(TEXT("Redo also waits for acknowledgement"), SelectionHistory->PopRedo(Info));
	Proxy->ApplyAcknowledgement(FHyperManageLightweightRef(), FFactoryCustomizationData());
	TestTrue(TEXT("Acknowledged record can be redone"), SelectionHistory->PopRedo(Info));
	World->DestroyWorld(false);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageWorldOffsetTest, "HyperManage.Transform.WorldOffset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageWorldOffsetTest::RunTest(const FString& Parameters)
{
	auto* Transform = NewObject<UHyperManageTransform>();
	FHyperManageTransformData Data;
	Data.IsRot = true; Data.IsScale = true; Data.WorldAlignment = true; Data.ViewRelative = true;
	TestTrue(TEXT("Accept mixed-axis fractional meter offsets"), UHyperManageTransform::MakeWorldOffset(FVector(0.375, -1.25, 2), Data));
	const FTransform First(FRotator(20, 90, -15), FVector(-1200, 50, -300), FVector(2, 0.5, 1.5));
	const FTransform Second(FRotator(-30, -45, 10), FVector(900, -700, 100), FVector(0.75, 1, 2));
	const FTransform MovedFirst = Transform->ComputeTransform(First, Data);
	const FTransform MovedSecond = Transform->ComputeTransform(Second, Data);
	const FVector ExpectedOffset(37.5, -125, 200);
	TestTrue(TEXT("Meters convert to world centimeters"), MovedFirst.GetLocation().Equals(First.GetLocation() + ExpectedOffset));
	TestTrue(TEXT("Offset ignores each object's orientation"), MovedSecond.GetLocation().Equals(Second.GetLocation() + ExpectedOffset));
	TestTrue(TEXT("Spacing is preserved"), (MovedSecond.GetLocation() - MovedFirst.GetLocation()).Equals(Second.GetLocation() - First.GetLocation()));
	TestTrue(TEXT("Rotation is preserved"), MovedFirst.GetRotation().Equals(First.GetRotation()));
	TestTrue(TEXT("Nonuniform scale is preserved"), MovedFirst.GetScale3D().Equals(First.GetScale3D()));
	TestTrue(TEXT("Negative offset reverses the move"), UHyperManageTransform::MakeWorldOffset(FVector(-0.375, 1.25, -2), Data));
	TestTrue(TEXT("Inverse offset restores transform"), Transform->ComputeTransform(MovedFirst, Data).Equals(First));
	TestTrue(TEXT("Height-only offset is supported"), UHyperManageTransform::MakeWorldOffset(FVector(0, 0, -0.01), Data));
	TestTrue(TEXT("Height-only preserves horizontal coordinates"), Transform->ComputeTransform(First, Data).GetLocation().Equals(First.GetLocation() + FVector(0, 0, -1)));
	TestTrue(TEXT("Boundary values are accepted"), UHyperManageTransform::MakeWorldOffset(FVector(-1000, 1000, 1000), Data));
	TestFalse(TEXT("Zero offset is not an edit"), UHyperManageTransform::MakeWorldOffset(FVector::ZeroVector, Data));
	TestFalse(TEXT("Oversized offset is rejected"), UHyperManageTransform::MakeWorldOffset(FVector(1000.01, 0, 0), Data));
	TestFalse(TEXT("NaN offset is rejected"), UHyperManageTransform::MakeWorldOffset(FVector(std::numeric_limits<double>::quiet_NaN(), 0, 0), Data));
	TestFalse(TEXT("Infinite offset is rejected"), UHyperManageTransform::MakeWorldOffset(FVector(0, std::numeric_limits<double>::infinity(), 0), Data));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageWorldRotationOffsetTest, "HyperManage.Transform.WorldRotationOffset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageWorldRotationOffsetTest::RunTest(const FString& Parameters)
{
	auto* Transform = NewObject<UHyperManageTransform>();
	const FVector Pivot(-100, 200, 300);
	const FTransform First(FRotator(20, 40, -15), Pivot + FVector(100, 0, 0), FVector(2, 0.5, 1.5));
	const FTransform Second(FRotator(-30, 70, 25), Pivot + FVector(-100, 0, 0), FVector(1, 1, 1));
	FHyperManageTransformData Data;
	TestTrue(TEXT("Accept grouped yaw"), UHyperManageTransform::MakeWorldRotationOffset(FRotator(0, 90, 0), true, Pivot, Data));
	const FTransform RotatedFirst = Transform->ComputeTransform(First, Data);
	const FTransform RotatedSecond = Transform->ComputeTransform(Second, Data);
	TestTrue(TEXT("Group yaw orbits around chosen pivot"), RotatedFirst.GetLocation().Equals(Pivot + FVector(0, 100, 0), 0.001));
	TestTrue(TEXT("Other object follows same group rotation"), RotatedSecond.GetLocation().Equals(Pivot + FVector(0, -100, 0), 0.001));
	TestTrue(TEXT("Group preserves separation"), FMath::IsNearlyEqual(FVector::Dist(RotatedFirst.GetLocation(), RotatedSecond.GetLocation()), 200.0, 0.001));
	TestTrue(TEXT("Rotation preserves nonuniform scale"), RotatedFirst.GetScale3D().Equals(First.GetScale3D()));
	const FQuat RelativeBefore = First.GetRotation().Inverse() * Second.GetRotation();
	TestTrue(TEXT("Group preserves relative orientation"), (RotatedFirst.GetRotation().Inverse() * RotatedSecond.GetRotation()).Equals(RelativeBefore));
	const FTransform Anchor(FRotator::ZeroRotator, Pivot);
	TestTrue(TEXT("Anchor position stays fixed"), Transform->ComputeTransform(Anchor, Data).GetLocation().Equals(Pivot));
	const FRotator Compound(22.5, -45, 15);
	TestTrue(TEXT("Accept compound individual rotation"), UHyperManageTransform::MakeWorldRotationOffset(Compound, false, Pivot, Data));
	const FTransform Individual = Transform->ComputeTransform(First, Data);
	TestTrue(TEXT("Individual keeps its location"), Individual.GetLocation().Equals(First.GetLocation()));
	TestTrue(TEXT("World rotation is composed before object orientation"), Individual.GetRotation().Equals(Compound.Quaternion() * First.GetRotation()));
	TestTrue(TEXT("Compound inverse is accepted"), UHyperManageTransform::MakeWorldRotationOffset(Compound.Quaternion().Inverse().Rotator(), false, Pivot, Data));
	TestTrue(TEXT("Compound inverse restores original transform"), Transform->ComputeTransform(Individual, Data).Equals(First));
	TestTrue(TEXT("Negative half turn accepted"), UHyperManageTransform::IsValidRotationOffset(FRotator(0, -180, 0)));
	TestFalse(TEXT("Zero is not a rotation edit"), UHyperManageTransform::IsValidRotationOffset(FRotator::ZeroRotator));
	TestFalse(TEXT("Equivalent identity is not an edit"), UHyperManageTransform::IsValidRotationOffset(FRotator(180, 180, 180)));
	TestFalse(TEXT("Reject out of bounds rotation"), UHyperManageTransform::IsValidRotationOffset(FRotator(0, 181, 0)));
	TestFalse(TEXT("Reject nonfinite rotation"), UHyperManageTransform::IsValidRotationOffset(FRotator(std::numeric_limits<double>::infinity(), 0, 0)));
	TestFalse(TEXT("Reject invalid pivot"), UHyperManageTransform::MakeWorldRotationOffset(Compound, true, FVector(std::numeric_limits<double>::quiet_NaN(), 0, 0), Data));
	Data.WorldRotationOffset = true; Data.WorldAlignment = true; Data.Rot = FRotator(0, 181, 0);
	TestTrue(TEXT("Invalid replay leaves transform unchanged"), Transform->ComputeTransform(First, Data).Equals(First));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageExactScaleTest, "HyperManage.Transform.ExactScale", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageExactScaleTest::RunTest(const FString& Parameters)
{
	const FTransform Original(FRotator(15, 70, -25), FVector(-100, 200, 500), FVector(2, 0.75, 3));
	const FVector Percent(125, 50, 200);
	TestTrue(TEXT("Accept independent axis percentages"), UHyperManageTransform::IsValidScalePercent(Percent));
	FTransform Result;
	TestTrue(TEXT("Absolute scale is accepted"), UHyperManageTransform::MakeAbsoluteScale(Original, Percent / 100.0, Result));
	TestTrue(TEXT("Absolute scale replaces rather than multiplies"), Result.GetScale3D().Equals(FVector(1.25, 0.5, 2)));
	TestTrue(TEXT("Scaling preserves world origin"), Result.GetLocation().Equals(Original.GetLocation()));
	TestTrue(TEXT("Scaling preserves rotation"), Result.GetRotation().Equals(Original.GetRotation()));
	FTransform Repeated;
	TestTrue(TEXT("Repeated apply accepted"), UHyperManageTransform::MakeAbsoluteScale(Result, Percent / 100.0, Repeated));
	TestTrue(TEXT("Repeated apply does not compound"), Repeated.Equals(Result));
	TestTrue(TEXT("Reset original size accepted"), UHyperManageTransform::MakeAbsoluteScale(Result, FVector::OneVector, Repeated));
	TestTrue(TEXT("Reset yields unit scale"), Repeated.GetScale3D().Equals(FVector::OneVector));
	for (const FVector Invalid : {FVector(0, 100, 100), FVector(-50, 100, 100), FVector(100, 1001, 100), FVector(100, 100, 0.5),
		FVector(std::numeric_limits<double>::quiet_NaN(), 100, 100), FVector(100, std::numeric_limits<double>::infinity(), 100)}) {
		TestFalse(TEXT("Reject invalid UI percentages"), UHyperManageTransform::IsValidScalePercent(Invalid));
	}
	TestTrue(TEXT("UI lower bound accepted"), UHyperManageTransform::IsValidScalePercent(FVector(1)));
	TestTrue(TEXT("UI upper bound accepted"), UHyperManageTransform::IsValidScalePercent(FVector(1000)));
	const FTransform Sentinel = Repeated;
	TestFalse(TEXT("Reject degenerate transport scale"), UHyperManageTransform::MakeAbsoluteScale(Original, FVector(0, 1, 1), Repeated));
	TestTrue(TEXT("Rejected scale leaves output untouched"), Repeated.Equals(Sentinel));
	TestTrue(TEXT("Existing mirrored match-scale remains supported"), UHyperManageTransform::MakeAbsoluteScale(Original, FVector(-1, 1, 1), Repeated));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageSelectionHistoryTest, "HyperManage.History.SelectionChanges", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageSelectionHistoryTest::RunTest(const FString& Parameters)
{
	auto* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* System = NewObject<UHyperManageSystem>(); System->CurrentWorld = World;
	System->Selection = InitComponent<UHyperManageSelection>(System);
	System->Undo = InitComponent<UHyperManageUndo>(System);
	auto* Selection = System->Selection;
	auto* History = System->Undo;
	auto* A = World->SpawnActor<AFGTargetPoint>();
	auto* B = World->SpawnActor<AFGTargetPoint>();
	auto* C = World->SpawnActor<AFGTargetPoint>();
	if (!A || !B || !C) { AddError(TEXT("Selection test actors failed to spawn")); World->DestroyWorld(false); return false; }
	auto Replay = [&](bool Redo) {
		FUndoInfo Info;
		const bool Found = Redo ? History->PopRedo(Info) : History->PopUndo(Info);
		TestTrue(Redo ? TEXT("Redo selection frame exists") : TEXT("Undo selection frame exists"), Found);
		if (Found) Selection->RestoreHistory(Info);
	};
	TestTrue(TEXT("Select first actor"), Selection->SelectActorWithHistory(A, true));
	TestFalse(TEXT("Duplicate selection is a no-op"), Selection->SelectActorWithHistory(A, true));
	TestEqual(TEXT("No-op selection does not add history"), History->GetUndoCount(), 1);
	Replay(false); TestFalse(TEXT("Undo click deselects actor"), Selection->Contains(A));
	Replay(true); TestTrue(TEXT("Redo click reselects actor"), Selection->Contains(A));
	Selection->SelectActorWithHistory(B, true);
	Selection->SetMarkerWithHistory(A, true);
	Selection->SetMarkerWithHistory(B, false);
	TestEqual(TEXT("Editable selection count already excludes the target"), Selection->SelectCount(), 1);
	const int32 BeforeClear = History->GetUndoCount();
	Selection->SelectClear(false);
	TestEqual(TEXT("Cancelled clear creates no history"), History->GetUndoCount(), BeforeClear);
	Selection->SelectClear();
	TestEqual(TEXT("Clear is one history step"), History->GetUndoCount(), BeforeClear + 1);
	TestEqual(TEXT("Clear empties editable selection"), Selection->SelectCount(), 0);
	Replay(false);
	TestTrue(TEXT("Undo clear restores both objects"), Selection->Contains(A) && Selection->Contains(B));
	TestTrue(TEXT("Undo clear restores anchor and target"), Selection->AnchorActor == A && Selection->TargetActor == B);
	Replay(true);
	TestFalse(TEXT("Redo clear removes objects"), Selection->Contains(A) || Selection->Contains(B));
	Replay(false);
	Selection->SelectActorWithHistory(A, false);
	TestEqual(TEXT("New selection edit invalidates redo"), History->GetRedoCount(), 0);
	TestNull(TEXT("Deselecting anchor clears marker"), Selection->AnchorActor);
	Replay(false);
	TestTrue(TEXT("Undo deselection restores anchor"), Selection->Contains(A) && Selection->AnchorActor == A);
	Selection->SetMarkerWithHistory(C, true);
	TestTrue(TEXT("New anchor is selected automatically"), Selection->Contains(C));
	Replay(false);
	TestTrue(TEXT("Undo anchor replacement restores previous marker and membership"), Selection->AnchorActor == A && !Selection->Contains(C));
	Selection->SaveSelection();
	Selection->ClearWithoutHistory();
	Selection->SelectActor(C); Selection->SetAnchor(C);
	History->ClearUndoStack();
	Selection->LoadSelection();
	TestEqual(TEXT("Recall creates exactly one history step"), History->GetUndoCount(), 1);
	TestTrue(TEXT("Recall restores saved objects and markers"), Selection->Contains(A) && Selection->Contains(B) && !Selection->Contains(C) && Selection->AnchorActor == A && Selection->TargetActor == B);
	Replay(false);
	TestTrue(TEXT("Undo recall restores replaced selection"), Selection->Contains(C) && !Selection->Contains(A) && !Selection->Contains(B) && Selection->AnchorActor == C && !Selection->TargetActor);
	Replay(true);
	Selection->LoadSelection();
	TestEqual(TEXT("Recalling identical selection adds no history"), History->GetUndoCount(), 1);
	System->Transform = InitComponent<UHyperManageTransform>(System);
	auto* Root = NewObject<USceneComponent>(C); C->SetRootComponent(Root); C->AddInstanceComponent(Root); Root->RegisterComponent();
	auto* DetachedPart = NewObject<USceneComponent>(C); C->AddInstanceComponent(DetachedPart); DetachedPart->RegisterComponent();
	Root->SetWorldLocation(FVector(100, 200, 300));
	DetachedPart->SetWorldLocation(FVector(-100, 400, 800));
	FHyperManageTransformData Alignment;
	UHyperManageTransform::MakeWorldOriginAlignment(FVector(0, 0, 500), EAxis::Z, Alignment);
	System->Transform->ProcessTransform({C}, Alignment);
	TestTrue(TEXT("Native actor origin aligns to reference height"), C->GetActorLocation().Equals(FVector(100, 200, 500)));
	TestTrue(TEXT("Detached actor component preserves its relative offset"), DetachedPart->GetComponentLocation().Equals(FVector(-100, 400, 1000)));
	Root->SetWorldLocation(FVector(100, 200, 537));
	DetachedPart->SetWorldLocation(FVector(-100, 400, 1037));
	Alignment.WorldOriginAlignment = false; Alignment.SnapWorldHeight = true; Alignment.AlignmentGridCm = 100;
	System->Transform->ProcessTransform({C}, Alignment);
	TestTrue(TEXT("Height grid snaps native root"), C->GetActorLocation().Equals(FVector(100, 200, 500)));
	TestTrue(TEXT("Height grid preserves detached component offsets"), DetachedPart->GetComponentLocation().Equals(FVector(-100, 400, 1000)));
	B->Destroy();
	Replay(false); Replay(true);
	TestNull(TEXT("Destroyed target is not restored"), Selection->TargetActor);
	Selection->ClearWithoutHistory();
	History->ClearUndoStack();
	Selection->SelectClear();
	TestEqual(TEXT("Clearing empty selection adds no history"), History->GetUndoCount(), 0);
 TestTrue(TEXT("Default slot remains occupied after clearing current selection"), Selection->HasSavedSelection());
 TestEqual(TEXT("Saved count excludes destroyed target"), Selection->GetSavedSelectionCount(), 1);
 TestTrue(TEXT("Select tenth slot"), Selection->SetSelectionSlot(9));
 TestFalse(TEXT("New slot is empty"), Selection->HasSavedSelection());
 Selection->SelectActor(C);
 Selection->LoadSelection();
 TestTrue(TEXT("Recalling unused slot leaves current selection alone"), Selection->Contains(C));
 TestEqual(TEXT("Unused recall creates no history"), History->GetUndoCount(), 0);
 Selection->SetAnchor(C); Selection->SaveSelection();
 TestEqual(TEXT("Tenth slot holds its own selection"), Selection->GetSavedSelectionCount(), 1);
 Selection->SetSelectionSlot(0); Selection->LoadSelection();
 TestTrue(TEXT("First slot survives saving another slot"), Selection->Contains(A) && !Selection->Contains(C) && Selection->AnchorActor == A);
 Selection->SetSelectionSlot(9);
 TestTrue(TEXT("Changing slot alone leaves live selection unchanged"), Selection->Contains(A) && !Selection->Contains(C));
 Selection->LoadSelection();
 TestTrue(TEXT("Tenth slot restores its anchor"), Selection->Contains(C) && !Selection->Contains(A) && Selection->AnchorActor == C);
 TestFalse(TEXT("Reject slot below range"), Selection->SetSelectionSlot(-1));
 TestFalse(TEXT("Reject slot above range"), Selection->SetSelectionSlot(10));
 TestEqual(TEXT("Rejected slot preserves active slot"), Selection->GetSelectionSlot(), 9);
 Selection->ClearWithoutHistory(); Selection->SaveSelection();
 TestTrue(TEXT("An intentionally saved empty selection is occupied"), Selection->HasSavedSelection());
 Selection->SelectActor(A); Selection->LoadSelection();
 TestEqual(TEXT("Saved empty selection can be recalled"), Selection->SelectCount(), 0);
 System->Config = InitComponent<UHyperManageConfiguration>(System);
 System->Config->MMConfig.SelectionTolerance = 1.f;
 Selection->ClearWithoutHistory();
 auto MakeBoxActor = [&](const FVector& Location) {
  auto* Actor = World->SpawnActor<AFGTargetPoint>();
  auto* Box = NewObject<UBoxComponent>(Actor); Box->SetBoxExtent(FVector(50));
  Actor->SetRootComponent(Box); Actor->AddInstanceComponent(Box); Box->RegisterComponent();
  Actor->SetActorLocation(Location); Selection->SelectActor(Actor); return Actor;
 };
 auto* BoxAnchor = MakeBoxActor(FVector(-100, -100, -100));
 auto* BoxTarget = MakeBoxActor(FVector(100, 100, 100));
 auto* Inside = MakeBoxActor(FVector::ZeroVector);
 auto* EdgeOnly = MakeBoxActor(FVector(125, 0, 0));
 auto* Outside = MakeBoxActor(FVector(500, 500, 500));
 Selection->SetAnchor(BoxAnchor); Selection->SetTarget(BoxTarget);
 History->ClearUndoStack();
 Selection->ChangeAnchorTargetBoxSelection(false, true);
 TestFalse(TEXT("Center subtraction removes interior selection"), Selection->Contains(Inside));
 TestTrue(TEXT("Center subtraction keeps objects beyond reference centers"), Selection->Contains(EdgeOnly));
 TestTrue(TEXT("Box subtraction keeps references and outside objects"), Selection->Contains(BoxAnchor) && Selection->Contains(BoxTarget) && Selection->Contains(Outside));
 TestTrue(TEXT("Box subtraction preserves markers"), Selection->AnchorActor == BoxAnchor && Selection->TargetActor == BoxTarget);
 TestEqual(TEXT("Box subtraction is one undo step"), History->GetUndoCount(), 1);
 Selection->ChangeAnchorTargetBoxSelection(false, true);
 TestEqual(TEXT("No-op subtraction creates no history"), History->GetUndoCount(), 1);
 Replay(false); TestTrue(TEXT("Undo restores subtracted selection"), Selection->Contains(Inside));
 Replay(true); TestFalse(TEXT("Redo repeats subtraction"), Selection->Contains(Inside));
 Selection->ChangeAnchorTargetBoxSelection(true, true);
 TestFalse(TEXT("Edge subtraction includes reference extents"), Selection->Contains(EdgeOnly));
 TestTrue(TEXT("Deselecting never destroys objects"), IsValid(Inside) && IsValid(EdgeOnly));
	World->DestroyWorld(false);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageOriginAlignmentTest, "HyperManage.Transform.OriginAlignment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageOriginAlignmentTest::RunTest(const FString& Parameters)
{
	auto* Transform = NewObject<UHyperManageTransform>();
	const FTransform Original(FRotator(20, -70, 35), FVector(-100, 200, 300), FVector(2, 0.5, 1.5));
	const FVector Reference(-800, -400, 1200);
	for (EAxis::Type Axis : {EAxis::X, EAxis::Y, EAxis::Z}) {
		FHyperManageTransformData Data;
		TestTrue(TEXT("Valid alignment axis accepted"), UHyperManageTransform::MakeWorldOriginAlignment(Reference, Axis, Data));
		const FTransform Aligned = Transform->ComputeTransform(Original, Data);
		FVector Expected = Original.GetLocation();
		Expected[static_cast<int32>(Axis) - 1] = Reference[static_cast<int32>(Axis) - 1];
		TestTrue(TEXT("Only the requested world coordinate changes"), Aligned.GetLocation().Equals(Expected));
		TestTrue(TEXT("Alignment preserves rotation"), Aligned.GetRotation().Equals(Original.GetRotation()));
		TestTrue(TEXT("Alignment preserves scale"), Aligned.GetScale3D().Equals(Original.GetScale3D()));
		TestTrue(TEXT("Repeated alignment is idempotent"), Transform->ComputeTransform(Aligned, Data).Equals(Aligned));
		TestTrue(TEXT("Already aligned origin has no offset"), UHyperManageTransform::OriginAlignmentDelta(Expected, Reference, Axis).IsNearlyZero());
	}
	FHyperManageTransformData Invalid;
	TestFalse(TEXT("No axis is rejected"), UHyperManageTransform::MakeWorldOriginAlignment(Reference, EAxis::None, Invalid));
	TestFalse(TEXT("Nonfinite reference is rejected"), UHyperManageTransform::MakeWorldOriginAlignment(FVector(std::numeric_limits<double>::infinity(), 0, 0), EAxis::X, Invalid));
	TestTrue(TEXT("Invalid replay axis cannot move an object"), UHyperManageTransform::OriginAlignmentDelta(Original.GetLocation(), Reference, EAxis::None).IsZero());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageHeightGridTest, "HyperManage.Transform.HeightGrid", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageHeightGridTest::RunTest(const FString& Parameters)
{
	auto* Calculator = NewObject<UHyperManageTransform>();
	FHyperManageTransformData Data;
	Data.WorldAlignment = true; Data.SnapWorldHeight = true; Data.GroupMode = false; Data.AlignmentGridCm = 100;
	const FTransform Original(FRotator(17, -43, 9), FVector(-813, 274, -176), FVector(2, 0.5, 1.25));
	const FTransform Result = Calculator->ComputeTransform(Original, Data);
	TestTrue(TEXT("Height rounds correctly below world zero without changing XY"), Result.GetLocation().Equals(FVector(-813, 274, -200)));
	TestTrue(TEXT("Height snap preserves rotation"), Result.GetRotation().Equals(Original.GetRotation()));
	TestTrue(TEXT("Height snap preserves scale"), Result.GetScale3D().Equals(Original.GetScale3D()));
	TestTrue(TEXT("Repeated height snap is stable"), Calculator->ComputeTransform(Result, Data).Equals(Result));
	Data.GroupMode = true; Data.PivotLoc = Original.GetLocation(); Data.AnchorQuat = Original.GetRotation();
	FTransform Member = Original; Member.AddToTranslation(FVector(300, -100, 37));
	const FTransform GroupMember = Calculator->ComputeTransform(Member, Data);
	TestTrue(TEXT("Grouped snap preserves relative height and spacing"), (GroupMember.GetLocation() - Result.GetLocation()).Equals(FVector(300, -100, 37)));
	Data.GroupMode = false;
	TestTrue(TEXT("Individual snap uses each object's own height"), Calculator->ComputeTransform(Member, Data).GetLocation().Equals(FVector(-513, 174, -100)));
	Data.AlignmentGridCm = 25;
	TestTrue(TEXT("Fractional-meter grid is supported"), Calculator->ComputeTransform(Original, Data).GetLocation().Equals(FVector(-813, 274, -175)));
	Data.AlignmentGridCm = 0;
	TestTrue(TEXT("Invalid grid cannot move objects"), Calculator->ComputeTransform(Original, Data).Equals(Original));
	FHyperManageConfig Legacy;
	TestTrue(TEXT("Older settings without height grid still load"), FJsonObjectConverter::JsonObjectStringToUStruct(TEXT("{\"alignmentGridCm\":400}"), &Legacy));
	TestEqual(TEXT("Older settings get a one-meter height grid"), Legacy.HeightGridCm, 100.f);
	TestEqual(TEXT("Older horizontal grid stays intact"), Legacy.AlignmentGridCm, 400.f);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageWorldPositionTest, "HyperManage.Transform.WorldPosition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageWorldPositionTest::RunTest(const FString& Parameters)
{
 auto* Transform = NewObject<UHyperManageTransform>();
 FHyperManageTransformData Data;
 const FTransform First(FRotator(15, 45, -10), FVector(-25000, 30000, 12500), FVector(2, 1, 0.5));
 const FTransform Second(FRotator(0, 90, 0), FVector(-24200, 30800, 12500));
 const FVector Reference = (First.GetLocation() + Second.GetLocation()) / 2.0;
 const FVector Destination(-200.125, 250.375, 130.5);
 TestTrue(TEXT("Absolute coordinates produce a valid group offset"), UHyperManageTransform::MakeWorldPositionOffset(Destination, Reference, Data));
 const auto MovedFirst = Transform->ComputeTransform(First, Data);
 const auto MovedSecond = Transform->ComputeTransform(Second, Data);
 TestTrue(TEXT("Selection center arrives at destination"), ((MovedFirst.GetLocation() + MovedSecond.GetLocation()) / 2.0).Equals(Destination * 100.0));
 TestTrue(TEXT("Group spacing is unchanged"), (MovedSecond.GetLocation() - MovedFirst.GetLocation()).Equals(Second.GetLocation() - First.GetLocation()));
 TestTrue(TEXT("Orientation and nonuniform scale are unchanged"), MovedFirst.GetRotation().Equals(First.GetRotation()) && MovedFirst.GetScale3D().Equals(First.GetScale3D()));
 TestTrue(TEXT("Anchor origin can be the reference"), UHyperManageTransform::MakeWorldPositionOffset(Destination, First.GetLocation(), Data));
 TestTrue(TEXT("Selected anchor arrives at destination"), Transform->ComputeTransform(First, Data).GetLocation().Equals(Destination * 100.0));
 TestFalse(TEXT("Reapplying the same position makes no edit"), UHyperManageTransform::MakeWorldPositionOffset(Destination, Destination * 100.0, Data));
 TestTrue(TEXT("World zero is a destination, not an ignored axis"), UHyperManageTransform::MakeWorldPositionOffset(FVector::ZeroVector, First.GetLocation(), Data));
 TestTrue(TEXT("All coordinates reach world zero"), Transform->ComputeTransform(First, Data).GetLocation().IsNearlyZero());
 TestFalse(TEXT("Long-distance move is rejected"), UHyperManageTransform::MakeWorldPositionOffset(FVector(1000.001, 0, 0), FVector::ZeroVector, Data));
 TestFalse(TEXT("Out-of-range world coordinates are rejected"), UHyperManageTransform::MakeWorldPositionOffset(FVector(10001, 0, 0), FVector(1000000, 0, 0), Data));
 TestFalse(TEXT("Invalid reference is rejected"), UHyperManageTransform::MakeWorldPositionOffset(FVector::ZeroVector, FVector(std::numeric_limits<double>::infinity(), 0, 0), Data));
 TestFalse(TEXT("Invalid destination is rejected"), UHyperManageTransform::MakeWorldPositionOffset(FVector(std::numeric_limits<double>::quiet_NaN(), 0, 0), FVector::ZeroVector, Data));
 for (uint8 Mask = 1; Mask <= 7; ++Mask) {
  TestTrue(TEXT("Any nonempty axis combination can move"), UHyperManageTransform::MakeWorldPositionOffset(Destination, Reference, Data, Mask));
  const auto A = Transform->ComputeTransform(First, Data), B = Transform->ComputeTransform(Second, Data);
  const FVector Center = (A.GetLocation() + B.GetLocation()) / 2.0;
  for (int32 Axis = 0; Axis < 3; ++Axis) {
   TestTrue(TEXT("Checked axes reach destination; unchecked axes remain fixed"), FMath::IsNearlyEqual(Center[Axis], Mask & (1 << Axis) ? Destination[Axis] * 100.0 : Reference[Axis]));
  }
  TestTrue(TEXT("Axis-constrained move preserves spacing"), (B.GetLocation() - A.GetLocation()).Equals(Second.GetLocation() - First.GetLocation()));
  TestTrue(TEXT("Axis-constrained move preserves rotation and scale"), A.GetRotation().Equals(First.GetRotation()) && A.GetScale3D().Equals(First.GetScale3D()));
  TestFalse(TEXT("Same checked coordinates are a no-op"), UHyperManageTransform::MakeWorldPositionOffset(Destination, Center, Data, Mask));
 }
 TestFalse(TEXT("All unchecked makes no edit"), UHyperManageTransform::MakeWorldPositionOffset(Destination, Reference, Data, 0));
 TestFalse(TEXT("Unknown axis bits rejected"), UHyperManageTransform::MakeWorldPositionOffset(Destination, Reference, Data, 8));
 TestTrue(TEXT("Unchecked distant coordinates do not block a height-only move"), UHyperManageTransform::MakeWorldPositionOffset(FVector(-9999, 9999, 130), Reference, Data, 4));
 TestTrue(TEXT("Height-only move has no horizontal offset"), Data.PivotTranslation.X == 0 && Data.PivotTranslation.Y == 0);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageWorldOrientationTest, "HyperManage.Transform.WorldOrientation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageWorldOrientationTest::RunTest(const FString& Parameters)
{
 auto* Transform = NewObject<UHyperManageTransform>();
 const FTransform Reference(FRotator(25, 170, -35), FVector(-1000, 2300, 750), FVector(2, 0.5, 1));
 const FTransform Neighbor(FRotator(-10, 70, 20), FVector(-300, 2000, 950));
 for (const FRotator Desired : {FRotator::ZeroRotator, FRotator(40, -170, 65), FRotator(90, 35, -20), FRotator(-90, 180, 0)}) {
  FHyperManageTransformData Data;
  TestTrue(TEXT("Absolute orientation creates an edit"), UHyperManageTransform::MakeWorldOrientation(Desired, Reference, Data));
  const auto Result = Transform->ComputeTransform(Reference, Data);
  const auto Other = Transform->ComputeTransform(Neighbor, Data);
  TestTrue(TEXT("Reference reaches requested quaternion including wraparound and vertical pitch"), Result.GetRotation().Equals(Desired.Quaternion(), 0.000001));
  TestTrue(TEXT("Reference origin stays fixed"), Result.GetLocation().Equals(Reference.GetLocation()));
  TestTrue(TEXT("Nonuniform scale is preserved"), Result.GetScale3D().Equals(Reference.GetScale3D()));
  TestTrue(TEXT("Group distance is preserved"), FMath::IsNearlyEqual(FVector::Distance(Result.GetLocation(), Other.GetLocation()), FVector::Distance(Reference.GetLocation(), Neighbor.GetLocation()), 0.000001));
  const FQuat RelativeBefore = Reference.GetRotation().Inverse() * Neighbor.GetRotation();
  TestTrue(TEXT("Group relative orientations are preserved"), (Result.GetRotation().Inverse() * Other.GetRotation()).Equals(RelativeBefore, 0.000001));
  TestFalse(TEXT("Repeated absolute orientation is a no-op"), UHyperManageTransform::MakeWorldOrientation(Desired, Result, Data));
 }
 FHyperManageTransformData Data;
 TestFalse(TEXT("Angles outside input bounds are rejected"), UHyperManageTransform::MakeWorldOrientation(FRotator(0, 181, 0), Reference, Data));
 TestFalse(TEXT("NaN angle is rejected"), UHyperManageTransform::MakeWorldOrientation(FRotator(0, std::numeric_limits<double>::quiet_NaN(), 0), Reference, Data));
 FTransform Invalid = Reference; Invalid.SetRotation(FQuat(0, 0, 0, 0));
 TestFalse(TEXT("Invalid reference rotation is rejected"), UHyperManageTransform::MakeWorldOrientation(FRotator::ZeroRotator, Invalid, Data));
 const FTransform Tilted(FRotator(25, 170, -35), FVector(120, -320, 450), FVector(2, 1, 0.5));
 const FRotator Requested(-40, -175, 60);
 for (uint8 Mask = 1; Mask <= 7; ++Mask) {
  const FRotator Original = Tilted.Rotator();
  const FRotator Expected(Mask & 2 ? Requested.Pitch : Original.Pitch, Mask & 1 ? Requested.Yaw : Original.Yaw, Mask & 4 ? Requested.Roll : Original.Roll);
  TestTrue(TEXT("Any nonempty angle combination can rotate"), UHyperManageTransform::MakeWorldOrientation(Requested, Tilted, Data, Mask));
  const auto Result = Transform->ComputeTransform(Tilted, Data), Other = Transform->ComputeTransform(Neighbor, Data);
  TestTrue(TEXT("Unchecked reference Euler components are retained"), Result.GetRotation().Equals(Expected.Quaternion(), 0.000001));
  TestTrue(TEXT("Constrained orientation keeps the pivot and scale"), Result.GetLocation().Equals(Tilted.GetLocation()) && Result.GetScale3D().Equals(Tilted.GetScale3D()));
  TestTrue(TEXT("Group relative rotation survives constrained orientation"), (Result.GetRotation().Inverse() * Other.GetRotation()).Equals(Tilted.GetRotation().Inverse() * Neighbor.GetRotation(), 0.000001));
  TestFalse(TEXT("Repeated checked angles make no edit"), UHyperManageTransform::MakeWorldOrientation(Requested, Result, Data, Mask));
 }
 TestFalse(TEXT("No checked angles makes no edit"), UHyperManageTransform::MakeWorldOrientation(Requested, Tilted, Data, 0));
 TestFalse(TEXT("Unknown orientation axis bits rejected"), UHyperManageTransform::MakeWorldOrientation(Requested, Tilted, Data, 8));
 TestTrue(TEXT("Unused angle values are ignored"), UHyperManageTransform::MakeWorldOrientation(FRotator(999, 90, -999), Tilted, Data, 1));
 const FTransform Vertical(FRotator(90, 40, 20));
 const FRotator VerticalEuler = Vertical.Rotator();
 TestTrue(TEXT("Partial vertical orientation is supported"), UHyperManageTransform::MakeWorldOrientation(FRotator(0, -70, 0), Vertical, Data, 1));
 TestTrue(TEXT("Vertical orientation uses the reference canonical Euler angles"), Transform->ComputeTransform(Vertical, Data).GetRotation().Equals(FRotator(VerticalEuler.Pitch, -70, VerticalEuler.Roll).Quaternion(), 0.000001));
 return true;
}

#endif
