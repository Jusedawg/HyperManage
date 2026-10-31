#include "HyperManageCopyPreview.h"
#include "HyperManageInput.h"
#include "HyperManageCopyPlacement.h"
#include "Camera/PlayerCameraManager.h"
#include "HyperManageUI.h"
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

void UHyperManageCopyPreview::CaptureSelection()
{
	if (!System || !System->Selection || !System->UI) return;
	if (System->Selection->HasPendingOperations())
	{
		System->UI->ShowPopup(TEXT("Copy preview"), TEXT("Wait for pending edits before copying."));
		return;
	}
	TArray<AActor*> Actors;
	System->Selection->SelectedActorsNoTarget(Actors);
	FString Message;
	if (!Capture(Actors, PreferredOffsetMeters, Message))
	{
		System->UI->ShowPopup(TEXT("Cannot copy selection"), Message);
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
	TArray<FHyperManageCopySnapshot> Metadata;
	FBox Bounds(ForceInit);
	for (auto* Actor : Actors)
	{
		auto& Snapshot = Metadata.AddDefaulted_GetRef();
		Snapshot.Transform = Actor->GetActorTransform();
		if (const auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor))
		{
			auto* Subsystem = AFGLightweightBuildableSubsystem::Get(World);
			const auto* Data = Subsystem ? Subsystem->GetRuntimeDataForBuildableClassAndIndex(Proxy->Ref.BuildableClass, Proxy->Ref.Index) : nullptr;
			if (Proxy->Ref.Matches(Data))
			{
				Snapshot.Class = Proxy->Ref.BuildableClass;
				Snapshot.Recipe = Data->BuiltWithRecipe;
				Snapshot.Transform = Data->Transform;
				Snapshot.Customization = Data->CustomizationData;
				Snapshot.CanPlace = !Data->TypeSpecificData.IsValid();
			}
		}
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
			Bounds += Mesh->Bounds.GetBox();
		}
	}
	Clear();
	Pieces = MoveTemp(Captured);
	Snapshots = MoveTemp(Metadata);
	OriginalMeshTransforms = MoveTemp(Transforms);
	CapturedBounds = Bounds;
	ApplyOffset(Meters, true);
	ShowPreview(!FollowMouse);
	Message = FString::Printf(TEXT("%d pieces previewed. Close the panel to inspect. Place asks for confirmation and material costs."), Pieces.Num());
	return true;
}

bool UHyperManageCopyPreview::SetOffset(const FVector& Meters)
{
	if (!ValidOffset(Meters) || Pieces.IsEmpty()) return false;
	SetFollowMouse(false);
	return ApplyOffset(Meters, true);
}

bool UHyperManageCopyPreview::ApplyOffset(const FVector& Meters, bool Remember)
{
	if (!ValidOffset(Meters) || Pieces.IsEmpty()) return false;
	OffsetCm = Meters * 100.;
	if (Remember)
	{
		PreferredOffsetMeters = Meters;
	}
	if (System && System->Input)
	{
		System->Input->RefreshPreviewBinding();
	}
	++Revision;
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

bool UHyperManageCopyPreview::SurfaceOffset(const FBox& Bounds, const FVector& Point, const FVector& Normal, FVector& Meters)
{
	if (!Bounds.IsValid || Point.ContainsNaN() || Normal.ContainsNaN() || Normal.IsNearlyZero()) return false;
	const FVector Direction = Normal.GetSafeNormal();
	const double Radius = FVector::DotProduct(Bounds.GetExtent(), Direction.GetAbs());
	Meters = (Point + Direction * Radius - Bounds.GetCenter()) / 100.;
	return ValidOffset(Meters);
}

void UHyperManageCopyPreview::ShowPreview(bool Visible)
{
	for (auto& Piece : Pieces)
	{
		if (IsValid(Piece.HighlightOwner))
		{
			Piece.HighlightOwner->SetActorHiddenInGame(!Visible);
		}
	}
}

void UHyperManageCopyPreview::SetFollowMouse(bool Enabled)
{
	if (FollowMouse == Enabled) return;
	FollowMouse = Enabled;
	SurfaceValid = false;
	++Revision;
	if (!Enabled && !Pieces.IsEmpty())
	{
		PreferredOffsetMeters = GetOffsetMeters();
	}
	ShowPreview(!Enabled);
}

void UHyperManageCopyPreview::TickPlacement()
{
	if (!FollowMouse || Pieces.IsEmpty() || !System || !System->GetWorld()) return;
	auto* Controller = System->GetLocalController();
	if (!Controller || !Controller->PlayerCameraManager || Controller->bShowMouseCursor || Controller->IsLookInputIgnored() || System->GetWorld()->IsPaused()) return;
	if (System->CopyPlacement && System->CopyPlacement->IsAwaitingConfirmation()) return;
	const FVector Start = Controller->PlayerCameraManager->GetCameraLocation();
	const FVector End = Start + Controller->PlayerCameraManager->GetCameraRotation().Vector() * 10000.;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HyperManageCopyPlacement), true);
	Params.AddIgnoredActor(Controller->GetPawn());
	for (const auto& Piece : Pieces)
	{
		Params.AddIgnoredActor(Piece.HighlightOwner);
	}
	FHitResult Hit;
	FVector Meters;
	const bool HitSurface = System->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.bBlockingHit && !Hit.bStartPenetrating;
	const bool Ready = HitSurface && SurfaceOffset(CapturedBounds, Hit.ImpactPoint, Hit.ImpactNormal, Meters);
	if (Ready && !GetOffsetMeters().Equals(Meters, 0.0001))
	{
		ApplyOffset(Meters, false);
	}
	if (Ready != SurfaceValid)
	{
		++Revision;
	}
	SurfaceValid = Ready;
	ShowPreview(Ready);
}

void UHyperManageCopyPreview::Clear()
{
	DestroyPieces(Pieces);
	OriginalMeshTransforms.Reset();
	Snapshots.Reset();
	CapturedBounds = FBox(ForceInit);
	SurfaceValid = false;
	++Revision;
	OffsetCm = FVector::ZeroVector;
	if (System && System->Input)
	{
		System->Input->RefreshPreviewBinding();
	}
}
