#pragma once

#include "CoreMinimal.h"
#include "HyperManageSystem.h"
#include "HyperManageLightweight.h"
#include "HyperManageSelection.generated.h"

USTRUCT()
struct HYPERMANAGE_API FSelectedActorInfo
{
	GENERATED_BODY()
	UPROPERTY(Transient)
	TWeakObjectPtr<class UFGOutlineComponent> Outline;
	UPROPERTY(Transient)
	uint8 PreviousOutlineColor = 0;
	UPROPERTY(Transient)
	uint8 SelectionOutlineColor = 0;
};

USTRUCT()
struct FHyperManageSelectionSlot
{
 GENERATED_BODY()
 UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Actors;
 UPROPERTY(Transient) TObjectPtr<AActor> Anchor;
 UPROPERTY(Transient) TObjectPtr<AActor> Target;
 UPROPERTY(Transient) FString Name;
 bool Occupied = false;
};

UCLASS(BlueprintType)
class HYPERMANAGE_API UHyperManageSelection : public UHyperManageComponent
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TMap<AActor*, FSelectedActorInfo> SelectedMap;
	UPROPERTY(Transient)
	TArray<AHyperManageLightweightProxy*> LightweightProxies;
	UPROPERTY(Transient)
	TObjectPtr<class UPostProcessComponent> SelectionPostProcess;


	UPROPERTY(Transient) TArray<FHyperManageSelectionSlot> SelectionSlots;
	int32 ActiveSelectionSlot = 0;

	bool SetMarker(AActor* Actor, AActor** Marker1, AActor** Marker2);

public:
	UPROPERTY()
	AActor* AnchorActor;

	UPROPERTY()
	AActor* TargetActor;

	UFUNCTION()
	void SelectNextMaterial();

	//
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	void SetSelectedMaterial(TArray<UMaterialInterface*> Materials);

	//
	void ShowHologram(AActor* Actor, FSelectedActorInfo& ActorInfo);

	//
	void HideHologram(AActor* Actor, FSelectedActorInfo& ActorInfo);

	void ResetHologram(AActor* Actor);

	void RefreshMaterial(AActor* Actor);

	void RefreshMaterials(const TArray<AActor*>& Actors);

	FSelectedActorInfo* GetActorInfo(AActor* Actor);

	//
	bool IsValidActor(AActor* Actor);

	// Adds Actor to or removes Actor from the selection and updates the hologram
	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	bool SelectActor(AActor* Actor, bool Select = true, bool DeleteFromMap = true);

	//
	UFUNCTION()
	bool SetAnchor(AActor* Actor);

	//
	UFUNCTION()
	bool SetTarget(AActor* Actor);

	//
	UFUNCTION()
	bool Contains(AActor* Actor);

	// Returns the number of Actors currently selected
	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	int SelectCount();

	// Returns an array of all currently selected Actors
	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	void SelectedActors(TArray<AActor*>& Actors);

	UFUNCTION()
	void SelectedActorsNoTarget(TArray<AActor*>& Actors);

	UFUNCTION()
	void GetSelectionOrLineTrace(TArray<AActor*>& Actors);

	UFUNCTION()
	void AddAnchorTargetBoxToSelection(bool UseSides = true);
	void ChangeAnchorTargetBoxSelection(bool UseSides, bool Remove);

	//
	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	void SelectClear(bool ConfirmClicked = true);

	bool SelectActorWithHistory(AActor* Actor, bool Select);
	bool SelectPointedActorForTransform(AActor* Actor);
	bool SelectPlacedBlueprint(AActor* Actor);
	bool SetMarkerWithHistory(AActor* Actor, bool Anchor);
	void ClearWithoutHistory();
	void RestoreHistory(const struct FUndoInfo& Info);
	void SaveSelection();
	bool SetSelectionSlot(int32 Index);
	bool SetSelectionSlotName(int32 Index, const FString& Name);
	FString GetSelectionSlotName(int32 Index) const;
	FString GetSelectionSlotLabel(int32 Index) const;
	int32 GetSelectionSlot() const { return ActiveSelectionSlot; }
	bool HasSavedSelection() const;
	int32 GetSavedSelectionCount();

	void LoadSelection();
	void AddSavedSelection();
	void RemoveSavedSelection();
	static UClass* GetSelectionType(const AActor* Actor);
	void FilterAnchorType(bool KeepMatching);

	bool HasPendingOperations() const;
	AActor* LineTraceFromPlayer();
	AHyperManageLightweightProxy* GetLightweightProxy(TSubclassOf<AFGBuildable> Class, int32 Index, const FRuntimeBuildableInstanceData& Data);
	void AcknowledgeLightweight(const FHyperManageLightweightRef& Ref, const FFactoryCustomizationData& Data, bool Success);

public:
};


