#pragma once

#include "CoreMinimal.h"
#include "FGInventoryComponent.h"
#include "HyperManageDismantlePlan.h"
#include "HyperManageLightweight.h"
#include "HyperManageDismantleRefund.generated.h"

class AFGPlayerState;

UENUM()
enum class EHyperManageRefundStatus : uint8
{
 Ready, InvalidPlan, InvalidPlayer, NotAuthority, InvalidActor, UnsupportedActor, InvalidStack, TooManyStacks, InvalidInstance
};

USTRUCT()
struct FHyperManageActorRefund
{
 GENERATED_BODY()
 UPROPERTY() TWeakObjectPtr<AActor> Actor;
 UPROPERTY() TArray<FInventoryStack> Stacks;
};

USTRUCT()
struct FHyperManageLightweightRefund
{
 GENERATED_BODY()
 UPROPERTY() FHyperManageLightweightRef Ref;
 UPROPERTY() TArray<FInventoryStack> Stacks;
};

// Read-only snapshot, never a payout authorization. Rebuild before execution. Owners
// retaining this across frames must hold it in a reflected property for item-state GC.
USTRUCT()
struct FHyperManageRefundPreview
{
 GENERATED_BODY()
 UPROPERTY() EHyperManageRefundStatus Status = EHyperManageRefundStatus::InvalidPlan;
 UPROPERTY() TArray<FHyperManageActorRefund> Actors;
 UPROPERTY() TArray<FHyperManageLightweightRefund> Instances;
 UPROPERTY() TWeakObjectPtr<AActor> ProblemActor;
 UPROPERTY() bool NoBuildCost = false;
};

class HYPERMANAGE_API FHyperManageDismantleRefunds
{
public:
 static constexpr int32 MaxStacks = 16384;
 static FHyperManageRefundPreview Build(UWorld* World, const FHyperManageDismantlePlan& Plan, const AFGPlayerState* Player);

 // Separate from native planning: this previews costs only, not lightweight dependencies or deletion eligibility.
 static FHyperManageRefundPreview BuildLightweights(UWorld* World, const TArray<FHyperManageLightweightRef>& Refs, const AFGPlayerState* Player);

private:
 friend class FHyperManageLightweightRefundTest;
 using FResolveInstance = TFunctionRef<const FRuntimeBuildableInstanceData*(const FHyperManageLightweightRef&)>;
 using FReadInstanceRefund = TFunctionRef<void(const FHyperManageLightweightRef&, const FRuntimeBuildableInstanceData&, TArray<FInventoryStack>&)>;
 static FHyperManageRefundPreview BuildLightweightsWithReader(UWorld* World, const TArray<FHyperManageLightweightRef>& Refs,
  bool NoBuildCost, FResolveInstance Resolve, FReadInstanceRefund Read);
 friend class FHyperManageDismantleRefundTest;
 using FReadRefund = TFunctionRef<bool(AActor*, bool, TArray<FInventoryStack>&)>;
 static FHyperManageRefundPreview BuildWithReader(UWorld* World, const FHyperManageDismantlePlan& Plan, bool NoBuildCost, FReadRefund Read);
};
