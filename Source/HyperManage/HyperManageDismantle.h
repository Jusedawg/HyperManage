#pragma once
#include "HyperManageSystem.h"
#include "HyperManageDismantle.generated.h"

UCLASS()
class HYPERMANAGE_API UHyperManageDismantle : public UHyperManageComponent
{
 GENERATED_BODY()
public:
 void Request();
 UFUNCTION() void Confirm(bool Accepted);
 static constexpr int32 MaxBuildings = 50;
private:
 friend class FHyperManageDismantleExecutionTest;
 static bool ValidateCandidates(UWorld* World, const TArray<AActor*>& Input, AActor* Target, TArray<AActor*>& Output, FString& Error);
 static UFunction* FindNativeDispatch(UObject* State);
 static bool MatchesSnapshot(const TArray<AActor*>& Actors, const TMap<TWeakObjectPtr<AActor>, FTransform>& Snapshot);
 bool Preflight(TArray<AActor*>& Actors, FString& Error);
 TMap<TWeakObjectPtr<AActor>, FTransform> Pending;
 TWeakObjectPtr<AActor> PendingTarget;
 TWeakObjectPtr<AFGPlayerState> PendingPlayer;
 bool PendingNoBuildCost = false;
 double PendingAt = 0;
 bool AwaitingConfirmation = false;
};
