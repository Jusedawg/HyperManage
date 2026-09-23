#include "HyperManageRefundCapacity.h"
#include "Engine/World.h"
#include "FGPlayerState.h"
#include "FGCharacterPlayer.h"
#include "Resources/FGItemDescriptor.h"

EHyperManageRefundCapacity FHyperManageRefundCapacity::Check(UWorld* World, const FHyperManageRefundPreview& Preview, const AFGPlayerState* Player)
{
 if (!IsValid(World) || World->GetNetMode() == NM_Client || !IsValid(Player) || Player->GetWorld() != World)
  return EHyperManageRefundCapacity::Unavailable;
 const auto* Character = Cast<AFGCharacterPlayer>(Player->GetPawn());
 if (!IsValid(Character) || Character->GetWorld() != World || Character->GetPlayerState() != Player)
  return EHyperManageRefundCapacity::Unavailable;
 auto* Inventory = Character->GetInventory();
 if (!IsValid(Inventory) || Inventory->GetWorld() != World) return EHyperManageRefundCapacity::Unavailable;
 const auto Result = CheckWithReader(Preview, [Inventory](const TArray<FInventoryStack>& Stacks) {
  return Inventory->HasEnoughSpaceForStacks(Stacks);
 });
 return IsValid(Inventory) && IsValid(Character) && Character->GetInventory() == Inventory ? Result : EHyperManageRefundCapacity::Unavailable;
}

EHyperManageRefundCapacity FHyperManageRefundCapacity::CheckWithReader(const FHyperManageRefundPreview& Preview, FCheckBatch CheckBatch)
{
 if (Preview.Status != EHyperManageRefundStatus::Ready) return EHyperManageRefundCapacity::Unavailable;
 int64 Entries = 0;
 for (const auto& Entry : Preview.Actors) Entries += Entry.Stacks.Num();
 for (const auto& Entry : Preview.Instances) Entries += Entry.Stacks.Num();
 if (Entries > MaxCheckStacks) return EHyperManageRefundCapacity::TooLarge;
 TArray<FInventoryStack> Batch;
 int64 Items = 0;
 auto Append = [&](const TArray<FInventoryStack>& Stacks) -> EHyperManageRefundCapacity
 {
  for (const auto& Stack : Stacks)
  {
   if (Stack.NumItems < 0 || (Stack.NumItems > 0 && !IsValid(Stack.Item.GetItemClass().Get()))) return EHyperManageRefundCapacity::Unavailable;
   if (Stack.NumItems == 0) continue;
   Items += Stack.NumItems;
   if (Items > MaxCheckItems) return EHyperManageRefundCapacity::TooLarge;
   Batch.Add(Stack); // Preserve item state; display totals are unsuitable for capacity checks.
  }
  return EHyperManageRefundCapacity::Fits;
 };
 for (const auto& Entry : Preview.Actors) {
  const auto Status = Append(Entry.Stacks);
  if (Status != EHyperManageRefundCapacity::Fits) return Status;
 }
 for (const auto& Entry : Preview.Instances) {
  const auto Status = Append(Entry.Stacks);
  if (Status != EHyperManageRefundCapacity::Fits) return Status;
 }
 if (Batch.IsEmpty()) return EHyperManageRefundCapacity::NoRefund;
 // One whole-batch query avoids counting the same free slots several times.
 return CheckBatch(Batch) ? EHyperManageRefundCapacity::Fits : EHyperManageRefundCapacity::NeedsOverflow;
}
