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
 const bool Supported = CanPersist(Slot.Actors) && CanPersist({Slot.Anchor, Slot.Target});
 Slots[Index] = Supported ? Slot : FHyperManageStoredSlot();
 Slots[Index].Name = Slot.Name;
 return Supported;
}
