#pragma once
#include "HyperManageSystem.h"
#include "HyperManageLightweight.h"
#include "Equipment/FGBuildGunDismantle.h"
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
 using FResolveInstance = TFunctionRef<const FRuntimeBuildableInstanceData*(const FHyperManageLightweightRef&)>;
 static bool ValidateWithResolver(UWorld* World, const TArray<AActor*>& Input, AActor* Target, TArray<AActor*>& Output, FString& Error, FResolveInstance Resolve);
 static void MakeDispatch(const TArray<AActor*>& Selection, TArray<AActor*>& Actors, TArray<FDismantleLightweightBundle>& Bundles);
 static bool IsSupportedStorageClass(const UClass* Class);
 static bool RefundCoversContents(const TArray<FInventoryStack>& Contents, const TArray<FInventoryStack>& Refund);
 static UFunction* FindNativeDispatch(UObject* State);
 static bool MatchesSnapshot(const TArray<AActor*>& Actors, const TMap<TWeakObjectPtr<AActor>, FTransform>& Snapshot);
 bool Preflight(TArray<AActor*>& Actors, FString& Error);
 TMap<TWeakObjectPtr<AActor>, FTransform> Pending;
 struct FConfirmedInstance { FHyperManageLightweightRef Ref; TArray<FInstanceOwnerHandlePtr> Handles; };
 TMap<TWeakObjectPtr<AActor>, FConfirmedInstance> PendingInstances;
 static bool MatchesInstance(const FConfirmedInstance& Snapshot, const FHyperManageLightweightRef& Ref, const FRuntimeBuildableInstanceData* Data);
 using FConnections = TMap<TWeakObjectPtr<class UFGFactoryConnectionComponent>, TWeakObjectPtr<class UFGFactoryConnectionComponent>>;
 static FConnections CaptureConnections(const TArray<AActor*>& Actors);
 static bool ConnectionsMatch(const FConnections& Before, const FConnections& After);
 FConnections PendingConnections;
 TWeakObjectPtr<AActor> PendingTarget;
 TWeakObjectPtr<AFGPlayerState> PendingPlayer;
 bool PendingNoBuildCost = false;
 double PendingAt = 0;
 bool AwaitingConfirmation = false;
};
