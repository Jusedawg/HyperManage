#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

// Membership and ordering only. A plan is not permission to dismantle: execution must
// revalidate eligibility and membership, confirm added children, and handle every refund.
enum class EHyperManageDismantlePlanStatus : uint8
{
 Ready, EmptySelection, NotAuthority, InvalidActor, UnsupportedActor, ProtectedTarget,
 MissingDependency, DependencyCycle, TooManyActors
};

struct FHyperManageDismantlePlan
{
 EHyperManageDismantlePlanStatus Status = EHyperManageDismantlePlanStatus::EmptySelection;
 TArray<TWeakObjectPtr<AActor>> OrderedActors;
 TWeakObjectPtr<AActor> ProblemActor;
 TWeakObjectPtr<AActor> RequiredActor;
 int32 AddedChildren = 0;
};

class HYPERMANAGE_API FHyperManageDismantlePlanner
{
public:
 static constexpr int32 MaxActors = 1024;
 // Native actors only for now. Lightweight instances require a separate identity/refund path.
 static FHyperManageDismantlePlan Build(UWorld* World, const TArray<AActor*>& Selection, AActor* ProtectedTarget);

private:
 friend class FHyperManageDismantlePlanTest;
 struct FLinks
 {
  TArray<AActor*> Children;
  TArray<AActor*> Dependencies;
 };
 using FResolve = TFunctionRef<bool(AActor*, FLinks&)>;
 static FHyperManageDismantlePlan BuildWithResolver(UWorld* World, const TArray<AActor*>& Selection, AActor* ProtectedTarget, FResolve Resolve);
};
