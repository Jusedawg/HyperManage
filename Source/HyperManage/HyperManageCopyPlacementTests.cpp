#include "HyperManageCopyPlacement.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageCopyPlacementTest, "HyperManage.Copy.PlacementBatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageCopyPlacementTest::RunTest(const FString& Parameters)
{
	const auto Item = UFGItemDescriptor::StaticClass();
	TArray<FItemAmount> Total;
	TestTrue(TEXT("Repeated ingredients aggregate"), UHyperManageCopyPlacement::AccumulateCost({FItemAmount(Item, 2), FItemAmount(Item, 3)}, Total));
	TestEqual(TEXT("Aggregate amount"), Total[0].Amount, 5);
	TestFalse(TEXT("Overflow rejected"), UHyperManageCopyPlacement::AccumulateCost({FItemAmount(Item, 1000000)}, Total));
	TestFalse(TEXT("Negative amount rejected"), UHyperManageCopyPlacement::AccumulateCost({FItemAmount(Item, -1)}, Total));
	TestFalse(TEXT("Missing descriptor rejected"), UHyperManageCopyPlacement::AccumulateCost({FItemAmount(nullptr, 1)}, Total));
	TestEqual(TEXT("Rejected cost leaves aggregate unchanged"), Total[0].Amount, 5);
	TArray<TArray<FItemAmount>> Prices = {{FItemAmount(Item, 2)}, {FItemAmount(Item, 2)}, {FItemAmount(Item, 2)}};
	int32 Balance = 10, Creates = 0, Refunds = 0;
	auto Debit = [&](const TArray<FItemAmount>& Price, TArray<FItemAmount>& Paid)
	{
		Paid = Price;
		for (const auto& Cost : Price)
		{
			Balance -= Cost.Amount;
		}
		return true;
	};
	auto Refund = [&](const TArray<FItemAmount>& Paid)
	{
		++Refunds;
		for (const auto& Cost : Paid)
		{
			Balance += Cost.Amount;
		}
		return true;
	};
	auto Result = UHyperManageCopyPlacement::RunBatch(Prices, Debit, [&](int32 Index) { ++Creates; return Index != 1; }, Refund);
	TestEqual(TEXT("Earlier successful copy remains completed"), Result.Completed, 1);
	TestTrue(TEXT("Construction failure stops batch"), Result.Failed);
	TestEqual(TEXT("Unattempted piece never constructed"), Creates, 2);
	TestEqual(TEXT("Only completed piece remains charged"), Balance, 8);
	TestEqual(TEXT("Failed piece refunded once"), Refunds, 1);
	Creates = 0; Balance = 10;
	Result = UHyperManageCopyPlacement::RunBatch(Prices, [&](const TArray<FItemAmount>& Price, TArray<FItemAmount>& Paid)
	{
		Paid.Add(FItemAmount(Item, 1)); --Balance; return false;
	}, [&](int32) { ++Creates; return true; }, Refund);
	TestEqual(TEXT("Incomplete payment never constructs"), Creates, 0);
	TestEqual(TEXT("Partial debit is recovered"), Balance, 10);
	Result = UHyperManageCopyPlacement::RunBatch(Prices, Debit, [](int32) { return false; }, [](const TArray<FItemAmount>&) { return false; });
	TestTrue(TEXT("Failed material recovery is reported"), Result.RefundFailed);
	Balance = 10; Creates = 0; Refunds = 0;
	Result = UHyperManageCopyPlacement::RunBatch(Prices, Debit, [&](int32) { ++Creates; return true; }, Refund);
	TestEqual(TEXT("Successful batch completes all pieces"), Result.Completed, 3);
	TestEqual(TEXT("Successful copies each charged exactly once"), Balance, 4);
	TestEqual(TEXT("Successful batch has no refunds"), Refunds, 0);
	TArray<TArray<FItemAmount>> FreePrices; FreePrices.SetNum(2);
	Result = UHyperManageCopyPlacement::RunBatch(FreePrices, Debit, [](int32) { return true; }, Refund);
	TestEqual(TEXT("No-cost mode constructs without materials"), Result.Completed, 2);
	TestEqual(TEXT("No-cost leaves inventory unchanged"), Balance, 4);
	Creates = 0;
	Prices[2][0].Amount = -1;
	Result = UHyperManageCopyPlacement::RunBatch(Prices, Debit, [&](int32) { ++Creates; return true; }, Refund);
	TestTrue(TEXT("Invalid later price rejects entire batch"), Result.Failed);
	TestEqual(TEXT("Validation occurs before any construction"), Creates, 0);
	TestEqual(TEXT("Validation occurs before any payment"), Balance, 4);
	FreePrices.SetNum(51);
	Result = UHyperManageCopyPlacement::RunBatch(FreePrices, Debit, [](int32) { return true; }, Refund);
	TestTrue(TEXT("Oversized batch rejected"), Result.Failed);
	return true;
}
#endif
