#include "HyperManageSlotStore.h"
#include "HyperManageLightweight.h"
#include "Subsystem/SubsystemActorManager.h"
#include "Buildables/FGBuildable.h"

AHyperManageSlotStore::AHyperManageSlotStore()
{
 ReplicationPolicy = ESubsystemReplicationPolicy::SpawnOnServer;
 Slots.SetNum(10);
}

AHyperManageSlotStore* AHyperManageSlotStore::Get(UWorld* World)
{
 if (!World || World->GetNetMode() != NM_Standalone) return nullptr;
 auto* Manager = World->GetSubsystem<USubsystemActorManager>();
 return Manager ? Manager->GetSubsystemActor<AHyperManageSlotStore>() : nullptr;
}

bool AHyperManageSlotStore::CanPersist(const TArray<TObjectPtr<AActor>>& Actors)
{
 for (const auto& Actor : Actors) {
  if (!IsValid(Actor)) continue;
  if (Actor->IsA<AHyperManageLightweightProxy>() || Actor->HasAnyFlags(RF_Transient) || !Actor->Implements<UFGSaveInterface>()) return false;
  if (const auto* Buildable = Cast<AFGBuildable>(Actor); Buildable && Buildable->GetIsLightweightTemporary()) return false;
 }
 return true;
}

bool AHyperManageSlotStore::Store(int32 Index, const FHyperManageStoredSlot& Slot)
{
 if (Index < 0 || Index >= 10) return false;
 if (Slots.Num() != 10) Slots.SetNum(10);
 const auto InWorld = [this](AActor* Actor) { return !IsValid(Actor) || Actor->GetWorld() == GetWorld(); };
 bool Supported = CanPersist(Slot.Actors) && CanPersist({Slot.Anchor, Slot.Target}) && InWorld(Slot.Anchor) && InWorld(Slot.Target);
 for (const auto& Actor : Slot.Actors) Supported &= InWorld(Actor);
 if (Slot.BlueprintSlot) Supported = IsValid(Slot.Blueprint) && InWorld(Slot.Blueprint) && !Slot.Blueprint->HasAnyFlags(RF_Transient);
 Slots[Index] = Supported ? Slot : FHyperManageStoredSlot();
 Slots[Index].Name = Slot.Name;
 SanitizeSlots();
 return Supported;
}

void AHyperManageSlotStore::SanitizeSlots()
{
 Slots.SetNum(10);
 for (auto& Slot : Slots) {
  Slot.Name = Slot.Name.TrimStartAndEnd().Left(24);
  for (TCHAR& Character : Slot.Name) if (FChar::IsControl(Character)) Character = TEXT(' ');
  if (!Slot.Occupied) { Slot.Actors.Empty(); Slot.Anchor = nullptr; Slot.Target = nullptr; Slot.Blueprint = nullptr; Slot.BlueprintSlot = false; continue; }
  if (Slot.BlueprintSlot) {
   Slot.Actors.Empty(); Slot.Anchor = nullptr; Slot.Target = nullptr;
   if (!IsValid(Slot.Blueprint) || Slot.Blueprint->GetWorld() != GetWorld()) Slot.Blueprint = nullptr;
   continue;
  }
  Slot.Blueprint = nullptr;
  TArray<TObjectPtr<AActor>> Members;
  for (const auto& Actor : Slot.Actors) {
   if (IsValid(Actor) && Actor->GetWorld() == GetWorld() && CanPersist({Actor})) Members.AddUnique(Actor);
  }
  Slot.Actors = MoveTemp(Members);
  if (!IsValid(Slot.Anchor) || !Slot.Actors.Contains(Slot.Anchor)) Slot.Anchor = nullptr;
  if (!IsValid(Slot.Target) || !Slot.Actors.Contains(Slot.Target) || Slot.Target == Slot.Anchor) Slot.Target = nullptr;
 }
}
