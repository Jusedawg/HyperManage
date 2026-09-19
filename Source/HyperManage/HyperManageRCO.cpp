#include "HyperManageRCO.h"
#include "HyperManageSystem.h"
#include "HyperManageAction.h"
#include "HyperManageSelection.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Buildables/FGBuildableWire.h"
#include "FGFactoryConnectionComponent.h"
#include "FGCircuitConnectionComponent.h"
#include "FGVehicle.h"
#include "WheeledVehicles/FGTargetPoint.h"

namespace
{
	constexpr int32 MaxActorsPerRequest = 1024;

	bool IsEditableActor(const AActor* Actor, const UWorld* World)
	{
		if (const auto* Buildable = Cast<AFGBuildable>(Actor); Buildable && Buildable->GetIsLightweightTemporary()) return false;
		return IsValid(Actor) && Actor->GetWorld() == World && (Actor->IsA<AFGBuildable>() || Actor->IsA<AFGVehicle>() || Actor->IsA<AFGTargetPoint>());
	}

	bool AreActorsValid(const TArray<AActor*>& Actors, const UWorld* World)
	{
		if (Actors.Num() > MaxActorsPerRequest) return false;
		for (const AActor* Actor : Actors) {
			if (Actor && !IsEditableActor(Actor, World)) return false;
		}
		return true;
	}
}

AHyperManageEquip* UHyperManageRCO::GetRequestEquipment() const
{
	const AFGCharacterPlayer* Character = GetOwnerPlayerCharacter();
	if (!IsValid(Character)) return nullptr;
	for (AFGEquipment* Equipment : Character->GetActiveEquipments()) {
		if (auto* HyperManage = Cast<AHyperManageEquip>(Equipment)) return HyperManage;
	}
	return nullptr;
}
void UHyperManageRCO::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHyperManageRCO, Dummy);
}

void UHyperManageRCO::RemoveDuplicateWires(TArray<AFGBuildableWire*>& Wires)
{
	Algo::Sort(Wires);
	int Idx = Wires.Num() - 1;
	while (Idx > 0) {
		if (Wires[Idx] == Wires[Idx - 1]) {
			Wires.RemoveAtSwap(Idx);
		}
		Idx--;
	}
}

void UHyperManageRCO::ProcessWires(const TArray<AFGBuildableWire*>& Wires)
{
	// update all wires endpoints connecting to any circuit connections
	for (const auto& Wire : Wires) {
		if (!IsValid(Wire)) continue;
		UFGCircuitConnectionComponent* Cxn0 = Wire->GetConnection(0);
		UFGCircuitConnectionComponent* Cxn1 = Wire->GetConnection(1);
		if (!IsValid(Cxn0) || !IsValid(Cxn1)) continue;

		// create new wire based on old wire
		FActorSpawnParameters Params;
		Params.Instigator = Wire->GetInstigator();
		Params.Owner = Wire->GetOwner();
		const auto& NewWire = Wire->GetWorld()->SpawnActor<AFGBuildableWire>(Wire->GetClass(), Wire->GetActorTransform(), Params);
		if (!IsValid(NewWire)) continue;
		NewWire->SetBuiltWithRecipe(Wire->GetBuiltWithRecipe());

		// remove old wire
		Wire->Disconnect();
		Wire->GetWorld()->DestroyActor(Wire);

		// connect up new wire
		NewWire->Connect(Cxn0, Cxn1);
		Cxn0->AddConnection(NewWire);
		Cxn1->AddConnection(NewWire);
		// Connect initializes wire meshes; UpdateWireMeshes is private in the current game.
	}
}

void UHyperManageRCO::ServerTransformActors_Implementation(const TArray<AActor*>& Actors, FHyperManageTransformData TransformData)
{
	if (!GetRequestEquipment()) return;
	// get all connected wires
	TArray<AFGBuildableWire*> Wires;
	for (const auto& Actor : Actors) {
		for (const auto& CircuitCxnComp : TInlineComponentArray<UFGCircuitConnectionComponent*>(Actor)) {
			Cast<UFGCircuitConnectionComponent>(CircuitCxnComp)->GetWires(Wires);
		}
	}
	RemoveDuplicateWires(Wires);

	// process the transform across all actors
	GetRequestEquipment()->MulticastTransformActors(Actors, TransformData);

	// update the wire positions attached to all the actors
	ProcessWires(Wires);
}

bool UHyperManageRCO::ServerTransformActors_Validate(const TArray<AActor*>& Actors, FHyperManageTransformData TransformData)
{
	if (TransformData.WorldOriginAlignment && (!TransformData.WorldAlignment || TransformData.WorldRotationOffset ||
		(TransformData.TransformAxis != EAxis::X && TransformData.TransformAxis != EAxis::Y && TransformData.TransformAxis != EAxis::Z))) return false;
	return (!TransformData.WorldRotationOffset || (TransformData.WorldAlignment && UHyperManageTransform::IsValidRotationOffset(TransformData.Rot))) &&
		FMath::IsFinite(TransformData.AlignmentGridCm) && TransformData.AlignmentGridCm >= 1.0 && TransformData.AlignmentGridCm <= 100000.0 &&
		FMath::IsFinite(TransformData.AlignmentAngle) && TransformData.AlignmentAngle >= 0.1 && TransformData.AlignmentAngle <= 180.0 &&
		AreActorsValid(Actors, GetWorld()) && !TransformData.Loc.ContainsNaN() && !TransformData.Rot.ContainsNaN() &&
		!TransformData.Scale.ContainsNaN() && !TransformData.PivotQuat.ContainsNaN() && TransformData.PivotQuat.IsNormalized() &&
		!TransformData.PivotLoc.ContainsNaN() && !TransformData.PivotAxis.ContainsNaN() && FMath::IsFinite(TransformData.PivotAngle) &&
		!TransformData.PivotTranslation.ContainsNaN() && !TransformData.ViewVector.ContainsNaN() && !TransformData.AnchorQuat.ContainsNaN();
}

void UHyperManageRCO::ServerUndoAction_Implementation(const FUndoInfo& UndoInfo)
{
	if (!GetRequestEquipment()) return;
	if (UndoInfo.ColorSlotItems.Num() > 0) {
		for (const auto& UndoItem : UndoInfo.ColorSlotItems) {
			if (IsValid(UndoItem.Buildable)) {
				auto Data = UndoItem.Buildable->GetCustomizationData_Implementation();
				Data.SwatchDesc = UndoItem.CustomizationData.SwatchDesc;
				Data.OverrideColorData = UndoItem.CustomizationData.OverrideColorData;
				UndoItem.Buildable->SetCustomizationData_Native(Data, true);
				UndoItem.Buildable->ApplyCustomizationData_Native(Data);
			}
		}
	} else if (UndoInfo.TransformActors.Num() > 0 || UndoInfo.TransformComponents.Num() > 0) {
		// get all connected wires while on server
		TArray<AFGBuildableWire*> Wires;
		for (const auto& UndoActor : UndoInfo.TransformActors) {
			if (IsValid(UndoActor.Actor)) {
				for (auto CircuitCxnComp : TInlineComponentArray<UFGCircuitConnectionComponent*>(UndoActor.Actor)) {
					Cast<UFGCircuitConnectionComponent>(CircuitCxnComp)->GetWires(Wires);
				}
			}
		}
		RemoveDuplicateWires(Wires);

		// process the reverse transform across all undo items
		GetRequestEquipment()->MulticastUndoTransforms(UndoInfo);

		// update the wire positions attached to all the undo items while on server
		ProcessWires(Wires);
	}
}

bool UHyperManageRCO::ServerUndoAction_Validate(const FUndoInfo& UndoInfo)
{
	if (UndoInfo.TransformActors.Num() > MaxActorsPerRequest || UndoInfo.TransformComponents.Num() > MaxActorsPerRequest ||
		UndoInfo.ColorSlotItems.Num() > MaxActorsPerRequest || UndoInfo.SelectItems.Num() > MaxActorsPerRequest) return false;
	for (const auto& Item : UndoInfo.TransformActors) {
		if (Item.Transform.ContainsNaN() || !Item.Transform.GetRotation().IsNormalized()) return false;
		if (Item.Actor && !IsEditableActor(Item.Actor, GetWorld())) return false;
	}
	for (const auto& Item : UndoInfo.TransformComponents) {
		if (Item.Transform.ContainsNaN() || !Item.Transform.GetRotation().IsNormalized()) return false;
		if (Item.Component && (!IsValid(Item.Component) || !IsEditableActor(Item.Component->GetOwner(), GetWorld()))) return false;
	}
	for (const auto& Item : UndoInfo.ColorSlotItems) {
		if (Item.Buildable && !IsEditableActor(Item.Buildable, GetWorld())) return false;
	}
	return true;
}

void UHyperManageRCO::ServerPrepareActors_Implementation(const TArray<AActor*>& Actors)
{
	if (!GetRequestEquipment()) return;
	// make all characters fly
	TArray<TTuple<ACharacter*, EMovementMode>> CharacterInfo;
	for (FConstPlayerControllerIterator Iterator = UHyperManageSystem::GetForWorld(GetWorld())->GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
		ACharacter* Character = Iterator->Get()->GetCharacter();
		if (!IsValid(Character) || !Character->GetCharacterMovement()) continue;
		UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement();
		auto MovementMode = CharacterMovement->MovementMode;
		bool IsFlying = CharacterMovement->bCheatFlying;
		if (!IsFlying) {
			CharacterInfo.Emplace(Character, MovementMode);
			CharacterMovement->bCheatFlying = true;
			CharacterMovement->SetMovementMode(MOVE_Flying);
		}
	}

	// process through all actors
	for (const auto& Actor : Actors) {
		if (!IsEditableActor(Actor, GetWorld()) || !Actor->GetRootComponent()) continue;
		// save factory connections
		TMap<UFGFactoryConnectionComponent*, UFGFactoryConnectionComponent*> FactoryConnections;
		for (const auto& ActorComp : TInlineComponentArray<UFGFactoryConnectionComponent*>(Actor)) {
			auto FactoryCxnComp = Cast<UFGFactoryConnectionComponent>(ActorComp);
			if (FactoryCxnComp->IsConnected()) {
				UFGFactoryConnectionComponent* Cxn = FactoryCxnComp->GetConnection();
				FactoryConnections.Add(FactoryCxnComp, Cxn);
			}
		}

		const auto PreviousMobility = Actor->GetRootComponent()->Mobility;
		Actor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		Actor->GetRootComponent()->SetMobility(PreviousMobility);

		// reset factory connections
		for (const auto& Elem : FactoryConnections) {
			if (!Elem.Key->IsConnected()) {
				Elem.Key->SetConnection(Elem.Value);
			}
		}
	}

	// set all characters back to original movement mode
	for (const auto& CharMoveMode : CharacterInfo) {
		UCharacterMovementComponent* CharacterMovement = CharMoveMode.Key->GetCharacterMovement();
		CharacterMovement->bCheatFlying = false;
		CharacterMovement->SetMovementMode(CharMoveMode.Value);
	}

	// multi-cast out to all characters to redraw any changed objects
	GetRequestEquipment()->MulticastRefreshMaterials(Actors);
}

bool UHyperManageRCO::ServerPrepareActors_Validate(const TArray<AActor*>& Actors)
{
	return AreActorsValid(Actors, GetWorld());
}

void UHyperManageRCO::ServerPaintActors_Implementation(const TArray<AActor*>& Actors, const FFactoryCustomizationData& PaintData)
{
	if (!GetRequestEquipment()) return;
	// paint all buildables on server and colors will replicate on all clients
	for (const auto& Actor : Actors) {
		AFGBuildable* Buildable = Cast<AFGBuildable>(Actor);
		if (Buildable) {
			auto Data = Buildable->GetCustomizationData_Implementation();
			Data.SwatchDesc = PaintData.SwatchDesc;
			Data.OverrideColorData = PaintData.OverrideColorData;
			Buildable->SetCustomizationData_Native(Data, true);
			Buildable->ApplyCustomizationData_Native(Data);
		}
	}
}

bool UHyperManageRCO::ServerPaintActors_Validate(const TArray<AActor*>& Actors, const FFactoryCustomizationData& PaintData)
{
	return AreActorsValid(Actors, GetWorld());
}

void UHyperManageRCO::ServerHandleConnect_Implementation(const FGuid& Id, bool IsConnection, AActor* OutputActor, AActor* InputActor)
{
	if (!GetRequestEquipment()) return;
	FString Title;
	FString Body;
	if (IsConnection) {
		UHyperManageSystem::GetForWorld(GetWorld())->Action->MakeConnection(OutputActor, InputActor, Title, Body);
	} else {
		UHyperManageSystem::GetForWorld(GetWorld())->Action->BreakConnection(OutputActor, InputActor, Title, Body);
	}
	GetRequestEquipment()->MulticastShowPopup(Id, Title, Body);
}

bool UHyperManageRCO::ServerHandleConnect_Validate(const FGuid& Id, bool IsConnection, AActor* OutputActor, AActor* InputActor)
{
	return (!OutputActor || IsEditableActor(OutputActor, GetWorld())) && (!InputActor || IsEditableActor(InputActor, GetWorld()));
}

namespace
{
	constexpr int32 LightweightBatchSize = 64;
	template<typename T, typename F> void SendHyperManageBatches(const TArray<T>& Items, F Send)
	{
		for (int32 Start = 0; Start < Items.Num(); Start += LightweightBatchSize) {
			TArray<T> Batch; Batch.Append(Items.GetData() + Start, FMath::Min(LightweightBatchSize, Items.Num() - Start)); Send(Batch);
		}
	}
}

void UHyperManageRCO::RequestTransform(const TArray<AActor*>& Actors, FHyperManageTransformData Data)
{
	TArray<AActor*> NativeActors;
	TArray<FHyperManageLightweightRef> Refs;
	for (auto* Actor : Actors) {
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) { Proxy->BeginRequest(); Refs.Add(Proxy->Ref); }
		else if (IsValid(Actor)) NativeActors.Add(Actor);
	}
	if (IsValid(Data.Target)) Data.TargetRotation = Data.Target->GetActorQuat();
	if (Cast<AHyperManageLightweightProxy>(Data.Target)) Data.Target = nullptr;
	if (Cast<AHyperManageLightweightProxy>(Data.Anchor)) Data.Anchor = nullptr;
	SendHyperManageBatches(NativeActors, [&](const auto& Batch) { ServerTransformActors(Batch, Data); });
	Data.Target = nullptr; Data.Anchor = nullptr;
	SendHyperManageBatches(Refs, [&](const auto& Batch) { ServerTransformLightweights(Batch, Data); });
}

void UHyperManageRCO::RequestPaint(const TArray<AActor*>& Actors, const FFactoryCustomizationData& Paint)
{
	TArray<AActor*> NativeActors;
	TArray<FHyperManageLightweightEdit> Edits;
	for (auto* Actor : Actors) {
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
			Proxy->BeginRequest();
			FHyperManageLightweightEdit Edit; Edit.Ref = Proxy->Ref; Edit.Paint = true; Edit.Customization = Paint; Edits.Add(Edit);
		} else if (IsValid(Actor)) NativeActors.Add(Actor);
	}
	SendHyperManageBatches(NativeActors, [&](const auto& Batch) { ServerPaintActors(Batch, Paint); });
	SendHyperManageBatches(Edits, [&](const auto& Batch) { ServerEditLightweights(Batch); });
}

void UHyperManageRCO::RequestUndo(const FUndoInfo& Undo)
{
	FUndoInfo NativeUndo = Undo;
	NativeUndo.Lightweights.Empty();
	SendHyperManageBatches(NativeUndo.TransformActors, [&](const auto& Items) { FUndoInfo Batch; Batch.TransformActors = Items; ServerUndoAction(Batch); });
	SendHyperManageBatches(NativeUndo.TransformComponents, [&](const auto& Items) { FUndoInfo Batch; Batch.TransformComponents = Items; ServerUndoAction(Batch); });
	SendHyperManageBatches(NativeUndo.ColorSlotItems, [&](const auto& Items) { FUndoInfo Batch; Batch.ColorSlotItems = Items; ServerUndoAction(Batch); });
	TArray<FHyperManageLightweightEdit> Edits;
	for (const auto& Item : Undo.Lightweights) {
		if (!IsValid(Item.Proxy) || !Item.Proxy->IsAvailable()) continue;
		Item.Proxy->BeginRequest();
		FHyperManageLightweightEdit Edit; Edit.Ref = Item.Proxy->Ref; Edit.Transform = Item.Transform; Edit.Paint = Item.Paint;
		Edit.Customization = Item.Customization; Edits.Add(Edit);
	}
	SendHyperManageBatches(Edits, [&](const auto& Batch) { ServerEditLightweights(Batch); });
}

void UHyperManageRCO::RequestAbsoluteTransforms(const TArray<AActor*>& Actors, const FVector& Scale)
{
	FUndoInfo NativeTransforms;
	TArray<FHyperManageLightweightEdit> Edits;
	for (auto* Actor : Actors) {
		if (!IsValid(Actor)) continue;
		FTransform Transform;
		if (!UHyperManageTransform::MakeAbsoluteScale(Actor->GetActorTransform(), Scale, Transform)) continue;
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
			Proxy->BeginRequest();
			FHyperManageLightweightEdit Edit; Edit.Ref = Proxy->Ref; Edit.Transform = Transform; Edits.Add(Edit);
		} else {
			FUndoTransformActor Item; Item.Actor = Actor; Item.Transform = Transform; NativeTransforms.TransformActors.Add(Item);
		}
	}
	SendHyperManageBatches(NativeTransforms.TransformActors, [&](const auto& Items) { FUndoInfo Batch; Batch.TransformActors = Items; ServerUndoAction(Batch); });
	SendHyperManageBatches(Edits, [&](const auto& Batch) { ServerEditLightweights(Batch); });
}

FHyperManageLightweightRef* UHyperManageRCO::ResolveLightweight(const FHyperManageLightweightRef& Ref, AFGLightweightBuildableSubsystem* Subsystem)
{
	if (!Ref.SelectionId.IsValid() || !Ref.BuildableClass || !Ref.Recipe || Ref.Index < 0 || !Subsystem) return nullptr;
	auto* Stored = LightweightHandles.Find(Ref.SelectionId);
	const auto& Current = Stored ? *Stored : Ref;
	if (Current.BuildableClass != Ref.BuildableClass || Current.Recipe != Ref.Recipe) return nullptr;
	const auto* Instances = Subsystem->GetAllLightweightBuildableInstances().Find(Current.BuildableClass);
	if (!Instances || !Instances->IsValidIndex(Current.Index) || !Current.Matches(&(*Instances)[Current.Index])) return nullptr;
	if (!Stored && LightweightHandles.Num() >= 8192) return nullptr;
	return Stored ? Stored : &LightweightHandles.Add(Ref.SelectionId, Ref);
}

void UHyperManageRCO::ServerTransformLightweights_Implementation(const TArray<FHyperManageLightweightRef>& Refs, FHyperManageTransformData Data)
{
	if (!GetRequestEquipment() || Refs.Num() > LightweightBatchSize || !ServerTransformActors_Validate({}, Data) ||
		Data.TargetRotation.ContainsNaN() || !Data.TargetRotation.IsNormalized()) return;
	auto* Subsystem = AFGLightweightBuildableSubsystem::Get(GetWorld());
	auto* System = UHyperManageSystem::GetForWorld(GetWorld());
	if (!Subsystem || !System) return;
	TArray<FHyperManageLightweightEdit> Results;
	for (const auto& Ref : Refs) {
		FHyperManageLightweightEdit Result; Result.Ref = Ref;
		if (auto* Current = ResolveLightweight(Ref, Subsystem)) {
			const FTransform Updated = System->Transform->ComputeTransform(Current->ExpectedTransform, Data);
			Result.Accepted = HyperManageLightweight::Replace(Subsystem, *Current, Updated);
			Result.Ref = *Current;
			if (const auto* Runtime = Subsystem->GetRuntimeDataForBuildableClassAndIndex(Current->BuildableClass, Current->Index)) Result.Customization = Runtime->CustomizationData;
		}
		Results.Add(Result);
	}
	ClientLightweightResults(Results);
}

void UHyperManageRCO::ServerEditLightweights_Implementation(const TArray<FHyperManageLightweightEdit>& Edits)
{
	if (!GetRequestEquipment() || Edits.Num() > LightweightBatchSize) return;
	auto* Subsystem = AFGLightweightBuildableSubsystem::Get(GetWorld());
	if (!Subsystem) return;
	TArray<FHyperManageLightweightEdit> Results;
	for (const auto& Edit : Edits) {
		FHyperManageLightweightEdit Result; Result.Ref = Edit.Ref;
		if (auto* Current = ResolveLightweight(Edit.Ref, Subsystem)) {
			if (Edit.Paint) {
				auto Paint = Subsystem->GetRuntimeDataForBuildableClassAndIndex(Current->BuildableClass, Current->Index)->CustomizationData;
				Paint.SwatchDesc = Edit.Customization.SwatchDesc; Paint.OverrideColorData = Edit.Customization.OverrideColorData;
				FLightweightBuildableInstanceRef Instance; Instance.Initialize(Subsystem, Current->BuildableClass, Current->Index);
				Result.Accepted = Instance.SetCustomizationData(Paint);
			} else Result.Accepted = HyperManageLightweight::Replace(Subsystem, *Current, Edit.Transform);
			Result.Ref = *Current;
			if (const auto* Runtime = Subsystem->GetRuntimeDataForBuildableClassAndIndex(Current->BuildableClass, Current->Index)) Result.Customization = Runtime->CustomizationData;
		}
		Results.Add(Result);
	}
	ClientLightweightResults(Results);
}

void UHyperManageRCO::ClientLightweightResults_Implementation(const TArray<FHyperManageLightweightEdit>& Results)
{
	if (auto* System = UHyperManageSystem::GetForWorld(GetWorld())) {
		for (const auto& Result : Results) System->Selection->AcknowledgeLightweight(Result.Ref, Result.Customization, Result.Accepted);
	}
}