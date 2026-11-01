#include "HyperManageCopyPreview.h"
#include "HyperManageInput.h"
#include "HyperManageCopyPlacement.h"
#include "Camera/PlayerCameraManager.h"
#include "HyperManageUI.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Buildables/FGBuildableWall.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Hologram/FGHologram.h"
#include "Hologram/FGFoundationHologram.h"
#include "Materials/MaterialInterface.h"

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
	auto* HologramMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/FactoryGame/Equipment/BuildGun/Material/Hologram_Blue_Inst.Hologram_Blue_Inst"));
	if (!HologramMaterial)
	{
		Message = TEXT("The construction hologram material is unavailable. The previous preview is unchanged.");
		return false;
	}
	TArray<AActor*> Ordered = Actors;
	if (System && System->Selection)
	{
		const int32 AnchorIndex = Ordered.IndexOfByKey(System->Selection->AnchorActor);
		if (AnchorIndex > 0)
		{
			Ordered.Swap(0, AnchorIndex);
		}
	}
	TArray<FSelectedActorInfo> Captured;
	TArray<FTransform> Transforms;
	TArray<FHyperManageCopySnapshot> Metadata;
	FBox Bounds(ForceInit);
	for (auto* Actor : Ordered)
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
			Mesh->SetRenderInMainPass(true);
			Mesh->SetRenderInDepthPass(true);
			Mesh->SetRenderCustomDepth(false);
			for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
			{
				Mesh->SetMaterial(Slot, HologramMaterial);
			}
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
	Message = FString::Printf(TEXT("%d pieces previewed. Close the panel to inspect. Place builds immediately using available materials."), Pieces.Num());
	return true;
}

bool UHyperManageCopyPreview::SetOffset(const FVector& Meters)
{
	if (!ValidOffset(Meters) || Pieces.IsEmpty()) return false;
	SetFollowMouse(false);
	return ApplyOffset(Meters, true);
}

bool UHyperManageCopyPreview::ResetRotation()
{
	if (Pieces.IsEmpty()) return false;
	if (System && System->CopyPlacement && System->CopyPlacement->IsExecuting()) return false;
	RotationDegrees = 0;
	PlacementRotation = FQuat::Identity;
	ApplyOffset(GetOffsetMeters(), false);
	if (FollowMouse)
	{
		SurfaceValid = false;
		ShowPreview(false);
		TickPlacement();
	}
	return true;
}

bool UHyperManageCopyPreview::RotatePreview(int32 Degrees)
{
	if (Pieces.IsEmpty() || (Degrees != 90 && Degrees != -90)) return false;
	if (System && System->CopyPlacement && System->CopyPlacement->IsExecuting()) return false;
	if (System && System->GetWorld() && System->GetLocalController())
	{
		auto* Controller = System->GetLocalController();
		if (Controller->bShowMouseCursor || Controller->IsLookInputIgnored()) return false;
	}
	RotationDegrees = (RotationDegrees + Degrees + 360) % 360;
	PlacementRotation = (FRotator(0, Degrees, 0).Quaternion() * PlacementRotation).GetNormalized();
	ApplyOffset(GetOffsetMeters(), false);
	if (FollowMouse)
	{
		SurfaceValid = false;
		ShowPreview(false);
		TickPlacement();
	}
	return true;
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
			const FTransform Pose = GetPlacementTransform(OriginalMeshTransforms[Index++]);
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

FTransform UHyperManageCopyPreview::GetPlacementTransform(const FTransform& Source) const
{
	const FVector Pivot = Snapshots.IsEmpty() ? FVector::ZeroVector : Snapshots[0].Transform.GetLocation();
	return FTransform((PlacementRotation * Source.GetRotation()).GetNormalized(), Pivot + OffsetCm + PlacementRotation.RotateVector(Source.GetLocation() - Pivot), Source.GetScale3D());
}

bool UHyperManageCopyPreview::TryNativeSnap(const FHitResult& Hit, FTransform& Pose)
{
	if (Snapshots.IsEmpty() || !System) return false;
	const auto& Reference = Snapshots[0];
	if (!Reference.Recipe || !Reference.Class || !Reference.Class->IsChildOf(AFGBuildableFoundation::StaticClass()) || !Reference.Transform.GetScale3D().Equals(FVector::OneVector)) return false;
	auto* Controller = System->GetLocalController();
	APawn* Pawn = Controller ? Controller->GetPawn().Get() : nullptr;
	if (!Pawn) return false;
	if (!IsValid(SnapHologram))
	{
		SnapHologram = AFGHologram::SpawnHologramFromRecipe(Reference.Recipe, Pawn, Reference.Transform.GetLocation(), Pawn, [](AFGHologram* Hologram)
		{
			Hologram->SetFlags(RF_Transient);
			Hologram->SetReplicates(false);
		});
		if (!IsValid(SnapHologram)) return false;
		SnapHologram->SetActorEnableCollision(false);
		SnapHologram->SetActorTickEnabled(false);
		SnapHologram->SetActorHiddenInGame(true);
	}
	if (!SnapHologram->IsA<AFGFoundationHologram>()) return false;
	FHitResult SnapHit = Hit;
	FInstanceHandle Handle;
	if (auto* Manager = AAbstractInstanceManager::GetInstanceManager(System->GetWorld()); Manager && Manager->ResolveHit(Hit, Handle))
	{
		FLightweightBuildableInstanceRef Instance;
		if (AFGLightweightBuildableSubsystem::ResolveLightweightInstance(Handle, Instance))
		{
			if (auto* Target = Instance.SpawnTemporaryBuildable())
			{
				SnapHit = FHitResult(Target, Cast<UPrimitiveComponent>(Target->GetRootComponent()), Hit.ImpactPoint, Hit.ImpactNormal);
				SnapHit.TraceStart = Hit.TraceStart; SnapHit.TraceEnd = Hit.TraceEnd;
				SnapHit.bBlockingHit = true; SnapHit.Distance = Hit.Distance;
			}
		}
	}
	// Seed the captured orientation, then let the native hologram choose the snapped pose.
	SnapHologram->SetActorTransform(Reference.Transform);
	SnapHologram->UpdateRotationValuesFromTransform();
	SnapHologram->ScrollRotate(RotationDegrees / 90, 90);
	const bool Snapped = SnapHologram->TrySnapToActor(SnapHit);
	Pose = SnapHologram->GetActorTransform();
	SnapHologram->SetActorHiddenInGame(true);
	return Snapped && HyperManageLightweight::IsValidTransform(Pose);
}

void UHyperManageCopyPreview::TickPlacement()
{
	if (!FollowMouse || Pieces.IsEmpty() || !System || !System->GetWorld()) return;
	auto* Controller = System->GetLocalController();
	if (!Controller || !Controller->PlayerCameraManager || Controller->bShowMouseCursor || Controller->IsLookInputIgnored() || System->GetWorld()->IsPaused()) return;
	if (System->CopyPlacement && System->CopyPlacement->IsExecuting()) return;
	const FVector Start = Controller->PlayerCameraManager->GetCameraLocation();
	const FVector End = Start + Controller->PlayerCameraManager->GetCameraRotation().Vector() * 10000.;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HyperManageCopyPlacement), true);
	Params.AddIgnoredActor(Controller->GetPawn());
	Params.AddIgnoredActor(SnapHologram);
	for (const auto& Piece : Pieces)
	{
		Params.AddIgnoredActor(Piece.HighlightOwner);
	}
	FHitResult Hit;
	FVector Meters;
	const bool HitSurface = System->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.bBlockingHit && !Hit.bStartPenetrating;
	FTransform SnappedPose;
	const bool Snapped = HitSurface && TryNativeSnap(Hit, SnappedPose);
	FQuat Rotation = FRotator(0, RotationDegrees, 0).Quaternion();
	bool Ready = false;
	if (Snapped)
	{
		Meters = (SnappedPose.GetLocation() - Snapshots[0].Transform.GetLocation()) / 100.;
		Rotation = (SnappedPose.GetRotation() * Snapshots[0].Transform.GetRotation().Inverse()).GetNormalized();
		Ready = ValidOffset(Meters);
	}
	else
	{
		const FVector Pivot = Snapshots[0].Transform.GetLocation();
		const FTransform BoundsRotation(Rotation, Pivot - Rotation.RotateVector(Pivot));
		Ready = HitSurface && SurfaceOffset(CapturedBounds.TransformBy(BoundsRotation), Hit.ImpactPoint, Hit.ImpactNormal, Meters);
	}
	const bool RotationChanged = !PlacementRotation.Equals(Rotation);
	if (Ready && (RotationChanged || !GetOffsetMeters().Equals(Meters, 0.0001)))
	{
		PlacementRotation = Rotation;
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
	if (IsValid(SnapHologram))
	{
		SnapHologram->Destroy();
	}
	SnapHologram = nullptr;
	PlacementRotation = FQuat::Identity;
	RotationDegrees = 0;
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
