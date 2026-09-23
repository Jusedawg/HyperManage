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
