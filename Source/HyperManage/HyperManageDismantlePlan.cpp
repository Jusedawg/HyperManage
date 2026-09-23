#include "HyperManageDismantlePlan.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "FGDismantleInterface.h"
#include "HyperManageLightweight.h"

FHyperManageDismantlePlan FHyperManageDismantlePlanner::Build(UWorld* World, const TArray<AActor*>& Selection, AActor* ProtectedTarget)
{
 return BuildWithResolver(World, Selection, ProtectedTarget, [](AActor* Actor, FLinks& Links)
 {
  if (Actor->IsA<AHyperManageLightweightProxy>() || !Actor->Implements<UFGDismantleInterface>()) return false;
  if (const auto* Buildable = Cast<AFGBuildable>(Actor); Buildable && Buildable->GetIsLightweightTemporary()) return false;
  IFGDismantleInterface::Execute_GetChildDismantleActors(Actor, Links.Children);
  IFGDismantleInterface::Execute_GetDismantleDependencies(Actor, Links.Dependencies);
  return true;
 });
}

FHyperManageDismantlePlan FHyperManageDismantlePlanner::BuildWithResolver(
 UWorld* World, const TArray<AActor*>& Selection, AActor* ProtectedTarget, FResolve Resolve)
{
 FHyperManageDismantlePlan Result;
 if (!IsValid(World) || World->GetNetMode() == NM_Client)
 {
  Result.Status = EHyperManageDismantlePlanStatus::NotAuthority;
  return Result;
 }
 auto Fail = [&Result](EHyperManageDismantlePlanStatus Status, AActor* Actor, AActor* Required = nullptr)
 {
  Result.Status = Status;
  Result.ProblemActor = Actor;
  Result.RequiredActor = Required;
  Result.OrderedActors.Empty();
  Result.AddedChildren = 0;
  return Result;
 };
 TArray<AActor*> Actors;
 TMap<AActor*, int32> Indices;
 auto Add = [&](AActor* Actor) -> EHyperManageDismantlePlanStatus
 {
  if (!IsValid(Actor) || Actor->GetWorld() != World) return EHyperManageDismantlePlanStatus::InvalidActor;
  if (Actor == ProtectedTarget) return EHyperManageDismantlePlanStatus::ProtectedTarget;
  if (Indices.Contains(Actor)) return EHyperManageDismantlePlanStatus::Ready;
  if (Actors.Num() >= MaxActors) return EHyperManageDismantlePlanStatus::TooManyActors;
  Indices.Add(Actor, Actors.Add(Actor));
  return EHyperManageDismantlePlanStatus::Ready;
 };
 for (AActor* Actor : Selection)
 {
  const auto Status = Add(Actor);
  if (Status != EHyperManageDismantlePlanStatus::Ready) return Fail(Status, Actor);
 }
 if (Actors.IsEmpty()) return Result;
 const int32 SelectedCount = Actors.Num();
 TArray<FLinks> Graph;
 // Expand children only. Dependencies outside this closure must be explicitly selected;
 // silently pulling in a train or other connected building would widen a destructive action.
 for (int32 Index = 0; Index < Actors.Num(); ++Index)
 {
  AActor* Actor = Actors[Index];
  if (!IsValid(Actor) || Actor->GetWorld() != World) return Fail(EHyperManageDismantlePlanStatus::InvalidActor, Actor);
  FLinks Links;
  if (!Resolve(Actor, Links)) return Fail(EHyperManageDismantlePlanStatus::UnsupportedActor, Actor);
  for (AActor* Child : Links.Children)
  {
   const auto Status = Add(Child);
   if (Status != EHyperManageDismantlePlanStatus::Ready) return Fail(Status, Child);
  }
  Graph.Add(MoveTemp(Links));
 }
 TArray<TSet<int32>> Prerequisites;
 Prerequisites.SetNum(Actors.Num());
 for (int32 Index = 0; Index < Actors.Num(); ++Index)
 {
  // Recheck after interface callbacks before exposing any plan.
  if (!IsValid(Actors[Index]) || Actors[Index]->GetWorld() != World) return Fail(EHyperManageDismantlePlanStatus::InvalidActor, Actors[Index]);
  auto Require = [&](AActor* Required) -> EHyperManageDismantlePlanStatus
  {
   if (!IsValid(Required) || Required->GetWorld() != World) return EHyperManageDismantlePlanStatus::InvalidActor;
   if (Required == ProtectedTarget) return EHyperManageDismantlePlanStatus::ProtectedTarget;
   const int32* RequiredIndex = Indices.Find(Required);
   if (!RequiredIndex) return EHyperManageDismantlePlanStatus::MissingDependency;
   Prerequisites[Index].Add(*RequiredIndex);
   return EHyperManageDismantlePlanStatus::Ready;
  };
  for (AActor* Child : Graph[Index].Children)
  {
   const auto Status = Require(Child);
   if (Status != EHyperManageDismantlePlanStatus::Ready) return Fail(Status, Actors[Index], Child);
  }
  for (AActor* Dependency : Graph[Index].Dependencies)
  {
   const auto Status = Require(Dependency);
   if (Status != EHyperManageDismantlePlanStatus::Ready) return Fail(Status, Actors[Index], Dependency);
  }
 }
 // Stable iterative ordering also bounds stack use for long chains of child actors.
 TSet<int32> Ordered;
 while (Ordered.Num() < Actors.Num())
 {
  bool Progress = false;
  for (int32 Index = 0; Index < Actors.Num(); ++Index)
  {
   if (Ordered.Contains(Index)) continue;
   bool Ready = true;
   for (int32 Required : Prerequisites[Index]) if (!Ordered.Contains(Required)) { Ready = false; break; }
   if (!Ready) continue;
   Ordered.Add(Index);
   Result.OrderedActors.Add(Actors[Index]);
   Progress = true;
  }
  if (!Progress) return Fail(EHyperManageDismantlePlanStatus::DependencyCycle, nullptr);
 }
 Result.AddedChildren = Actors.Num() - SelectedCount;
 Result.Status = EHyperManageDismantlePlanStatus::Ready;
 return Result;
}
