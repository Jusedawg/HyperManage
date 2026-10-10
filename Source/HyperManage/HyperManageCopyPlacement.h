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
	UFUNCTION() void Confirm(bool Accepted);
	static bool AccumulateCost(const TArray<FItemAmount>& Costs, TArray<FItemAmount>& Total);
	using FDebit = TFunctionRef<bool(const TArray<FItemAmount>&, TArray<FItemAmount>&)>;
	using FCreate = TFunctionRef<bool(int32)>;
	using FRefund = TFunctionRef<bool(const TArray<FItemAmount>&)>;
	static FHyperManageCopyBatchResult RunBatch(const TArray<TArray<FItemAmount>>& Costs, FDebit Debit, FCreate Create, FRefund Refund);
private:
	bool Preflight(TArray<TArray<FItemAmount>>& Costs, TArray<FItemAmount>& Total, FString& Error);
	UPROPERTY(Transient) TArray<FItemAmount> ConfirmedCosts;
	TWeakObjectPtr<class AFGCharacterPlayer> ConfirmedCharacter;
	uint64 ConfirmedRevision = 0;
	double ConfirmedAt = 0;
	bool ConfirmedNoCost = false;
	bool AwaitingConfirmation = false;
	bool Executing = false;
};
