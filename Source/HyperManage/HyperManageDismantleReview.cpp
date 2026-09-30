#include "HyperManageDismantleReview.h"
#include "Buildables/FGBuildable.h"
#include "Engine/World.h"
#include "FGPlayerState.h"
#include "FGDismantleInterface.h"
#include "FGConstructDisqualifier.h"
#include "Resources/FGItemDescriptor.h"

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
  [&](const TArray<FHyperManageLightweightRef>& Refs) { return FHyperManageDismantleRefunds::BuildLightweights(World, Refs, Player); },
  [](AActor* Actor, const TArray<AActor*>& Group, FHyperManageDismantleEligibility& Out)
  {
   if (!IsValid(Actor) || !Actor->Implements<UFGDismantleInterface>()) return false;
   Out.CanDismantle = IFGDismantleInterface::Execute_CanDismantle(Actor);
   if (!IsValid(Actor)) return false;
   if (IFGDismantleInterface::Execute_SupportsDismantleDisqualifiers(Actor))
   {
    TArray<TSubclassOf<UFGConstructDisqualifier>> Disqualifiers;
    IFGDismantleInterface::Execute_GetDismantleDisqualifiers(Actor, Disqualifiers, Group);
    for (const auto& Type : Disqualifiers)
    {
     const FString Reason = IsValid(Type.Get()) ? UFGConstructDisqualifier::GetDisqualifyingText(Type).ToString() : FString();
     Out.Reasons.AddUnique(Reason.IsEmpty() ? TEXT("The game reported an unspecified dismantle warning.") : Reason);
    }
   }
   return IsValid(Actor);
  });
}

FString FHyperManageDismantleReviewer::FormatRefundBreakdown(const FHyperManageRefundPreview& Preview)
{
 if (Preview.Status != EHyperManageRefundStatus::Ready) return FString();
 FString Report = TEXT("\n\nPER-BUILDING REFUNDS\nItem types are grouped within each building for display only.\n");
 auto Append = [&Report](const FString& Building, const TArray<FInventoryStack>& Stacks) {
  Report += TEXT("\n") + Building + TEXT("\n");
  TMap<TSubclassOf<UFGItemDescriptor>, int64> Totals;
  for (const auto& Stack : Stacks) if (Stack.NumItems > 0) Totals.FindOrAdd(Stack.Item.GetItemClass()) += Stack.NumItems;
  TArray<FString> Lines;
  for (const auto& Entry : Totals) {
   const FText Name = IsValid(Entry.Key.Get()) ? UFGItemDescriptor::GetItemName(Entry.Key) : FText();
   Lines.Add(FString::Printf(TEXT("  %s: %lld"), Name.IsEmpty() ? TEXT("Item") : *Name.ToString(), Entry.Value));
  }
  Lines.Sort();
  for (const auto& Line : Lines) Report += Line + TEXT("\n");
  if (Lines.IsEmpty()) Report += TEXT("  No refundable items reported.\n");
 };
 for (const auto& Entry : Preview.Actors) Append(TEXT("Standard: ") + DescribeBuilding(Entry.Actor), Entry.Stacks);
 for (const auto& Entry : Preview.Instances) {
  const auto* Building = Entry.Ref.BuildableClass.GetDefaultObject();
  const FString Name = Building && !Building->mDisplayName.IsEmpty() ? Building->mDisplayName.ToString() : TEXT("Building");
  const FVector Position = Entry.Ref.ExpectedTransform.GetLocation() / 100.0;
  Append(FString::Printf(TEXT("Lightweight: %s (world m: X %.1f, Y %.1f, Z %.1f)"), *Name, Position.X, Position.Y, Position.Z), Entry.Stacks);
 }
 return Report;
}

FString FHyperManageDismantleReviewer::DescribeBuilding(const TWeakObjectPtr<AActor>& Actor)
{
 if (!Actor.IsValid()) return TEXT("Unavailable building");
 const auto* Buildable = Cast<AFGBuildable>(Actor.Get());
 const FString Name = Buildable && !Buildable->mDisplayName.IsEmpty() ? Buildable->mDisplayName.ToString() : TEXT("Building");
 const FVector Position = Actor->GetActorLocation() / 100.0;
 return FString::Printf(TEXT("%s (world m: X %.1f, Y %.1f, Z %.1f)"), *Name, Position.X, Position.Y, Position.Z);
}

FString FHyperManageDismantleReviewer::DescribePlanFailure(const FHyperManageDismantlePlan& Plan)
{
 switch (Plan.Status)
 {
  case EHyperManageDismantlePlanStatus::MissingDependency:
   return FString::Printf(TEXT("A building requires an unselected dependency.\nBuilding: %s\nRequired: %s\n\nClose the panel, select the required building, then refresh. Nothing was added automatically."), *DescribeBuilding(Plan.ProblemActor), *DescribeBuilding(Plan.RequiredActor));
  case EHyperManageDismantlePlanStatus::ProtectedTarget:
   return FString::Printf(TEXT("A child or dependency is the protected target.\nProtected: %s\n\nChange the target or selection, then refresh."), *DescribeBuilding(Plan.RequiredActor.IsValid() ? Plan.RequiredActor : Plan.ProblemActor));
  case EHyperManageDismantlePlanStatus::TooManyActors:
   return TEXT("The group exceeds 1,024 buildings after including children. Review a smaller selection.");
  case EHyperManageDismantlePlanStatus::DependencyCycle:
   return TEXT("The group contains a dependency cycle and cannot be safely ordered. Review a smaller independent group; if the cycle remains, this group is unsupported.");
  case EHyperManageDismantlePlanStatus::UnsupportedActor:
   return FString::Printf(TEXT("This building does not support the current review path:\n%s\n\nExclude it and refresh. No partial estimate is shown."), *DescribeBuilding(Plan.ProblemActor));
  case EHyperManageDismantlePlanStatus::InvalidActor:
   return TEXT("A building or related dependency is unavailable. Reselect the group and refresh. No partial estimate is shown.");
  default:
   return TEXT("The full building group could not be reviewed. Reselect the group and refresh.");
 }
}

FHyperManageDismantleReview FHyperManageDismantleReviewer::BuildWithSources(UWorld* World,
 const TArray<AActor*>& Selection, AActor* Target, FPlan PlanNative, FNative ReadNative, FLightweight ReadLightweight, FEligibility ReadEligibility)
{
 auto Fail = [](const FString& Error)
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
  if (Plan.Status != EHyperManageDismantlePlanStatus::Ready) return Fail(DescribePlanFailure(Plan));
 }
 if (Plan.OrderedActors.Num() + Refs.Num() > FHyperManageDismantlePlanner::MaxActors)
  return Fail(TEXT("Review supports at most 1024 buildings, including children."));
 FHyperManageDismantleReview Result;
 TArray<AActor*> NativeGroup;
 for (const auto& Ref : Plan.OrderedActors)
 {
  if (!Ref.IsValid() || Ref->GetWorld() != World) return Fail(TEXT("A building changed before eligibility review. Please retry."));
  NativeGroup.Add(Ref.Get());
  if (!Seen.Contains(Ref.Get())) Result.AddedChildDetails.Add(DescribeBuilding(Ref));
 }
 for (AActor* Actor : NativeGroup)
 {
  if (!IsValid(Actor)) return Fail(TEXT("A building changed during eligibility review. Please retry."));
  FHyperManageDismantleEligibility Eligibility;
  if (!ReadEligibility(Actor, NativeGroup, Eligibility)) return Fail(TEXT("A building's dismantle eligibility could not be read. Please retry."));
  ++Result.NativeChecked;
  if (!Eligibility.CanDismantle) ++Result.NativeBlocked;
  if (!Eligibility.Reasons.IsEmpty()) ++Result.NativeWarnings;
  for (const auto& Reason : Eligibility.Reasons) Result.EligibilityReasons.AddUnique(Reason);
  if (!Eligibility.CanDismantle || !Eligibility.Reasons.IsEmpty())
  {
   FString Detail = FString::Printf(TEXT("%s: %s"), Eligibility.CanDismantle ? TEXT("Warning (advisory)") : TEXT("Refuses dismantling"), *DescribeBuilding(Actor));
   TArray<FString> Reasons;
   for (const auto& Reason : Eligibility.Reasons) if (!Reason.IsEmpty()) Reasons.AddUnique(Reason);
   for (const auto& Reason : Reasons) Detail += TEXT("\n  ") + Reason;
   if (!Eligibility.CanDismantle && Reasons.IsEmpty()) Detail += TEXT("\n  The game did not provide a reason.");
   Result.EligibilityDetails.Add(MoveTemp(Detail));
  }
 }
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
 Result.AddedChildren = Plan.AddedChildren;
 Result.Refunds.Status = EHyperManageRefundStatus::Ready;
 Result.Refunds.NoBuildCost = !Actors.IsEmpty() ? Native.NoBuildCost : Lightweight.NoBuildCost;
 Result.Refunds.Actors = MoveTemp(Native.Actors);
 Result.Refunds.Instances = MoveTemp(Lightweight.Instances);
 return Result;
}
