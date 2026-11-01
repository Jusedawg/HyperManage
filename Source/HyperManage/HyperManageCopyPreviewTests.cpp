#include "HyperManageCopyPreview.h"
#include "HyperManageCopyPlacement.h"
#include "HyperManageInput.h"
#include "HyperManageEquip.h"
#include "FGPlayerController.h"
#include "Components/InputComponent.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageCopyPreviewTest, "HyperManage.Copy.PreviewSnapshot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageCopyPreviewTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Preview test world"), World)) return false;
	auto* SourceClass = LoadClass<AFGBuildableFoundation>(nullptr, TEXT("/Game/FactoryGame/Buildable/Building/Foundation/Build_Foundation_8x1_01.Build_Foundation_8x1_01_C"));
	auto* Source = SourceClass ? World->SpawnActor<AFGBuildableFoundation>(SourceClass) : nullptr;
	if (!TestNotNull(TEXT("Concrete foundation fixture"), Source))
	{
		World->DestroyWorld(false);
		return false;
	}
	for (auto* Component : TInlineComponentArray<UStaticMeshComponent*>(Source))
	{
		Component->DestroyComponent();
	}
	auto* Mesh = NewObject<UStaticMeshComponent>(Source);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Source->SetRootComponent(Mesh);
	Source->AddInstanceComponent(Mesh);
	Mesh->RegisterComponent();
	const FTransform Initial(FRotator(10, 35, 0), FVector(200, 300, 400), FVector(1.5));
	Source->SetActorTransform(Initial);
	auto* System = NewObject<UHyperManageSystem>();
	System->CopyPreview = InitComponent<UHyperManageCopyPreview>(System);
	System->Input = InitComponent<UHyperManageInput>(System);
	auto* Preview = System->CopyPreview;
	auto* Input = System->Input;
	Input->HyperManageInputComponent = NewObject<UInputComponent>();
	Input->SetupKeyBinding(FHyperManageKeyConfig(CancelCopy, EKeys::Escape, false, false, false, false), IE_Pressed);
	TestFalse(TEXT("Escape passes through without preview"), Input->HyperManageInputComponent->KeyBindings[0].bConsumeInput);
	const TArray<FVector> ReferencePoints = {FVector(-400, -400, 0), FVector(400, 400, 0)};
	const TArray<FVector> GroupPoints = {FVector(-1200, -400, 0), FVector(1200, 400, 0)};
	TestTrue(TEXT("Centered three-piece group moves outward to meet its near edge"), UHyperManageCopyPreview::EdgeSnapOffset(GroupPoints, ReferencePoints, FQuat::Identity, FVector::ForwardVector).Equals(FVector(800, 0, 0)));
	TestTrue(TEXT("Opposite edge shifts in the opposite direction"), UHyperManageCopyPreview::EdgeSnapOffset(GroupPoints, ReferencePoints, FQuat::Identity, -FVector::ForwardVector).Equals(FVector(-800, 0, 0)));
	TestTrue(TEXT("Single piece retains native snap location"), UHyperManageCopyPreview::EdgeSnapOffset(ReferencePoints, ReferencePoints, FQuat::Identity, FVector::ForwardVector).IsNearlyZero());
	TestTrue(TEXT("Quarter-turned group uses its rotated edge"), UHyperManageCopyPreview::EdgeSnapOffset(GroupPoints, ReferencePoints, FRotator(0, 90, 0).Quaternion(), FVector::RightVector).Equals(FVector(0, 800, 0), 0.001));
	const FQuat Diagonal = FRotator(0, 37, 0).Quaternion();
	TestTrue(TEXT("Diagonal foundations preserve edge distance"), UHyperManageCopyPreview::EdgeSnapOffset(GroupPoints, ReferencePoints, Diagonal, Diagonal.RotateVector(FVector::ForwardVector)).Equals(Diagonal.RotateVector(FVector(800, 0, 0)), 0.001));
	TestTrue(TEXT("Missing edge geometry does not offset preview"), UHyperManageCopyPreview::EdgeSnapOffset({}, ReferencePoints, FQuat::Identity, FVector::ForwardVector).IsNearlyZero());
	FVector SurfaceMeters;
	const FBox Bounds(FVector(-100), FVector(100));
	TestTrue(TEXT("Surface positioning accepts floor"), UHyperManageCopyPreview::SurfaceOffset(Bounds, FVector(1000, 2000, 0), FVector::UpVector, SurfaceMeters));
	TestTrue(TEXT("Group bottom rests on floor"), SurfaceMeters.Equals(FVector(10, 20, 1)));
	TestTrue(TEXT("Surface positioning accepts wall"), UHyperManageCopyPreview::SurfaceOffset(Bounds, FVector(1000, 2000, 0), FVector::ForwardVector, SurfaceMeters));
	TestTrue(TEXT("Group rests outside wall"), SurfaceMeters.Equals(FVector(11, 20, 0)));
	TestFalse(TEXT("Invalid surface normal is rejected"), UHyperManageCopyPreview::SurfaceOffset(Bounds, FVector::ZeroVector, FVector::ZeroVector, SurfaceMeters));
	TestFalse(TEXT("Missing geometry bounds rejected"), UHyperManageCopyPreview::SurfaceOffset(FBox(ForceInit), FVector::ZeroVector, FVector::UpVector, SurfaceMeters));
	TestTrue(TEXT("Mouse following is default"), Preview->IsFollowingMouse());
	FString Message;
	TestTrue(TEXT("Capture a supported source"), Preview->Capture({Source}, FVector(8, 0, 0), Message));
	TestEqual(TEXT("One captured piece"), Preview->GetCount(), 1);
	const uint64 CapturedRevision = Preview->GetRevision();
	if (Preview->GetCount() == 1 && !Preview->Pieces[0].HighlightMeshes.IsEmpty())
	{
		TestTrue(TEXT("Escape is consumed while preview exists"), Input->HyperManageInputComponent->KeyBindings[0].bConsumeInput);
		auto* Ghost = Preview->Pieces[0].HighlightMeshes[0].Get();
		auto* Owner = Preview->Pieces[0].HighlightOwner.Get();
		FTransform Expected = Initial; Expected.AddToTranslation(FVector(800, 0, 0));
		TestTrue(TEXT("Offset preserves snapshot rotation and scale"), Ghost->GetComponentTransform().Equals(Expected));
		TestTrue(TEXT("Preview draws its actual mesh"), Ghost->bRenderInMainPass);
		TestFalse(TEXT("Preview does not depend on selection outline"), Ghost->bRenderCustomDepth);
		TestTrue(TEXT("Preview uses construction hologram material"), Ghost->GetMaterial(0) && Ghost->GetMaterial(0)->GetPathName().Contains(TEXT("Hologram_Blue_Inst")));
		TestNull(TEXT("Snapshot detached from live source"), Ghost->GetAttachParent());
		TestFalse(TEXT("Preview has no gameplay collision"), Owner->GetActorEnableCollision());
		TestTrue(TEXT("Capture leaves source untouched"), Source->GetActorTransform().Equals(Initial));
		Source->SetActorLocation(FVector(9000));
		TestTrue(TEXT("Source movement cannot move captured preview"), Ghost->GetComponentTransform().Equals(Expected));
		TestTrue(TEXT("Preview accepts a new offset"), Preview->SetOffset(FVector(0, 0, 2)));
		TestTrue(TEXT("Changing offset invalidates old confirmation revision"), Preview->GetRevision() != CapturedRevision);
		Expected = Initial; Expected.AddToTranslation(FVector(0, 0, 200));
		TestTrue(TEXT("Offsets are absolute from snapshot, not cumulative"), Ghost->GetComponentTransform().Equals(Expected));
		TestFalse(TEXT("Nonfinite offset rejected"), Preview->SetOffset(FVector(std::numeric_limits<double>::infinity(), 0, 0)));
		TestFalse(TEXT("Oversized offset rejected"), Preview->SetOffset(FVector(1001, 0, 0)));
		TestTrue(TEXT("Rejected offsets preserve geometry"), Ghost->GetComponentTransform().Equals(Expected));
		auto* Unsupported = World->SpawnActor<AActor>();
		TestFalse(TEXT("Unsupported mixed capture rejected"), Preview->Capture({Source, Unsupported}, FVector::ZeroVector, Message));
		TestFalse(TEXT("Duplicate source rejected"), Preview->Capture({Source, Source}, FVector::ZeroVector, Message));
		TestTrue(TEXT("Failed replacement keeps existing preview"), IsValid(Owner) && Ghost->GetComponentTransform().Equals(Expected));
		TestFalse(TEXT("Explicit offset switches to exact mode"), Preview->IsFollowingMouse());
		const uint64 BeforeRotation = Preview->GetRevision();
		TestTrue(TEXT("Quarter turn accepts clockwise wheel"), Preview->RotatePreview(90));
		TestTrue(TEXT("Rotation invalidates prior confirmation"), Preview->GetRevision() > BeforeRotation);
		TestTrue(TEXT("Rotation preserves exact offset"), Preview->GetOffsetMeters().Equals(FVector(0, 0, 2)));
		TestTrue(TEXT("Counterclockwise reverses turn"), Preview->RotatePreview(-90) && !Preview->HasPlacementRotation());
		TestFalse(TEXT("Unsupported wheel step rejected"), Preview->RotatePreview(45));
		TestTrue(TEXT("Quarter turn can be reapplied"), Preview->RotatePreview(90));
		TestTrue(TEXT("Preview turn leaves source unchanged"), Source->GetActorLocation().Equals(FVector(9000)));
		const FTransform Neighbor(FRotator(0, 15, 0), Initial.GetLocation() + FVector(800, 0, 0), FVector(2));
		const FTransform Placed = Preview->GetPlacementTransform(Neighbor);
		TestTrue(TEXT("Snapped group rotates spacing around reference"), Placed.GetLocation().Equals(Initial.GetLocation() + FVector(0, 800, 200), 0.01));
		TestTrue(TEXT("Snapped group preserves local scale"), Placed.GetScale3D().Equals(FVector(2)));
		TestTrue(TEXT("Snapped group rotates each piece consistently"), Placed.GetRotation().Equals(FRotator(0, 105, 0).Quaternion(), 0.001));
		const FVector FixedOffset = Preview->GetOffsetMeters();
		Preview->SetFollowMouse(true);
		TestFalse(TEXT("Mouse mode requires a fresh surface before placement"), Preview->IsPlacementReady());
		Preview->SetFollowMouse(false);
		TestTrue(TEXT("Switch to exact mode preserves preview position"), Preview->GetOffsetMeters().Equals(FixedOffset) && Preview->IsPlacementReady());
		TestTrue(TEXT("Exact mode keeps snapped rotation"), Preview->HasPlacementRotation());
		const uint64 BeforeReset = Preview->GetRevision();
		TestTrue(TEXT("Reset rotation succeeds for active preview"), Preview->ResetRotation());
		TestFalse(TEXT("Reset removes manual and snapped rotation in exact mode"), Preview->HasPlacementRotation());
		TestTrue(TEXT("Reset preserves preview offset"), Preview->GetOffsetMeters().Equals(FixedOffset));
		TestTrue(TEXT("Reset updates visible mesh orientation"), Ghost->GetComponentQuat().Equals(Initial.GetRotation(), 0.001));
		TestTrue(TEXT("Reset invalidates prior confirmation revision"), Preview->GetRevision() > BeforeReset);
		TestEqual(TEXT("Reset clears wheel accumulator"), Preview->RotationDegrees, 0);
		System->CurrentWorld = World;
		System->LocalController = World->SpawnActor<AFGPlayerController>();
		System->Selection = InitComponent<UHyperManageSelection>(System);
		System->Config = NewObject<UHyperManageConfiguration>(System);
		System->Config->MMKeyConfigs.ActionKeys = {FHyperManageKeyConfig(RotateCopyRight, EKeys::MouseScrollUp, false, false, false, false), FHyperManageKeyConfig(RotateCopyLeft, EKeys::MouseScrollDown, false, false, false, false)};
		Input->Equip = World->SpawnActor<AHyperManageEquip>();
		Input->RefreshPreviewBinding();
		TestTrue(TEXT("Active preview registers wheel interception"), Input->PreviewWheelProcessor.IsValid());
		if (TestNotNull(TEXT("Wheel test controller"), System->LocalController))
		{
			System->LocalController->bShowMouseCursor = false;
			TestTrue(TEXT("Preview consumes wheel before equipment switching"), Input->HandlePreviewWheel(1.f, false, false, false));
			TestTrue(TEXT("Consumed wheel rotates preview"), Preview->HasPlacementRotation());
			TestFalse(TEXT("Unmapped modifier chord passes through"), Input->HandlePreviewWheel(1.f, true, false, false));
			System->LocalController->bShowMouseCursor = true;
			TestFalse(TEXT("Open panel keeps wheel scrolling"), Input->HandlePreviewWheel(-1.f, false, false, false));
			System->LocalController->bShowMouseCursor = false;
			TestTrue(TEXT("Opposite wheel reverses preview turn"), Input->HandlePreviewWheel(-1.f, false, false, false) && !Preview->HasPlacementRotation());
		}
		System->CopyPlacement = InitComponent<UHyperManageCopyPlacement>(System);
		FHyperManageCopyBatchResult Complete;
		Complete.Completed = 1;
		TestTrue(TEXT("Complete placement retains preview by default"), System->CopyPlacement->FinishPreview(Complete));
		TestTrue(TEXT("Retained preview preserves geometry and input"), Ghost->IsRegistered() && Input->PreviewWheelProcessor.IsValid());
		System->Config->MMConfig.AutoAnchor = false;
		TestTrue(TEXT("Select-after-copy can preserve the reusable snapshot"), System->Selection->SelectPlacedCopies({Source}, true));
		TestEqual(TEXT("Selection replacement leaves reusable snapshot intact"), Preview->GetCount(), 1);
		Source->Destroy();
		TestTrue(TEXT("Source deletion leaves snapshot registered"), Ghost->IsRegistered());
		FHyperManageCopyBatchResult Partial;
		Partial.Completed = 1; Partial.Failed = true;
		TestFalse(TEXT("Partial failure ends repeat placement"), System->CopyPlacement->FinishPreview(Partial));
		TestFalse(TEXT("Empty preview cannot reset rotation"), Preview->ResetRotation());
		TestFalse(TEXT("Empty preview cannot rotate"), Preview->RotatePreview(90));
		TestFalse(TEXT("Cancel unregisters wheel interception"), Input->PreviewWheelProcessor.IsValid());
		TestFalse(TEXT("No preview leaves equipment wheel alone"), Input->HandlePreviewWheel(1.f, false, false, false));
		TestEqual(TEXT("Cancel releases all pieces"), Preview->GetCount(), 0);
		TestFalse(TEXT("Escape passes through after cancel"), Input->HyperManageInputComponent->KeyBindings[0].bConsumeInput);
		TestTrue(TEXT("Last applied offset survives cancellation"), Preview->GetPreferredOffsetMeters().Equals(FVector(0, 0, 2)));
		TestTrue(TEXT("Cancel destroys preview owner"), Owner->IsActorBeingDestroyed());
		TestFalse(TEXT("Cancel unregisters geometry"), Ghost->IsRegistered());
		TestFalse(TEXT("Offset update without capture rejected"), Preview->SetOffset(FVector::ZeroVector));
		Preview->Clear();
	}
	auto* AnotherSource = World->SpawnActor<AFGBuildableFoundation>(SourceClass);
	if (TestNotNull(TEXT("Single-placement source"), AnotherSource))
	{
		auto* AnotherMesh = NewObject<UStaticMeshComponent>(AnotherSource);
		AnotherMesh->SetMobility(EComponentMobility::Movable);
		AnotherMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		AnotherSource->SetRootComponent(AnotherMesh);
		AnotherSource->AddInstanceComponent(AnotherMesh);
		AnotherMesh->RegisterComponent();
		TestTrue(TEXT("Single-placement captures geometry"), Preview->Capture({AnotherSource}, FVector(8, 0, 0), Message));
		System->CopyPlacement->RepeatPlacement = false;
		FHyperManageCopyBatchResult Complete;
		Complete.Completed = 1;
		TestFalse(TEXT("Single-placement option ends successful preview"), System->CopyPlacement->FinishPreview(Complete));
		TestEqual(TEXT("Single-placement clears snapshot"), Preview->GetCount(), 0);
	}
	World->DestroyWorld(false);
	return true;
}
#endif
