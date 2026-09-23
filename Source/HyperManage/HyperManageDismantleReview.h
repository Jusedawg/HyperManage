#pragma once

#include "HyperManageDismantleRefund.h"

// Synchronous review only. No plan or payout token is retained by the UI.
struct FHyperManageDismantleReview
{
 FString Error;
 FHyperManageRefundPreview Refunds;
 int32 AddedChildren = 0;
};

class HYPERMANAGE_API FHyperManageDismantleReviewer
{
public:
 static FHyperManageDismantleReview Build(UWorld* World, const TArray<AActor*>& Selection, AActor* Target, const AFGPlayerState* Player);
private:
 friend class FHyperManageDismantleReviewTest;
 using FPlan = TFunctionRef<FHyperManageDismantlePlan(const TArray<AActor*>&)>;
 using FNative = TFunctionRef<FHyperManageRefundPreview(const FHyperManageDismantlePlan&)>;
 using FLightweight = TFunctionRef<FHyperManageRefundPreview(const TArray<FHyperManageLightweightRef>&)>;
 static FHyperManageDismantleReview BuildWithSources(UWorld* World, const TArray<AActor*>& Selection, AActor* Target,
  FPlan PlanNative, FNative ReadNative, FLightweight ReadLightweight);
};
