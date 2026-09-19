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
	UndoStack.Empty();
	RedoStack.Empty();
}

bool UHyperManageUndo::HasPending(const FUndoInfo& Info) const
{
	for (const auto& Item : Info.Lightweights) if (IsValid(Item.Proxy) && Item.Proxy->IsPending()) return true;
	for (const auto& Item : Info.SelectItems) {
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Item.Actor); IsValid(Proxy) && Proxy->IsPending()) return true;
	}
	return false;
}

void UHyperManageUndo::Push(FUndoInfo&& Info)
{
	if (Info.TransformActors.IsEmpty() && Info.TransformComponents.IsEmpty() && Info.Lightweights.IsEmpty() && Info.ColorSlotItems.IsEmpty() && Info.SelectItems.IsEmpty()) return;
	RedoStack.Empty();
	if (UndoStack.Num() == MAXUNDO) UndoStack.RemoveAt(0);
	UndoStack.Add(MoveTemp(Info));
}

bool UHyperManageUndo::Transfer(TArray<FUndoInfo>& From, TArray<FUndoInfo>& To, FUndoInfo& Info)
{
	Info.Clear();
	while (!From.IsEmpty()) {
		if (HasPending(From.Last())) return false;
		Info = From.Pop();
		Info.TransformActors.RemoveAll([](const auto& Item) { return !IsValid(Item.Actor); });
		Info.TransformComponents.RemoveAll([](const auto& Item) { return !IsValid(Item.Component) || !IsValid(Item.Component->GetOwner()); });
		Info.ColorSlotItems.RemoveAll([](const auto& Item) { return !IsValid(Item.Buildable); });
		Info.Lightweights.RemoveAll([](const auto& Item) { return !IsValid(Item.Proxy) || !Item.Proxy->IsAvailable(); });
		FUndoInfo Inverse;
		for (const auto& Item : Info.TransformActors) {
			FUndoTransformActor Current; Current.Actor = Item.Actor; Current.Transform = Item.Actor->GetActorTransform(); Inverse.TransformActors.Add(Current);
		}
		for (const auto& Item : Info.TransformComponents) {
			FUndoTransformComponent Current; Current.Component = Item.Component; Current.Transform = Item.Component->GetComponentTransform(); Inverse.TransformComponents.Add(Current);
		}
		for (const auto& Item : Info.ColorSlotItems) {
			FUndoColorSlot Current; Current.Buildable = Item.Buildable; Current.CustomizationData = Item.Buildable->GetCustomizationData_Implementation(); Inverse.ColorSlotItems.Add(Current);
		}
		for (const auto& Item : Info.Lightweights) {
			FUndoLightweight Current; Current.Proxy = Item.Proxy; Current.Paint = Item.Paint;
			Current.Transform = Item.Proxy->GetActorTransform(); Current.Customization = Item.Proxy->Customization; Inverse.Lightweights.Add(Current);
		}
		if (Info.SelectItems.Num() >= 2 && System && System->Selection) {
			for (int32 Index = 0; Index < Info.SelectItems.Num(); ++Index) {
				auto& Item = Info.SelectItems[Index];
				if (!IsValid(Item.Actor)) Item.Actor = nullptr;
				FUndoSelect Current;
				Current.Actor = Index == 0 ? System->Selection->AnchorActor : Index == 1 ? System->Selection->TargetActor : Item.Actor;
				Current.Select = IsValid(Current.Actor) && System->Selection->Contains(Current.Actor);
				Inverse.SelectItems.Add(Current);
			}
		} else {
			Info.SelectItems.Empty();
		}
		if (Inverse.TransformActors.IsEmpty() && Inverse.TransformComponents.IsEmpty() && Inverse.ColorSlotItems.IsEmpty() && Inverse.Lightweights.IsEmpty() && Inverse.SelectItems.IsEmpty()) continue;
		if (To.Num() == MAXUNDO) To.RemoveAt(0);
		To.Add(MoveTemp(Inverse));
		return true;
	}
	return false;
}

bool UHyperManageUndo::PopUndo(FUndoInfo& Info) { return Transfer(UndoStack, RedoStack, Info); }
bool UHyperManageUndo::PopRedo(FUndoInfo& Info) { return Transfer(RedoStack, UndoStack, Info); }

void UHyperManageUndo::PushUndoTransforms(TArray<AActor*>& Actors)
{
	FUndoInfo Info;
	for (auto* Actor : Actors) {
		if (!IsValid(Actor)) continue;
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
			if (!Proxy->IsAvailable() || Proxy->IsPending()) continue;
			FUndoLightweight Item; Item.Proxy = Proxy; Item.Transform = Proxy->GetActorTransform(); Info.Lightweights.Add(Item);
			continue;
		}
		for (auto* Component : TInlineComponentArray<USceneComponent*>(Actor)) {
			if (!IsValid(Component) || Component->GetAttachParent()) continue;
			if (Component == Actor->GetRootComponent()) {
				FUndoTransformActor Item; Item.Actor = Actor; Item.Transform = Component->GetComponentTransform(); Info.TransformActors.Add(Item);
			} else {
				FUndoTransformComponent Item; Item.Component = Component; Item.Transform = Component->GetComponentTransform(); Info.TransformComponents.Add(Item);
			}
		}
	}
	Push(MoveTemp(Info));
}

void UHyperManageUndo::PushUndoColorSlot(TArray<AActor*>& Actors)
{
	FUndoInfo Info;
	for (auto* Actor : Actors) {
		if (!IsValid(Actor)) continue;
		if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
			if (!Proxy->IsAvailable() || Proxy->IsPending()) continue;
			FUndoLightweight Item; Item.Proxy = Proxy; Item.Paint = true; Item.Customization = Proxy->Customization; Info.Lightweights.Add(Item);
		} else if (auto* Buildable = Cast<AFGBuildable>(Actor)) {
			FUndoColorSlot Item; Item.Buildable = Buildable; Item.CustomizationData = Buildable->GetCustomizationData_Implementation(); Info.ColorSlotItems.Add(Item);
		}
	}
	Push(MoveTemp(Info));
}

void UHyperManageUndo::PushUndoSelection(TArray<AActor*>& Actors)
{
	if (!System || !System->Selection) return;
	FUndoInfo Info;
	auto Add = [&](AActor* Actor) {
		FUndoSelect Item; Item.Actor = IsValid(Actor) ? Actor : nullptr;
		Item.Select = Item.Actor && System->Selection->Contains(Item.Actor); Info.SelectItems.Add(Item);
	};
	Add(System->Selection->AnchorActor);
	Add(System->Selection->TargetActor);
	for (auto* Actor : Actors) Add(Actor);
	Push(MoveTemp(Info));
}