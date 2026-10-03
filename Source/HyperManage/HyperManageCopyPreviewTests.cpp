#include "HyperManageCopyPreview.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
	auto* Preview = NewObject<UHyperManageCopyPreview>();
	FString Message;
	TestTrue(TEXT("Capture a supported source"), Preview->Capture({Source}, FVector(8, 0, 0), Message));
	TestEqual(TEXT("One captured piece"), Preview->GetCount(), 1);
	if (Preview->GetCount() == 1 && !Preview->Pieces[0].HighlightMeshes.IsEmpty())
	{
		auto* Ghost = Preview->Pieces[0].HighlightMeshes[0].Get();
		auto* Owner = Preview->Pieces[0].HighlightOwner.Get();
		FTransform Expected = Initial; Expected.AddToTranslation(FVector(800, 0, 0));
		TestTrue(TEXT("Offset preserves snapshot rotation and scale"), Ghost->GetComponentTransform().Equals(Expected));
		TestNull(TEXT("Snapshot detached from live source"), Ghost->GetAttachParent());
		TestFalse(TEXT("Preview has no gameplay collision"), Owner->GetActorEnableCollision());
		TestTrue(TEXT("Capture leaves source untouched"), Source->GetActorTransform().Equals(Initial));
		Source->SetActorLocation(FVector(9000));
		TestTrue(TEXT("Source movement cannot move captured preview"), Ghost->GetComponentTransform().Equals(Expected));
		TestTrue(TEXT("Preview accepts a new offset"), Preview->SetOffset(FVector(0, 0, 2)));
		Expected = Initial; Expected.AddToTranslation(FVector(0, 0, 200));
		TestTrue(TEXT("Offsets are absolute from snapshot, not cumulative"), Ghost->GetComponentTransform().Equals(Expected));
		TestFalse(TEXT("Nonfinite offset rejected"), Preview->SetOffset(FVector(std::numeric_limits<double>::infinity(), 0, 0)));
		TestFalse(TEXT("Oversized offset rejected"), Preview->SetOffset(FVector(1001, 0, 0)));
		TestTrue(TEXT("Rejected offsets preserve geometry"), Ghost->GetComponentTransform().Equals(Expected));
		auto* Unsupported = World->SpawnActor<AActor>();
		TestFalse(TEXT("Unsupported mixed capture rejected"), Preview->Capture({Source, Unsupported}, FVector::ZeroVector, Message));
		TestFalse(TEXT("Duplicate source rejected"), Preview->Capture({Source, Source}, FVector::ZeroVector, Message));
		TestTrue(TEXT("Failed replacement keeps existing preview"), IsValid(Owner) && Ghost->GetComponentTransform().Equals(Expected));
		Source->Destroy();
		TestTrue(TEXT("Source deletion leaves snapshot registered"), Ghost->IsRegistered());
		Preview->Clear();
		TestEqual(TEXT("Cancel releases all pieces"), Preview->GetCount(), 0);
		TestTrue(TEXT("Cancel destroys preview owner"), Owner->IsActorBeingDestroyed());
		TestFalse(TEXT("Cancel unregisters geometry"), Ghost->IsRegistered());
		TestFalse(TEXT("Offset update without capture rejected"), Preview->SetOffset(FVector::ZeroVector));
		Preview->Clear();
	}
	World->DestroyWorld(false);
	return true;
}
#endif
