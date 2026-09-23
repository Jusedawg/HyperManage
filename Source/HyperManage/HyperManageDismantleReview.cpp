#include "HyperManageDismantleReview.h"
#include "Engine/World.h"
#include "FGPlayerState.h"

FHyperManageDismantleReview FHyperManageDismantleReviewer::Build(
 UWorld* World, const TArray<AActor*>& Selection, AActor* Target, const AFGPlayerState* Player)
{
 if (!IsValid(Player) || Player->GetWorld() != World)
 {
  FHyperManageDismantleReview Result;
  Result.Error = TEXT("The local player is unavailable. Re-equip the tool and retry.");
  return Result;
 }
 return BuildWithSources(World, Selection, Target,
  [&](const TArray<AActor*>& Actors) { return FHyperManageDismantlePlanner::Build(World, Actors, Target); },
  [&](const FHyperManageDismantlePlan& Plan) { return FHyperManageDismantleRefunds::Build(World, Plan, Player); },
  [&](const TArray<FHyperManageLightweightRef>& Refs) { return FHyperManageDismantleRefunds::BuildLightweights(World, Refs, Player); });
}

FHyperManageDismantleReview FHyperManageDismantleReviewer::BuildWithSources(UWorld* World,
 const TArray<AActor*>& Selection, AActor* Target, FPlan PlanNative, FNative ReadNative, FLightweight ReadLightweight)
{
 auto Fail = [](const TCHAR* Error)
 {
  FHyperManageDismantleReview Result; Result.Error = Error; return Result;
 };
 if (!IsValid(World) || World->GetNetMode() == NM_Client) return Fail(TEXT("Refund review requires the authority world."));
 if (Selection.IsEmpty()) return Fail(TEXT("Select buildings to review. The target is excluded."));
 TArray<AActor*> Actors;
 TArray<AHyperManageLightweightProxy*> Proxies;
 TArray<FHyperManageLightweightRef> Refs;
 TSet<AActor*> Seen;
 for (AActor* Actor : Selection)
 {
  if (!IsValid(Actor) || Actor->GetWorld() != World) return Fail(TEXT("A selected building is unavailable. Reselect and retry."));
  if (Actor == Target) return Fail(TEXT("The target is protected and cannot be part of this review."));
  if (Seen.Contains(Actor)) continue;
  Seen.Add(Actor);
  if (Seen.Num() > FHyperManageDismantlePlanner::MaxActors) return Fail(TEXT("Review supports at most 1024 buildings, including children."));
  if (auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor))
  {
   if (!Proxy->Available || Proxy->IsPending()) return Fail(TEXT("Wait for building edits to finish, then review again."));
   if (const auto* TargetProxy = Cast<AHyperManageLightweightProxy>(Target); IsValid(TargetProxy)
    && Proxy->Ref.BuildableClass == TargetProxy->Ref.BuildableClass && Proxy->Ref.Index == TargetProxy->Ref.Index)
    return Fail(TEXT("A selected instance is the protected target."));
   Proxies.Add(Proxy); Refs.Add(Proxy->Ref);
  }
  else Actors.Add(Actor);
 }
 FHyperManageDismantlePlan Plan;
 if (!Actors.IsEmpty())
 {
  Plan = PlanNative(Actors);
  if (Plan.Status != EHyperManageDismantlePlanStatus::Ready)
  {
   if (Plan.Status == EHyperManageDismantlePlanStatus::MissingDependency)
    return Fail(TEXT("A building requires an unselected dependency. Select its related buildings and retry; nothing was added automatically."));
   if (Plan.Status == EHyperManageDismantlePlanStatus::ProtectedTarget)
    return Fail(TEXT("A child or dependency is the protected target. Change the target or selection and retry."));
   return Fail(TEXT("The full building group could not be reviewed. It contains unavailable, unsupported or cyclic dependencies."));
  }
 }
 if (Plan.OrderedActors.Num() + Refs.Num() > FHyperManageDismantlePlanner::MaxActors)
  return Fail(TEXT("Review supports at most 1024 buildings, including children."));
 FHyperManageRefundPreview Native, Lightweight;
 if (!Actors.IsEmpty())
 {
  Native = ReadNative(Plan);
  if (Native.Status != EHyperManageRefundStatus::Ready) return Fail(TEXT("A building's refund could not be read. No partial estimate is shown."));
 }
 if (!Refs.IsEmpty())
 {
  Lightweight = ReadLightweight(Refs);
  if (Lightweight.Status != EHyperManageRefundStatus::Ready) return Fail(TEXT("A lightweight building changed or its refund is unavailable. Reselect and retry."));
 }
 if (!Actors.IsEmpty() && !Refs.IsEmpty() && Native.NoBuildCost != Lightweight.NoBuildCost)
  return Fail(TEXT("The build-cost rule changed during review. Please retry."));
 int32 StackCount = 0;
 for (const auto& Entry : Native.Actors) StackCount += Entry.Stacks.Num();
 for (const auto& Entry : Lightweight.Instances) StackCount += Entry.Stacks.Num();
 if (StackCount > FHyperManageDismantleRefunds::MaxStacks) return Fail(TEXT("This group has too many refund stacks. Review a smaller selection."));
 for (const auto& Ref : Plan.OrderedActors)
  if (!Ref.IsValid() || Ref->GetWorld() != World) return Fail(TEXT("A building changed during review. Please retry."));
 for (int32 Index = 0; Index < Proxies.Num(); ++Index)
 {
  const auto* Proxy = Proxies[Index];
  const auto& Before = Refs[Index];
  if (!IsValid(Proxy) || Proxy->IsPending() || !Proxy->Available || Proxy->Ref.Index != Before.Index
   || Proxy->Ref.BuildableClass != Before.BuildableClass || Proxy->Ref.Recipe != Before.Recipe
   || !Proxy->Ref.ExpectedTransform.Equals(Before.ExpectedTransform, 0.01)) return Fail(TEXT("A selected instance changed during review. Please retry."));
 }
 FHyperManageDismantleReview Result;
 Result.AddedChildren = Plan.AddedChildren;
 Result.Refunds.Status = EHyperManageRefundStatus::Ready;
 Result.Refunds.NoBuildCost = !Actors.IsEmpty() ? Native.NoBuildCost : Lightweight.NoBuildCost;
 Result.Refunds.Actors = MoveTemp(Native.Actors);
 Result.Refunds.Instances = MoveTemp(Lightweight.Instances);
 return Result;
}
