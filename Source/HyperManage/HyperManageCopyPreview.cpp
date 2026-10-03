#include "HyperManageCopyPreview.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Buildables/FGBuildableWall.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

namespace
{
	bool ValidOffset(const FVector& Meters)
	{
		return !Meters.ContainsNaN() && FMath::IsFinite(Meters.X) && FMath::IsFinite(Meters.Y) && FMath::IsFinite(Meters.Z) && Meters.GetAbsMax() <= 1000.;
	}
	void DestroyPieces(TArray<FSelectedActorInfo>& Pieces)
	{
		for (auto& Piece : Pieces)
		{
			if (IsValid(Piece.HighlightOwner))
			{
				Piece.HighlightOwner->Destroy();
			}
		}
		Pieces.Reset();
	}
}

bool UHyperManageCopyPreview::Capture(const TArray<AActor*>& Actors, const FVector& Meters, FString& Message)
{
	Message = TEXT("Select 1-50 vanilla foundations, ramps or walls. Preview only; nothing is built.");
	if (Actors.IsEmpty() || Actors.Num() > 50 || !ValidOffset(Meters)) return false;
	UWorld* World = IsValid(Actors[0]) ? Actors[0]->GetWorld() : nullptr;
	if (!World || World->GetNetMode() != NM_Standalone)
	{
		Message = TEXT("Copy preview currently requires single-player.");
		return false;
	}
	TSet<AActor*> Seen;
	for (auto* Actor : Actors)
	{
		if (!IsValid(Actor) || Actor->GetWorld() != World || Seen.Contains(Actor)) return false;
		Seen.Add(Actor);
		const auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor);
		const UClass* Class = Proxy ? Proxy->Ref.BuildableClass.Get() : Actor->GetClass();
		if (Proxy && (!Proxy->IsAvailable() || Proxy->IsPending())) return false;
		if (!Class || (!Class->IsChildOf(AFGBuildableFoundation::StaticClass()) && !Class->IsChildOf(AFGBuildableWall::StaticClass()))) return false;
		const FString Package = Class->GetOutermost()->GetName();
		if (!Package.StartsWith(TEXT("/Game/FactoryGame/")) && Package != TEXT("/Script/FactoryGame")) return false;
	}
	TArray<FSelectedActorInfo> Captured;
	TArray<FTransform> Transforms;
	for (auto* Actor : Actors)
	{
		auto& Piece = Captured.AddDefaulted_GetRef();
		UHyperManageSelection::CreateStorageHighlight(Actor, Piece);
		if (Piece.HighlightMeshes.IsEmpty())
		{
			DestroyPieces(Captured);
			Message = TEXT("A piece has no available preview geometry. The previous preview is unchanged.");
			return false;
		}
		for (const auto& Mesh : Piece.HighlightMeshes)
		{
			Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			Transforms.Add(Mesh->GetComponentTransform());
		}
	}
	Clear();
	Pieces = MoveTemp(Captured);
	OriginalMeshTransforms = MoveTemp(Transforms);
	SetOffset(Meters);
	Message = FString::Printf(TEXT("%d pieces previewed. Close the panel to inspect. No buildings or costs; Cancel clears the preview."), Pieces.Num());
	return true;
}

bool UHyperManageCopyPreview::SetOffset(const FVector& Meters)
{
	if (!ValidOffset(Meters) || Pieces.IsEmpty()) return false;
	OffsetCm = Meters * 100.;
	int32 Index = 0;
	for (auto& Piece : Pieces)
	{
		for (const auto& Mesh : Piece.HighlightMeshes)
		{
			FTransform Pose = OriginalMeshTransforms[Index++];
			Pose.AddToTranslation(OffsetCm);
			if (IsValid(Mesh))
			{
				Mesh->SetWorldTransform(Pose);
			}
		}
	}
	return true;
}

void UHyperManageCopyPreview::Clear()
{
	DestroyPieces(Pieces);
	OriginalMeshTransforms.Reset();
	OffsetCm = FVector::ZeroVector;
}
