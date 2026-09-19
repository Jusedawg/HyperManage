#include "HyperManageUndo.h"
#include "HyperManageSelection.h"

void FUndoInfo::Clear()
{
	Lightweights.Empty();
	ColorSlotItems.Empty();
	TransformActors.Empty();
	TransformComponents.Empty();
	SelectItems.Empty();
}

void UHyperManageUndo::ClearUndoStack()
{
	while (UndoCount > 0) {
		UndoInfoArr[UndoHead].Clear();
		Pop();
	}
}

void UHyperManageUndo::Pop()
{
	UndoCount--;
	UndoHead = (UndoHead + (MAXUNDO - 1)) % MAXUNDO;
}

bool UHyperManageUndo::PopUndo(FUndoInfo& UndoInfo)
{
	if (UndoCount > 0) {
		UndoInfo = UndoInfoArr[UndoHead];
		Pop();
		return true;
	}
	return false;
}

void UHyperManageUndo::Push()
{
	UndoCount = FMath::Min(UndoCount + 1, MAXUNDO);
	UndoHead = (UndoHead + 1) % MAXUNDO;
}

void UHyperManageUndo::PushUndoTransforms(TArray<AActor*>& Actors)
{
	Push();
	UndoInfoArr[UndoHead].Clear();
	UndoInfoArr[UndoHead].TransformActors.Reserve(Actors.Num());
	for (const auto& Actor : Actors) {
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
			FUndoLightweight Item; Item.Proxy = Proxy; Item.Transform = Proxy->GetActorTransform();
			UndoInfoArr[UndoHead].Lightweights.Add(Item);
			continue;
		}
		// push all "root" components to TransformActors/Components
		for (const auto& ActorComp : TInlineComponentArray<USceneComponent*>(Actor)) {
			USceneComponent* SceneComp = Cast<USceneComponent>(ActorComp);
			if (SceneComp && !SceneComp->GetAttachParent()) {
				if (SceneComp == Actor->GetRootComponent()) {
					FUndoTransformActor UndoTransformActor;
					UndoTransformActor.Actor = Actor;
					UndoTransformActor.Transform = SceneComp->GetComponentTransform();
					UndoInfoArr[UndoHead].TransformActors.Add(UndoTransformActor);
				} else {
					FUndoTransformComponent UndoTransformComponent;
					UndoTransformComponent.Component = SceneComp;
					UndoTransformComponent.Transform = SceneComp->GetComponentTransform();
					UndoInfoArr[UndoHead].TransformComponents.Add(UndoTransformComponent);
				}
			}
		}
	}
}

void UHyperManageUndo::PushUndoColorSlot(TArray<AActor*>& Actors)
{
	Push();
	UndoInfoArr[UndoHead].Clear();
	UndoInfoArr[UndoHead].ColorSlotItems.Reserve(Actors.Num());
	for (const auto& Actor : Actors) {
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
			FUndoLightweight Item; Item.Proxy = Proxy; Item.Paint = true; Item.Customization = Proxy->Customization;
			UndoInfoArr[UndoHead].Lightweights.Add(Item);
			continue;
		}
		AFGBuildable* Buildable = Cast<AFGBuildable>(Actor);
		if (Buildable) {
			FUndoColorSlot UndoColorSlot;
			UndoColorSlot.Buildable = Buildable;
			UndoColorSlot.CustomizationData = Buildable->GetCustomizationData_Implementation();
			UndoInfoArr[UndoHead].ColorSlotItems.Add(UndoColorSlot);
		}
	}
}

void UHyperManageUndo::PushUndoSelection(TArray<AActor*>& Actors)
{
	auto AddUndoSelect = [&](AActor* Actor)
	{
		FUndoSelect UndoSelect;
		UndoSelect.Actor = Actor;
		UndoSelect.Select = (Actor != nullptr) && System->Selection->Contains(Actor);
		UndoInfoArr[UndoHead].SelectItems.Add(UndoSelect);
	};

	Push();
	UndoInfoArr[UndoHead].Clear();
	UndoInfoArr[UndoHead].SelectItems.Reserve(Actors.Num() + 2);
	AddUndoSelect(System->Selection->AnchorActor);
	AddUndoSelect(System->Selection->TargetActor);
	for (const auto& Actor : Actors) {
		AddUndoSelect(Actor);
	}
}
