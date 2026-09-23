#pragma once
#include "HyperManageDismantleRefund.h"

enum class EHyperManageRefundCapacity : uint8 { Unavailable, NoRefund, Fits, NeedsOverflow, TooLarge };

class HYPERMANAGE_API FHyperManageRefundCapacity
{
public:
 static constexpr int32 MaxCheckStacks = 256;
 static constexpr int64 MaxCheckItems = 100000;
 static EHyperManageRefundCapacity Check(UWorld* World, const FHyperManageRefundPreview& Preview, const AFGPlayerState* Player);
private:
 friend class FHyperManageRefundCapacityTest;
 using FCheckBatch = TFunctionRef<bool(const TArray<FInventoryStack>&)>;
 static EHyperManageRefundCapacity CheckWithReader(const FHyperManageRefundPreview& Preview, FCheckBatch CheckBatch);
};
