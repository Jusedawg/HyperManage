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
	TestNotNull(TEXT("Undo control created"), Tools->UndoButton.Get());
	TestNotNull(TEXT("Redo control created"), Tools->RedoButton.Get());
	TestNotNull(TEXT("Movement presets created"), Tools->MovementPreset.Get());
	TestNotNull(TEXT("Rotation presets created"), Tools->RotationPreset.Get());
	TestNotNull(TEXT("Grid presets created"), Tools->GridPreset.Get());
	TestNotNull(TEXT("Exact movement field created"), Tools->MovementValue.Get());
	TestNotNull(TEXT("Exact rotation field created"), Tools->RotationValue.Get());
	TestNotNull(TEXT("Exact grid field created"), Tools->GridValue.Get());
	TestFalse(TEXT("Numeric fields cannot consume drag gestures as a slider"), Tools->MovementValue->GetEnableSlider());
	TestNotNull(TEXT("Toolbar body has an explicit size"), Cast<USizeBox>(Content->GetParent()));
	TArray<FString> Labels;
	Tree->ForEachWidget([&](UWidget* Widget) {
		if (auto* Text = Cast<UTextBlock>(Widget)) Labels.Add(Text->GetText().ToString());
	});
	TestTrue(TEXT("Working actions get readable labels"), Labels.Contains(TEXT("Clear selection")));
	TestNull(TEXT("Unfinished copy action is removed from the visible layout"), Tools->btnCopySelection->GetParent());
	TestTrue(TEXT("Snap XY is in the visible hierarchy"), Labels.Contains(TEXT("Snap XY")));
	TestTrue(TEXT("Snap rotation is in the visible hierarchy"), Labels.Contains(TEXT("Snap rotation")));
	TestTrue(TEXT("Level is in the visible hierarchy"), Labels.Contains(TEXT("Level")));
	TestTrue(TEXT("Version label identifies the repaired menu"), Labels.Contains(TEXT("HyperManage | dev.17")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageClipboardLayoutTest, "HyperManage.UI.OriginalClipboard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageClipboardLayoutTest::RunTest(const FString& Parameters)
{
	auto* Clipboard = NewObject<UHyperManageClipboardWidget>();
	auto Slate = Clipboard->RebuildWidget();
	Clipboard->StatusText->SetText(FText::FromString(TEXT("Selected: 123 | Individual\nMove: 0.25m  Rotate: 90 deg\nGrid: 8m | Object axes")));
	Clipboard->ShortcutsText->SetText(FText::FromString(TEXT("Ctrl+LMB  Select\nCtrl+RMB  Deselect\nShift+LMB  Anchor\nShift+RMB  Target\nRMB  Tools    Ctrl+Z  Undo")));
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
	TestTrue(TEXT("Accept custom world grid in meters"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Grid, 1.25f));
	TestEqual(TEXT("Movement converts to centimeters"), Config->MMConfig.IncrementSettings[1].CentimetersToMove, 37.5f);
	TestEqual(TEXT("Other profiles remain unchanged"), Config->MMConfig.IncrementSettings[0].CentimetersToMove, 1.f);
	TestEqual(TEXT("Grid converts to centimeters"), Config->MMConfig.AlignmentGridCm, 125.f);
	TestFalse(TEXT("Unchanged value does not request another write"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Movement, 0.375f));
	for (float Invalid : {-1.f, 0.f, 1001.f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
		TestFalse(TEXT("Reject invalid movement"), Config->SetPrecisionValue(EHyperManagePrecisionSetting::Movement, Invalid));
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
	History->PushUndoTransforms(Actors);
	Actor->SetActorTransform(After);
	FUndoInfo Info;
	for (int32 Index = 0; Index < 3; ++Index) {
		TestTrue(TEXT("Undo is available"), History->PopUndo(Info));
		if (Info.TransformActors.Num() != 1) { AddError(TEXT("Expected one valid transform record")); break; }
		TestTrue(TEXT("Undo restores the original transform"), Info.TransformActors[0].Transform.Equals(Before));
		Actor->SetActorTransform(Info.TransformActors[0].Transform);
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
	for (int32 Index = 0; Index < 1005; ++Index) History->PushUndoTransforms(Actors);
	TestEqual(TEXT("History stays bounded"), History->GetUndoCount(), 1000);
	History->ClearUndoStack();
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
#endif
