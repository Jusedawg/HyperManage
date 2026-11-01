#include "HyperManageCopyPlacement.h"
#include "HyperManageCopyPreview.h"
#include "HyperManageUI.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Buildables/FGBuildableWall.h"
#include "FGCharacterPlayer.h"
#include "FGInventoryComponent.h"
#include "FGItemPickup_Spawnable.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"
#include "FGRecipeManager.h"
#include "Resources/FGBuildingDescriptor.h"
#include "Engine/World.h"

bool UHyperManageCopyPlacement::AccumulateCost(const TArray<FItemAmount>& Costs, TArray<FItemAmount>& Total)
{
	TArray<FItemAmount> Next = Total;
	for (const auto& Cost : Costs)
	{
		if (!IsValid(Cost.ItemClass) || Cost.Amount <= 0 || Cost.Amount > 1000000) return false;
		auto* Found = Next.FindByPredicate([&](const FItemAmount& Item) { return Item.ItemClass == Cost.ItemClass; });
		if (Found)
		{
			if (int64(Found->Amount) + Cost.Amount > 1000000) return false;
			Found->Amount += Cost.Amount;
		}
		else
		{
			Next.Add(Cost);
		}
	}
	Total = MoveTemp(Next);
	return true;
}

FHyperManageCopyBatchResult UHyperManageCopyPlacement::RunBatch(const TArray<TArray<FItemAmount>>& Costs, FDebit Debit, FCreate Create, FRefund Refund)
{
	FHyperManageCopyBatchResult Result;
	if (Costs.IsEmpty() || Costs.Num() > 50)
	{
		Result.Failed = true;
		return Result;
	}
	TArray<FItemAmount> Total;
	for (const auto& Price : Costs)
	{
		if (!AccumulateCost(Price, Total))
		{
			Result.Failed = true;
			return Result;
		}
	}
	for (int32 Index = 0; Index < Costs.Num(); ++Index)
	{
		TArray<FItemAmount> Paid;
		if (!Debit(Costs[Index], Paid) || !Create(Index))
		{
			Result.Failed = true;
			Result.RefundFailed = !Paid.IsEmpty() && !Refund(Paid);
			break;
		}
		++Result.Completed;
	}
	return Result;
}

bool UHyperManageCopyPlacement::Preflight(TArray<TArray<FItemAmount>>& Costs, TArray<FItemAmount>& Total, FString& Error)
{
	Costs.Reset(); Total.Reset();
	auto Fail = [&](const TCHAR* Reason) { Error = Reason; return false; };
	if (!System || !System->Selection || !System->CopyPreview || !System->GetWorld() || System->GetWorld()->GetNetMode() != NM_Standalone)
		return Fail(TEXT("Copy placement currently requires single-player."));
	auto* Controller = System->GetLocalController();
	auto* Character = Controller ? Cast<AFGCharacterPlayer>(Controller->GetPawn()) : nullptr;
	auto* Inventory = Character ? Character->GetInventory() : nullptr;
	auto* Subsystem = AFGLightweightBuildableSubsystem::Get(System->GetWorld());
	auto* Recipes = AFGRecipeManager::Get(System->GetWorld());
	if (!IsValid(Character) || !Character->HasAuthority() || !IsValid(Inventory) || !IsValid(Subsystem) || !Subsystem->HasAuthority() || !Recipes)
		return Fail(TEXT("The player inventory or construction system is unavailable."));
	if (!System->CopyPreview->IsPlacementReady()) return Fail(TEXT("Aim at a surface within 100 m, or switch to exact offset mode."));
	const auto& Snapshots = System->CopyPreview->GetSnapshots();
	if (Snapshots.IsEmpty() || Snapshots.Num() > 50 || Snapshots.Num() != System->CopyPreview->GetCount())
		return Fail(TEXT("Capture 1-50 pieces before placing."));
	if (System->Selection->HasPendingOperations()) return Fail(TEXT("Wait for pending edits before placing copies."));
	const FVector Offset = System->CopyPreview->GetOffsetMeters() * 100.;
	if (Offset.IsNearlyZero() && !System->CopyPreview->HasPlacementRotation()) return Fail(TEXT("Move the preview away from its captured position before placing copies."));
	const bool NoCost = Inventory->GetNoBuildCost();
	TArray<FTransform> Destinations;
	for (const auto& Snapshot : Snapshots)
	{
		if (!Snapshot.CanPlace || !IsValid(Snapshot.Class) || !IsValid(Snapshot.Recipe)
			|| (!Snapshot.Class->IsChildOf(AFGBuildableFoundation::StaticClass()) && !Snapshot.Class->IsChildOf(AFGBuildableWall::StaticClass()))
			|| !Snapshot.Class->GetOutermost()->GetName().StartsWith(TEXT("/Game/FactoryGame/")))
			return Fail(TEXT("This first placement pass supports vanilla lightweight foundations, ramps and walls without special instance data. Recapture supported pieces."));
		if (!Recipes->IsRecipeAvailable(Snapshot.Recipe)) return Fail(TEXT("A captured building recipe is not unlocked."));
		const auto Products = UFGRecipe::GetProducts(Snapshot.Recipe);
		if (Products.Num() != 1 || Products[0].Amount != 1 || !Products[0].ItemClass || !Products[0].ItemClass->IsChildOf(UFGBuildingDescriptor::StaticClass())
			|| UFGBuildingDescriptor::GetBuildableClass(*Products[0].ItemClass) != Snapshot.Class)
			return Fail(TEXT("A captured recipe does not match its building class. Recapture the selection."));
		const FTransform Destination = System->CopyPreview->GetPlacementTransform(Snapshot.Transform);
		if (!HyperManageLightweight::IsValidTransform(Destination) || FVector::DistSquared(Destination.GetLocation(), Character->GetActorLocation()) > FMath::Square(10000.))
			return Fail(TEXT("Every copy must be within 100 m of the player and have a valid transform."));
		for (int32 Index = 0; Index < Destinations.Num(); ++Index)
		{
			if (Snapshots[Index].Class == Snapshot.Class && Destinations[Index].Equals(Destination, 0.01))
				return Fail(TEXT("The preview contains duplicate pieces at the same transform."));
		}
		if (const auto* Instances = Subsystem->GetAllLightweightBuildableInstances().Find(Snapshot.Class))
		{
			for (const auto& Instance : *Instances)
			{
				if (Instance.IsValid() && Instance.Transform.Equals(Destination, 0.01))
					return Fail(TEXT("An identical building already occupies a preview position. Move or recapture the preview."));
			}
		}
		Destinations.Add(Destination);
		auto Ingredients = NoCost ? TArray<FItemAmount>() : UFGRecipe::GetIngredients(System->GetWorld(), Snapshot.Recipe);
		TArray<FItemAmount> PerPiece;
		if ((!NoCost && Ingredients.IsEmpty()) || !AccumulateCost(Ingredients, PerPiece) || !AccumulateCost(PerPiece, Total))
			return Fail(TEXT("A construction cost is missing, invalid or too large."));
		Costs.Add(MoveTemp(PerPiece));
	}
	TArray<FInventoryStack> Stacks; Inventory->GetInventoryStacks(Stacks);
	for (const auto& Cost : Total)
	{
		if (Inventory->GetNumItems(Cost.ItemClass) < Cost.Amount) return Fail(TEXT("Your inventory does not contain all required materials. Dimensional Depot materials are not used yet."));
		for (const auto& Stack : Stacks)
		{
			if (Stack.Item.GetItemClass() == Cost.ItemClass && (Stack.Item.HasState() || Stack.Item.LegacyItemStateActor))
				return Fail(TEXT("Stateful construction items are not supported for copy placement."));
		}
	}
	return true;
}

FString UHyperManageCopyPlacement::GetMaterialSummary() const
{
	if (!System || !System->CopyPreview || !System->CopyPreview->GetCount()) return TEXT("");
	auto* Controller = System->GetLocalController();
	auto* Character = Controller ? Cast<AFGCharacterPlayer>(Controller->GetPawn()) : nullptr;
	auto* Inventory = IsValid(Character) ? Character->GetInventory() : nullptr;
	if (!IsValid(Inventory)) return TEXT("Materials: player inventory unavailable.");
	if (Inventory->GetNoBuildCost()) return TEXT("Materials: free (No Build Cost).");
	TArray<FItemAmount> Total;
	for (const auto& Snapshot : System->CopyPreview->GetSnapshots())
	{
		if (!Snapshot.CanPlace || !IsValid(Snapshot.Recipe)) return TEXT("Materials unavailable: recapture supported pieces.");
		const auto Ingredients = UFGRecipe::GetIngredients(System->GetWorld(), Snapshot.Recipe);
		if (Ingredients.IsEmpty() || !AccumulateCost(Ingredients, Total)) return TEXT("Materials unavailable: invalid construction cost.");
	}
	TArray<FString> Lines;
	Lines.Add(TEXT("Materials per placement (need / have):"));
	for (const auto& Cost : Total)
	{
		const int32 Available = Inventory->GetNumItems(Cost.ItemClass);
		FString Name = UFGItemDescriptor::GetItemName(Cost.ItemClass).ToString();
		if (Name.IsEmpty())
		{
			Name = TEXT("Construction material");
		}
		const FString Shortage = Available < Cost.Amount ? FString::Printf(TEXT(" - missing %d"), Cost.Amount - Available) : TEXT("");
		Lines.Add(FString::Printf(TEXT("%s: %d / %d%s"), *Name, Cost.Amount, Available, *Shortage));
	}
	Lines.Add(TEXT("Inventory only; checked again when placing."));
	return FString::Join(Lines, TEXT("\n"));
}

bool UHyperManageCopyPlacement::FinishPreview(const FHyperManageCopyBatchResult& Result)
{
	if (!System || !System->CopyPreview) return false;
	const bool Keep = RepeatPlacement && !Result.Failed && !Result.RefundFailed && Result.Completed > 0 && Result.Completed == System->CopyPreview->GetCount();
	if (!Keep)
	{
		System->CopyPreview->Clear();
	}
	return Keep;
}

void UHyperManageCopyPlacement::Request()
{
	if (Executing || !System || !System->UI) return;
	FString Error; TArray<TArray<FItemAmount>> Costs; TArray<FItemAmount> Total;
	if (!Preflight(Costs, Total, Error))
	{
		System->UI->ShowPopup(TEXT("Cannot place copies"), Error);
		return;
	}
	auto* Controller = System->GetLocalController();
	auto* Character = Cast<AFGCharacterPlayer>(Controller->GetPawn());
	const bool SelectCompleted = SelectAfterPlacement;
	Executing = true;
	const auto Snapshots = System->CopyPreview->GetSnapshots();
	auto* Inventory = Character->GetInventory();
	auto* Subsystem = AFGLightweightBuildableSubsystem::Get(System->GetWorld());
	bool DroppedRefund = false;
	TArray<AActor*> PlacedCopies;
	bool CopySelectionAvailable = true;
	const auto Result = RunBatch(Costs, [&](const TArray<FItemAmount>& Price, TArray<FItemAmount>& Paid)
	{
		for (const auto& Cost : Price)
		{
			if (Inventory->GetNumItems(Cost.ItemClass) < Cost.Amount) return false;
		}
		for (const auto& Cost : Price)
		{
			const int32 Before = Inventory->GetNumItems(Cost.ItemClass);
			Inventory->Remove(Cost.ItemClass, Cost.Amount);
			const int32 Removed = FMath::Clamp(Before - Inventory->GetNumItems(Cost.ItemClass), 0, Cost.Amount);
			if (Removed > 0)
			{
				Paid.Add(FItemAmount(Cost.ItemClass, Removed));
			}
			if (Removed != Cost.Amount) return false;
		}
		return true;
	}, [&](int32 Index)
	{
		const auto& Snapshot = Snapshots[Index];
		FRuntimeBuildableInstanceData Data;
		Data.Transform = System->CopyPreview->GetPlacementTransform(Snapshot.Transform);
		Data.BuiltWithRecipe = Snapshot.Recipe;
		Data.CustomizationData = Snapshot.Customization;
		if (auto* Player = Controller->GetPlayerState<AFGPlayerState>())
		{
			Data.BuiltBy = Player->GetPlatformPlayerInfo();
		}
		const int32 CreatedIndex = Subsystem->AddFromBuildableInstanceData(Snapshot.Class, Data);
		if (CreatedIndex == INDEX_NONE) return false;
		const auto* Created = Subsystem->GetRuntimeDataForBuildableClassAndIndex(Snapshot.Class, CreatedIndex);
		if (Created && Created->IsValid() && Created->BuiltWithRecipe == Snapshot.Recipe && Created->Transform.Equals(Data.Transform, 0.01))
		{
			if (SelectCompleted)
			{
				auto* Proxy = System->Selection->GetLightweightProxy(Snapshot.Class, CreatedIndex, *Created);
				if (Proxy)
				{
					PlacedCopies.Add(Proxy);
				}
				else
				{
					CopySelectionAvailable = false;
				}
			}
			return true;
		}
		Subsystem->RemoveByInstanceIndex(Snapshot.Class, CreatedIndex);
		return false;
	}, [&](const TArray<FItemAmount>& Paid)
	{
		TArray<FInventoryStack> Remainder;
		for (const auto& Cost : Paid)
		{
			FInventoryStack Stack(Cost.Amount, Cost.ItemClass);
			Stack.NumItems -= Inventory->AddStack(Stack, true);
			if (Stack.NumItems > 0)
			{
				Remainder.Add(Stack);
			}
		}
		if (Remainder.IsEmpty()) return true;
		AFGCrate* Crate = nullptr;
		AFGItemPickup_Spawnable::SpawnInventoryCrate(System->GetWorld(), Remainder, Character->GetActorLocation(), {Character}, Crate);
		DroppedRefund = Crate != nullptr;
		return Crate != nullptr;
	});
	Executing = false;
	const bool KeepPreview = FinishPreview(Result);
	FString Message = FString::Printf(TEXT("Placed %d of %d copies. Originals are unchanged. Placement has no HyperManage undo."), Result.Completed, Snapshots.Num());
	if (SelectCompleted && Result.Completed > 0)
	{
		const bool Selected = CopySelectionAvailable && PlacedCopies.Num() == Result.Completed && System->Selection->SelectPlacedCopies(PlacedCopies, KeepPreview);
		Message += Selected ? TEXT(" Completed copies are selected. Undo restores the previous selection; it does not remove the copies.") : TEXT(" Copies were built, but could not all be selected. Your previous selection was kept.");
	}
	if (Result.Failed)
	{
		Message += TEXT(" The batch stopped at the first failure; later copies were not attempted. Recapture only the pieces still needed.");
	}
	if (DroppedRefund)
	{
		Message += TEXT(" Returned materials that did not fit are in a crate beside you.");
	}
	if (Result.RefundFailed)
	{
		Message += TEXT(" Material recovery failed. Do not repeat the operation; restore your previous save to recover those materials.");
	}
	if (Result.Failed || DroppedRefund || (SelectCompleted && !CopySelectionAvailable))
	{
		System->UI->ShowPopup(TEXT("Copy placement notice"), Message);
	}
}
