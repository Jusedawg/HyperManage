#include "HyperManageDismantleRefund.h"
#include "Engine/World.h"
#include "FGPlayerState.h"
#include "FGDismantleInterface.h"
#include "Buildables/FGBuildable.h"
#include "HyperManageLightweight.h"

FHyperManageRefundPreview FHyperManageDismantleRefunds::Build(UWorld* World, const FHyperManageDismantlePlan& Plan, const AFGPlayerState* Player)
{
 if (!IsValid(Player) || Player->GetWorld() != World)
 {
  FHyperManageRefundPreview Result;
  Result.Status = EHyperManageRefundStatus::InvalidPlayer;
  return Result;
 }
 return BuildWithReader(World, Plan, Player->GetPlayerRules().NoBuildCost, [](AActor* Actor, bool NoBuildCost, TArray<FInventoryStack>& Stacks)
 {
  if (Actor->IsA<AHyperManageLightweightProxy>() || !Actor->Implements<UFGDismantleInterface>()) return false;
  if (const auto* Buildable = Cast<AFGBuildable>(Actor); Buildable && Buildable->GetIsLightweightTemporary()) return false;
  IFGDismantleInterface::Execute_GetDismantleRefund(Actor, Stacks, NoBuildCost);
  return true;
 });
}

FHyperManageRefundPreview FHyperManageDismantleRefunds::BuildWithReader(
 UWorld* World, const FHyperManageDismantlePlan& Plan, bool NoBuildCost, FReadRefund Read)
{
 FHyperManageRefundPreview Result;
 Result.NoBuildCost = NoBuildCost;
 auto Fail = [&Result](EHyperManageRefundStatus Status, AActor* Actor = nullptr)
 {
  Result.Status = Status;
  Result.ProblemActor = Actor;
  Result.Actors.Empty();
  return Result;
 };
 if (!IsValid(World) || World->GetNetMode() == NM_Client) return Fail(EHyperManageRefundStatus::NotAuthority);
 if (Plan.Status != EHyperManageDismantlePlanStatus::Ready || Plan.OrderedActors.IsEmpty()
  || Plan.OrderedActors.Num() > FHyperManageDismantlePlanner::MaxActors) return Fail(EHyperManageRefundStatus::InvalidPlan);
 TSet<AActor*> Seen;
 // Validate every actor before asking any building for its refund. Duplicate actors
 // are an invalid plan, not a second payout and not a silently corrected request.
 for (const auto& Ref : Plan.OrderedActors)
 {
  AActor* Actor = Ref.Get();
  if (!IsValid(Actor) || Actor->GetWorld() != World) return Fail(EHyperManageRefundStatus::InvalidActor, Actor);
  if (Seen.Contains(Actor)) return Fail(EHyperManageRefundStatus::InvalidPlan, Actor);
  Seen.Add(Actor);
 }
 int32 StackCount = 0;
 for (const auto& Ref : Plan.OrderedActors)
 {
  AActor* Actor = Ref.Get();
  if (!IsValid(Actor) || Actor->GetWorld() != World) return Fail(EHyperManageRefundStatus::InvalidActor, Actor);
  TArray<FInventoryStack> Stacks;
  if (!Read(Actor, NoBuildCost, Stacks)) return Fail(EHyperManageRefundStatus::UnsupportedActor, Actor);
  if (Stacks.Num() > MaxStacks - StackCount) return Fail(EHyperManageRefundStatus::TooManyStacks, Actor);
  StackCount += Stacks.Num();
  for (const FInventoryStack& Stack : Stacks)
  {
   // Empty entries are permitted by inventory APIs. Negative quantities and positive
   // entries without a descriptor are rejected rather than silently losing refunds.
   if (Stack.NumItems < 0 || (Stack.NumItems > 0 && !IsValid(Stack.Item.GetItemClass().Get())))
    return Fail(EHyperManageRefundStatus::InvalidStack, Actor);
  }
  Stacks.RemoveAll([](const FInventoryStack& Stack) { return Stack.NumItems == 0; });
  FHyperManageActorRefund& Entry = Result.Actors.AddDefaulted_GetRef();
  Entry.Actor = Actor;
  Entry.Stacks = MoveTemp(Stacks); // Never aggregate by descriptor: item states can differ.
 }
 for (const auto& Ref : Plan.OrderedActors)
  if (!Ref.IsValid() || Ref->GetWorld() != World) return Fail(EHyperManageRefundStatus::InvalidActor, Ref.Get());
 Result.Status = EHyperManageRefundStatus::Ready;
 return Result;
}

FHyperManageRefundPreview FHyperManageDismantleRefunds::BuildLightweights(
 UWorld* World, const TArray<FHyperManageLightweightRef>& Refs, const AFGPlayerState* Player)
{
 if (!IsValid(Player) || Player->GetWorld() != World)
 {
  FHyperManageRefundPreview Result;
  Result.Status = EHyperManageRefundStatus::InvalidPlayer;
  return Result;
 }
 auto* Subsystem = IsValid(World) ? AFGLightweightBuildableSubsystem::Get(World) : nullptr;
 return BuildLightweightsWithReader(World, Refs, Player->GetPlayerRules().NoBuildCost,
  [Subsystem](const FHyperManageLightweightRef& Ref) -> const FRuntimeBuildableInstanceData*
  {
   return IsValid(Subsystem) ? Subsystem->GetRuntimeDataForBuildableClassAndIndex(Ref.BuildableClass, Ref.Index) : nullptr;
  }, [World](const FHyperManageLightweightRef& Ref, const FRuntimeBuildableInstanceData& Data, TArray<FInventoryStack>& Stacks)
  {
   Ref.BuildableClass.GetDefaultObject()->GetLightweightBuildableDismantleRefundReturns(World, Data.BuiltWithRecipe, Data.TypeSpecificData, Stacks);
  });
}

FHyperManageRefundPreview FHyperManageDismantleRefunds::BuildLightweightsWithReader(UWorld* World,
 const TArray<FHyperManageLightweightRef>& Refs, bool NoBuildCost, FResolveInstance Resolve, FReadInstanceRefund Read)
{
 FHyperManageRefundPreview Result;
 Result.NoBuildCost = NoBuildCost;
 auto Fail = [&Result](EHyperManageRefundStatus Status)
 {
  Result.Status = Status;
  Result.Instances.Empty();
  return Result;
 };
 if (!IsValid(World) || World->GetNetMode() == NM_Client) return Fail(EHyperManageRefundStatus::NotAuthority);
 if (Refs.IsEmpty() || Refs.Num() > FHyperManageDismantlePlanner::MaxActors) return Fail(EHyperManageRefundStatus::InvalidPlan);
 TMap<UClass*, TSet<int32>> Seen;
 for (const auto& Ref : Refs)
 {
  if (!IsValid(Ref.BuildableClass.Get()) || Ref.Index < 0 || !Ref.Matches(Resolve(Ref))) return Fail(EHyperManageRefundStatus::InvalidInstance);
  auto& Indices = Seen.FindOrAdd(Ref.BuildableClass.Get());
  if (Indices.Contains(Ref.Index)) return Fail(EHyperManageRefundStatus::InvalidPlan);
  Indices.Add(Ref.Index);
 }
 int32 StackCount = 0;
 for (const auto& Ref : Refs)
 {
  const auto* Data = Resolve(Ref);
  if (!Ref.Matches(Data)) return Fail(EHyperManageRefundStatus::InvalidInstance);
  TArray<FInventoryStack> Stacks;
  // Lightweight records have construction costs, not actor inventories. Do not grant
  // construction materials under the initiating player's no-build-cost rule.
  if (!NoBuildCost) Read(Ref, *Data, Stacks);
  if (Stacks.Num() > MaxStacks - StackCount) return Fail(EHyperManageRefundStatus::TooManyStacks);
  StackCount += Stacks.Num();
  for (const auto& Stack : Stacks)
   if (Stack.NumItems < 0 || (Stack.NumItems > 0 && !IsValid(Stack.Item.GetItemClass().Get()))) return Fail(EHyperManageRefundStatus::InvalidStack);
  Stacks.RemoveAll([](const FInventoryStack& Stack) { return Stack.NumItems == 0; });
  auto& Entry = Result.Instances.AddDefaulted_GetRef();
  Entry.Ref = Ref;
  Entry.Stacks = MoveTemp(Stacks);
 }
 // Recheck all records after refund callbacks; never expose a partial stale preview.
 for (const auto& Ref : Refs) if (!Ref.Matches(Resolve(Ref))) return Fail(EHyperManageRefundStatus::InvalidInstance);
 Result.Status = EHyperManageRefundStatus::Ready;
 return Result;
}
