#include "HyperManageCopyPreview.h"
#include "HyperManageInput.h"
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
		Source->Destroy();
		TestTrue(TEXT("Source deletion leaves snapshot registered"), Ghost->IsRegistered());
		Preview->Clear();
		TestFalse(TEXT("Empty preview cannot rotate"), Preview->RotatePreview(90));
		TestEqual(TEXT("Cancel releases all pieces"), Preview->GetCount(), 0);
		TestFalse(TEXT("Escape passes through after cancel"), Input->HyperManageInputComponent->KeyBindings[0].bConsumeInput);
		TestTrue(TEXT("Last applied offset survives cancellation"), Preview->GetPreferredOffsetMeters().Equals(FVector(0, 0, 2)));
		TestTrue(TEXT("Cancel destroys preview owner"), Owner->IsActorBeingDestroyed());
		TestFalse(TEXT("Cancel unregisters geometry"), Ghost->IsRegistered());
		TestFalse(TEXT("Offset update without capture rejected"), Preview->SetOffset(FVector::ZeroVector));
		Preview->Clear();
	}
	World->DestroyWorld(false);
	return true;
}
#endif
