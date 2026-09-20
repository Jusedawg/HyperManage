#include "HyperManageSelection.h"
#include "HyperManageConfig.h"
#include "HyperManageAction.h"
#include "HyperManageTransform.h"
#include "HyperManageUndo.h"
#include "HyperManageUI.h"
#include "HyperManageEquip.h"

#include "FGOutlineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "FGVehicle.h"
#include "Buildables/FGBuildableWire.h"
#include "WheeledVehicles/FGTargetPoint.h"


void UHyperManageSelection::Init()
{
	Super::Init();
	SelectedMap.Empty();
	AnchorActor = nullptr;
	TargetActor = nullptr;

}

void UHyperManageSelection::SetSelectedMaterial(TArray<UMaterialInterface*> Materials)
{
	// ignore .pak materials for now
}

void UHyperManageSelection::SelectNextMaterial()
{
	// Retain the legacy action binding, but never return to destructive material replacement.
	for (const auto& Elem : SelectedMap) ResetHologram(Elem.Key);
}

void UHyperManageSelection::ShowHologram(AActor* Actor, FSelectedActorInfo& ActorInfo)
{
	if (!IsValid(Actor)) return;
	auto* Outline = UFGOutlineComponent::Get(Actor->GetWorld());
	if (!Outline) return;
	// The runtime helper writes enum values directly to the stencil. Keep the proven game-owned geometry,
 // using a dedicated channel outside the animated built-in channels 1-9.
 if (!IsValid(SelectionPostProcess)) {
  auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/HyperManage/Materials/M_SelectionOutline.M_SelectionOutline"));
  if (Material && Outline->GetOwner()) {
   SelectionPostProcess = NewObject<UPostProcessComponent>(Outline->GetOwner(), NAME_None, RF_Transient);
   SelectionPostProcess->bUnbound = true; SelectionPostProcess->bEnabled = true;
   SelectionPostProcess->BlendWeight = 1.f; SelectionPostProcess->Priority = 100.f;
   SelectionPostProcess->Settings.AddBlendable(Material, 1.f);
   SelectionPostProcess->RegisterComponent();
  }
  const FString Report = FString::Printf(TEXT("Material=%s component=%d registered=%d\n"), *GetNameSafe(Material),
   IsValid(SelectionPostProcess), IsValid(SelectionPostProcess) && SelectionPostProcess->IsRegistered());
  FFileHelper::SaveStringToFile(Report, *FPaths::Combine(FPaths::ProjectLogDir(), TEXT("HyperManage-Outline.txt")));
 }
 const EOutlineColor SelectionColor = IsValid(SelectionPostProcess) && SelectionPostProcess->IsRegistered() ?
  static_cast<EOutlineColor>(252) : EOutlineColor::OC_DISMANTLE;
	const EOutlineColor Color = Actor == TargetActor ? EOutlineColor::OC_RED :
		Actor == AnchorActor ? EOutlineColor::OC_HOLOGRAM : SelectionColor;
	ActorInfo.Outline = Outline;
	ActorInfo.PreviousOutlineColor = static_cast<uint8>(Outline->GetOutlineStateColorForActor(Actor));
	ActorInfo.SelectionOutlineColor = static_cast<uint8>(Color);
	if (Actor->IsA<AHyperManageLightweightProxy>()) Actor->SetActorHiddenInGame(false);
	// Preserve the working game-managed geometry and cleanup; only the selection color channel changes.
	// Lightweight proxy meshes supply geometry only; they never render a second surface in the main pass.
	Outline->ShowOutline(Actor, Color);
 FString Report = FString::Printf(TEXT("Actor=%s requested=%d actual=%d\n"), *GetNameSafe(Actor), static_cast<uint8>(Color),
  static_cast<uint8>(Outline->GetOutlineStateColorForActor(Actor)));
 for (auto* Mesh : TInlineComponentArray<UStaticMeshComponent*>(Actor)) {
  Report += FString::Printf(TEXT("Mesh=%s depth=%d stencil=%d\n"), *GetNameSafe(Mesh), Mesh->bRenderCustomDepth, Mesh->CustomDepthStencilValue);
 }
 FFileHelper::SaveStringToFile(Report, *FPaths::Combine(FPaths::ProjectLogDir(), TEXT("HyperManage-Outline.txt")),
  FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
}

void UHyperManageSelection::HideHologram(AActor* Actor, FSelectedActorInfo& ActorInfo)
{
	if (!IsValid(Actor)) return;
	if (auto* Outline = ActorInfo.Outline.Get()) {
		if (static_cast<uint8>(Outline->GetOutlineStateColorForActor(Actor)) == ActorInfo.SelectionOutlineColor) {
			Outline->HideOutline(Actor);
			if (ActorInfo.PreviousOutlineColor != static_cast<uint8>(EOutlineColor::OC_NONE)) {
				Outline->ShowOutline(Actor, static_cast<EOutlineColor>(ActorInfo.PreviousOutlineColor));
			}
		}
	}
	ActorInfo.Outline.Reset();
	if (Actor->IsA<AHyperManageLightweightProxy>()) Actor->SetActorHiddenInGame(true);
}

void UHyperManageSelection::ResetHologram(AActor* Actor)
{
	auto* ActorInfo = GetActorInfo(Actor);
	if (!IsValidActor(Actor) || !ActorInfo) return;
	HideHologram(Actor, *ActorInfo);
	ShowHologram(Actor, *ActorInfo);
}

void UHyperManageSelection::RefreshMaterial(AActor* Actor)
{
	if (Contains(Actor)) {
		ResetHologram(Actor);
		return;
	}
	// Refresh unselected mesh render state after a transform.
	if (IsValid(Actor) && !Contains(Actor)) {
		for (UMeshComponent* Mesh : TInlineComponentArray<UMeshComponent*>(Actor)) {
			Mesh->MarkRenderStateDirty();
		}
		// reset color slot for this actor (e.g. for buggy ramps)
		auto Buildable = Cast<AFGBuildable>(Actor);
		if (Buildable) {
			Buildable->ApplyCustomizationData_Native(Buildable->GetCustomizationData_Implementation());
		}
	}
}

void UHyperManageSelection::RefreshMaterials(const TArray<AActor*>& Actors)
{
	for (auto& Actor : Actors) {
		RefreshMaterial(Actor);
	}
}

FSelectedActorInfo* UHyperManageSelection::GetActorInfo(AActor* Actor)
{
	return SelectedMap.Find(Actor);
}

bool UHyperManageSelection::IsValidActor(AActor* Actor)
{
	if (!IsValid(Actor) || Actor->GetWorld() != System->GetWorld()) return false;
	if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) return Proxy->IsAvailable();
	if (auto* Buildable = Cast<AFGBuildable>(Actor); Buildable && Buildable->GetIsLightweightTemporary()) return false;
	if (IsValid(Actor)) {
		if (Actor->IsA<AFGBuildable>() || Actor->IsA<AFGVehicle>() || Actor->IsA<AFGTargetPoint>()) {
			return true;
		}
	}
	return false;
}

bool UHyperManageSelection::SetMarker(AActor* Actor, AActor** Marker1, AActor** Marker2)
{
	if (Actor && !IsValidActor(Actor)) {
		return false;
	}
	bool ClearMarker = (Actor == nullptr) || (Actor == *Marker1);
	if (*Marker1) { // clear Marker1
		auto TempActor = *Marker1;
		*Marker1 = nullptr;
		ResetHologram(TempActor);
	}
	if (ClearMarker) {
		return false;
	}
	if (Actor == *Marker2) { // clear Marker2
		auto TempActor = *Marker2;
		*Marker2 = nullptr;
		ResetHologram(TempActor);
	}
	// set Marker1
	*Marker1 = Actor;
	if (Contains(Actor)) {
		ResetHologram(Actor);
	} else {
		SelectActor(Actor, true);
	}
	return true;
}

bool UHyperManageSelection::SetAnchor(AActor* Actor)
{
	return SetMarker(Actor, &AnchorActor, &TargetActor);
}

bool UHyperManageSelection::SetTarget(AActor* Actor)
{
	return SetMarker(Actor, &TargetActor, &AnchorActor);
}

bool UHyperManageSelection::SelectActor(AActor* Actor, bool Select, bool DeleteFromMap)
{
	if (Select) {
		if (IsValidActor(Actor) && !SelectedMap.Find(Actor)) {
			FSelectedActorInfo ActorInfo = FSelectedActorInfo();
			ShowHologram(Actor, ActorInfo);
			SelectedMap.Add(Actor, ActorInfo);
			return true;
		}
	} else {
		FSelectedActorInfo* ActorInfoPtr = SelectedMap.Find(Actor);
		if (ActorInfoPtr) {
			FSelectedActorInfo ActorInfo = *ActorInfoPtr;
			HideHologram(Actor, ActorInfo);
			if (DeleteFromMap) {
				SelectedMap.Remove(Actor);
			}
			if (Actor == AnchorActor) {
				AnchorActor = nullptr;
			}
			if (Actor == TargetActor) {
				TargetActor = nullptr;
			}
			return true;
		}
	}
	return false;
}

bool UHyperManageSelection::Contains(AActor* Actor)
{
	return SelectedMap.Contains(Actor);
}

int UHyperManageSelection::SelectCount()
{
	int32 Count = 0;
	for (const auto& Entry : SelectedMap) { if (IsValidActor(Entry.Key) && Entry.Key != TargetActor) ++Count; }
	return Count;
}

void UHyperManageSelection::SelectedActors(TArray<AActor*>& Actors)
{
	SelectedMap.GenerateKeyArray(Actors);
	Actors.RemoveAll([this](AActor* Actor) { return !IsValidActor(Actor); });
}

void UHyperManageSelection::SelectedActorsNoTarget(TArray<AActor*>& Actors)
{
	SelectedActors(Actors);
	if (TargetActor) {
		Actors.RemoveSingleSwap(TargetActor);
	}
}

void UHyperManageSelection::GetSelectionOrLineTrace(TArray<AActor*>& Actors)
{
	Actors.Empty();
	if (SelectCount() == 0) {
		AActor* Actor = LineTraceFromPlayer();
		if (IsValidActor(Actor)) {
			Actors.Add(Actor);
		}
		System->Action->MakeActorsMovable(Actors);
	} else {
		SelectedActorsNoTarget(Actors);
	}
}

// Selects buildables that are inside the cube formed by Anchor and Target. Calculates 6 planes
// that define the cube with an additional delta component (PlaneDelta) for the distance outside
// the cube that is still considered valid.  The planes of the cube are oriented so that positive
// distance calculations are facing the center of the cube.

// Delta: For pivot location mode, this delta will just be a small
// value (default 0.5m) to prevent slight variations from excluding desired objects.  For sides mode,
// the delta will be the 0.5m plus the additional distance from the origin point of the buildable and
// the side.

// CenterOnPlane is a boolean calculated for each plane that will be true if the center point between
// Target and Anchor is within +/- delta.  When CenterOnPlane is true (usually for walls or buildables
// along the same axis), a buildable must be within delta distance from the plane to be added.
// When CenterOnPlane is false, the buildable must be on the positive distance side of all planes
// (within delta) to be added.

void UHyperManageSelection::AddAnchorTargetBoxToSelection(bool UseSides)
{
	struct FCubeSide
	{
		FPlane Plane;
		float Delta;
		bool CenterOnPlane;
	public:
		FCubeSide() {}
		FCubeSide(const FVector& CubeFarLoc, const FVector& CornerLoc, const FVector& CornerNormal, const float InDelta)
			: Plane(FPlane(CornerLoc, CornerNormal))
			, Delta(InDelta)
		{
			const float Dist = Plane.PlaneDot(CubeFarLoc);
			if (Dist < 0) {
				Plane *= -1.f; // flip the plane
			}
			CenterOnPlane = FMath::Abs(Dist) < Delta;
		}
	};

	if (!AnchorActor || !TargetActor) {
		System->UI->ShowPopup(TITLE_REQUIRES_ANCHOR_AND_TARGET, BODY_REQUIRES_ANCHOR_AND_TARGET);
		return;
	}

	// initialize Anchor, Target, Center locations and Anchor, Target deltas
	FVector AnchorLoc;
	FVector TargetLoc;
	FVector AnchorDelta;
	FVector TargetDelta;
	System->Transform->GetActorOriginAndSize(AnchorActor, AnchorLoc, AnchorDelta);
	System->Transform->GetActorOriginAndSize(TargetActor, TargetLoc, TargetDelta);
	if (!UseSides) {
		AnchorDelta = FVector::ZeroVector;
		TargetDelta = FVector::ZeroVector;
	}
	AnchorDelta = (AnchorDelta / 2.f) + System->Config->MMConfig.SelectionTolerance;
	TargetDelta = (TargetDelta / 2.f) + System->Config->MMConfig.SelectionTolerance;
	FVector CenterLoc = (AnchorLoc + TargetLoc) / 2.f;

	// initialize cube sides
	TArray<FCubeSide> CubeSides;
	FQuat Quat = AnchorActor->GetActorQuat();
	CubeSides.Add(FCubeSide(TargetLoc, AnchorLoc, Quat.GetAxisX(), AnchorDelta.X));
	CubeSides.Add(FCubeSide(TargetLoc, AnchorLoc, Quat.GetAxisY(), AnchorDelta.Y));
	CubeSides.Add(FCubeSide(TargetLoc, AnchorLoc, Quat.GetAxisZ(), AnchorDelta.Z));
	Quat = TargetActor->GetActorQuat();
	CubeSides.Add(FCubeSide(AnchorLoc, TargetLoc, Quat.GetAxisX(), TargetDelta.X));
	CubeSides.Add(FCubeSide(AnchorLoc, TargetLoc, Quat.GetAxisY(), TargetDelta.Y));
	CubeSides.Add(FCubeSide(AnchorLoc, TargetLoc, Quat.GetAxisZ(), TargetDelta.Z));

	const auto InsideCube = [&CubeSides](const FVector& Center) {
		for (const auto& Side : CubeSides) {
			const float Distance = Side.Plane.PlaneDot(Center);
			if (Distance < -Side.Delta || (Side.CenterOnPlane && Distance > Side.Delta)) return false;
		}
		return true;
	};
	TArray<AActor*> AddedActors;
	for (TObjectIterator<AFGBuildable> Worker; Worker; ++Worker) {
		if (Worker->GetWorld() != System->GetWorld() || !IsValidActor(*Worker) || Worker->GetIsLightweightTemporary() || Contains(*Worker) || Worker->IsA<AFGBuildableWire>()) continue;
		FVector Center, Extent;
		Worker->GetActorBounds(false, Center, Extent);
		if (InsideCube(Center)) AddedActors.Add(*Worker);
	}
	if (auto* Subsystem = AFGLightweightBuildableSubsystem::Get(System->GetWorld())) {
		for (const auto& Entry : Subsystem->GetAllLightweightBuildableInstances()) {
			for (int32 Index = 0; Index < Entry.Value.Num(); ++Index) {
				const auto& Data = Entry.Value[Index];
				if (!Data.IsValid()) continue;
				const FVector Center = Data.BoundingBox.IsValid ? Data.Transform.TransformPosition(Data.BoundingBox.GetCenter()) : Data.Transform.GetLocation();
				if (!InsideCube(Center)) continue;
				if (auto* Proxy = GetLightweightProxy(Entry.Key, Index, Data); Proxy && !Contains(Proxy)) AddedActors.Add(Proxy);
			}
		}
	}

	// push selection undo (client), select all added actors (client) and then prepare those actors to be movable (server)
	System->Undo->PushUndoSelection(AddedActors);
	for (const auto& Actor : AddedActors) {
		SelectActor(Actor);
	}
	System->Action->MakeActorsMovable(AddedActors);

	// toggle (deselect) AnchorActor and TargetActor
	SetAnchor(AnchorActor);
	SetTarget(TargetActor);
}

bool UHyperManageSelection::SelectActorWithHistory(AActor* Actor, bool Select)
{
	if (HasPendingOperations() || !IsValidActor(Actor) || Contains(Actor) == Select) return false;
	TArray<AActor*> Affected = {Actor};
	if (System->Undo) System->Undo->PushUndoSelection(Affected);
	return SelectActor(Actor, Select);
}

bool UHyperManageSelection::SetMarkerWithHistory(AActor* Actor, bool Anchor)
{
	if (HasPendingOperations() || (Actor && !IsValidActor(Actor))) return false;
	if (!Actor && !(Anchor ? AnchorActor : TargetActor)) return false;
	TArray<AActor*> Affected;
	if (Actor) Affected.AddUnique(Actor);
	if (AnchorActor) Affected.AddUnique(AnchorActor);
	if (TargetActor) Affected.AddUnique(TargetActor);
	if (System->Undo) System->Undo->PushUndoSelection(Affected);
	return Anchor ? SetAnchor(Actor) : SetTarget(Actor);
}

void UHyperManageSelection::ClearWithoutHistory()
{
	AnchorActor = nullptr;
	TargetActor = nullptr;
	for (auto& Elem : SelectedMap) SelectActor(Elem.Key, false, false);
	SelectedMap.Empty();
	if (IsValid(SelectionPostProcess)) { SelectionPostProcess->DestroyComponent(); SelectionPostProcess = nullptr; }
}

void UHyperManageSelection::SelectClear(bool ConfirmClicked)
{
	if (!ConfirmClicked || HasPendingOperations()) return;
	TArray<AActor*> Affected;
	SelectedActors(Affected);
	if (Affected.IsEmpty() && !AnchorActor && !TargetActor) return;
	if (System->Undo) System->Undo->PushUndoSelection(Affected);
	ClearWithoutHistory();
}

void UHyperManageSelection::RestoreHistory(const FUndoInfo& Info)
{
	if (Info.SelectItems.Num() < 2) return;
	SetAnchor(nullptr);
	SetTarget(nullptr);
	for (int32 Index = 2; Index < Info.SelectItems.Num(); ++Index) {
		const auto& Item = Info.SelectItems[Index];
		if (IsValid(Item.Actor)) SelectActor(Item.Actor, Item.Select);
	}
	if (IsValidActor(Info.SelectItems[0].Actor)) SetAnchor(Info.SelectItems[0].Actor);
	if (IsValidActor(Info.SelectItems[1].Actor)) SetTarget(Info.SelectItems[1].Actor);
}

void UHyperManageSelection::SaveSelection()
{
	SelectedActorsNoTarget(SavedSelection);
	SavedAnchor = AnchorActor;
	SavedTarget = TargetActor;
}

void UHyperManageSelection::LoadSelection()
{
	if (HasPendingOperations()) return;
	TArray<AActor*> Desired;
	for (auto* Actor : SavedSelection) if (IsValidActor(Actor)) Desired.AddUnique(Actor);
	AActor* Anchor = IsValidActor(SavedAnchor) ? SavedAnchor : nullptr;
	AActor* Target = IsValidActor(SavedTarget) ? SavedTarget : nullptr;
	if (Anchor) Desired.AddUnique(Anchor);
	if (Target) Desired.AddUnique(Target);
	TArray<AActor*> Affected;
	SelectedActors(Affected);
	bool SameSelection = Affected.Num() == Desired.Num() && AnchorActor == Anchor && TargetActor == Target;
	for (auto* Actor : Desired) if (!Contains(Actor)) SameSelection = false;
	if (SameSelection) return;
	for (auto* Actor : Desired) Affected.AddUnique(Actor);
	if (System->Undo) System->Undo->PushUndoSelection(Affected);
	ClearWithoutHistory();
	for (auto* Actor : Desired) SelectActor(Actor);
	SetAnchor(Anchor);
	SetTarget(Target);
}

AActor* UHyperManageSelection::LineTraceFromPlayer()
{
	FVector Start = System->GetLocalController()->PlayerCameraManager->GetCameraLocation();
	FVector End = Start + (System->GetLocalController()->PlayerCameraManager->GetActorForwardVector() *
		(System->Config->MMConfig.MaxTargetRangeMeters * 100.0));
	FHitResult HitResult;
	FCollisionQueryParams TraceParams(TEXT("MMTrace"), false, System->GetLocalController()->GetPawn());
	if (System->GetWorld()->
		LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, TraceParams)) {
		UE_LOG(LogTemp, Display, TEXT("HyperManage trace: actor=%s component=%s item=%d"), *GetNameSafe(HitResult.GetActor()), *GetNameSafe(HitResult.GetComponent()), HitResult.Item);
		FInstanceHandle Handle;
		if (auto* Manager = AAbstractInstanceManager::GetInstanceManager(System->GetWorld()); Manager && Manager->ResolveHit(HitResult, Handle)) {
			FLightweightBuildableInstanceRef Instance;
			if (AFGLightweightBuildableSubsystem::ResolveLightweightInstance(Handle, Instance) && Handle.Metadata.IsValid()) {
				auto Metadata = StaticCastSharedPtr<FLightweightBuildableInstanceHandleMetadata>(Handle.Metadata);
				if (const auto* Data = Instance.ResolveBuildableInstanceData()) return GetLightweightProxy(Instance.GetBuildableClass(), Metadata->BuildableID, *Data);
			}
			if (IsValidActor(Handle.GetOwner())) return Handle.GetOwner();
		}
		if (auto* Buildable = Cast<AFGBuildable>(HitResult.GetActor()); Buildable && Buildable->GetIsLightweightTemporary()) {
			auto* Subsystem = AFGLightweightBuildableSubsystem::Get(System->GetWorld());
			const int32 Index = Subsystem ? Subsystem->GetRuntimeDataIndexForBuildable(Buildable) : INDEX_NONE;
			if (Index != INDEX_NONE) {
				if (const auto* Data = Subsystem->GetRuntimeDataForBuildableClassAndIndex(Buildable->GetClass(), Index)) return GetLightweightProxy(Buildable->GetClass(), Index, *Data);
			}
			return nullptr;
		}
		return HitResult.GetActor();
	}
	return nullptr;
}



AHyperManageLightweightProxy* UHyperManageSelection::GetLightweightProxy(TSubclassOf<AFGBuildable> Class, int32 Index, const FRuntimeBuildableInstanceData& Data)
{
	for (auto* Proxy : LightweightProxies) {
		if (IsValid(Proxy) && Proxy->IsAvailable() && Proxy->Ref.BuildableClass == Class && Proxy->Ref.Index == Index && Proxy->Ref.Matches(&Data)) return Proxy;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	auto* Proxy = System->GetWorld()->SpawnActor<AHyperManageLightweightProxy>(Params);
	if (Proxy) { Proxy->Initialize(Class, Index, Data); LightweightProxies.Add(Proxy); }
	return Proxy;
}

bool UHyperManageSelection::HasPendingOperations() const
{
	for (auto* Proxy : LightweightProxies) { if (IsValid(Proxy) && Proxy->IsPending()) return true; }
	return false;
}

void UHyperManageSelection::AcknowledgeLightweight(const FHyperManageLightweightRef& Ref, const FFactoryCustomizationData& Data, bool Success)
{
	for (auto* Proxy : LightweightProxies) {
		if (!IsValid(Proxy) || Proxy->Ref.SelectionId != Ref.SelectionId) continue;
		if (Success) {
			Proxy->ApplyAcknowledgement(Ref, Data);
			ResetHologram(Proxy);
		} else {
			SelectActor(Proxy, false);
			Proxy->Available = false;
			Proxy->SetActorHiddenInGame(true);
		}
		return;
	}
}