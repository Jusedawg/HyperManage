#pragma once

#include "HyperManageSystem.h"
#include "ItemAmount.h"
#include "HyperManageCopyPlacement.generated.h"

struct FHyperManageCopyBatchResult
{
	int32 Completed = 0;
	bool Failed = false;
	bool RefundFailed = false;
};

UCLASS()
class HYPERMANAGE_API UHyperManageCopyPlacement : public UHyperManageComponent
{
	GENERATED_BODY()
public:
	void Request();
	bool IsExecuting() const { return Executing; }
	bool SelectAfterPlacement = false;
	static bool AccumulateCost(const TArray<FItemAmount>& Costs, TArray<FItemAmount>& Total);
	using FDebit = TFunctionRef<bool(const TArray<FItemAmount>&, TArray<FItemAmount>&)>;
	using FCreate = TFunctionRef<bool(int32)>;
	using FRefund = TFunctionRef<bool(const TArray<FItemAmount>&)>;
	static FHyperManageCopyBatchResult RunBatch(const TArray<TArray<FItemAmount>>& Costs, FDebit Debit, FCreate Create, FRefund Refund);
private:
	bool Preflight(TArray<TArray<FItemAmount>>& Costs, TArray<FItemAmount>& Total, FString& Error);
	bool Executing = false;
};
