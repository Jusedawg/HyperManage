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
	TestNotNull(TEXT("Movement presets created"), Tools->MovementPreset.Get());
	TestNotNull(TEXT("Rotation presets created"), Tools->RotationPreset.Get());
	TestNotNull(TEXT("Grid presets created"), Tools->GridPreset.Get());
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
	TestTrue(TEXT("Version label identifies the repaired menu"), Labels.Contains(TEXT("HyperManage | dev.13")));
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
#endif
