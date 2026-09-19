#pragma once

#include "CoreMinimal.h"
#include "Equipment/FGEquipment.h"
#include "FGCharacterPlayer.h"
#include "FGPlayerController.h"
#include "HyperManageTransform.h"
#include "HyperManageUndo.h"
#include "HyperManageEquip.generated.h"

class UHyperManageSystem;

UCLASS()
class HYPERMANAGE_API AHyperManageEquip : public AFGEquipment
{
	GENERATED_BODY()
	
public:	
	AHyperManageEquip();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(Transient)
	UHyperManageSystem* System;

	bool ManagerEquipped = false;

	void SetupHyperManageSystem();

public:
	bool IsLocal = false;

	UFUNCTION(Reliable, NetMulticast, Category = "HyperManage")
	void MulticastShowPopup(const FGuid& Id, const FString& Title, const FString& Body);

	UFUNCTION(Reliable, NetMulticast, Category = "HyperManage")
	void MulticastTransformActors(const TArray<AActor*>& Actors, FHyperManageTransformData TransformData);

	UFUNCTION(Reliable, NetMulticast, Category = "HyperManage")
	void MulticastUndoTransforms(const FUndoInfo& UndoInfo);

	UFUNCTION(Reliable, NetMulticast, Category = "HyperManage")
	void MulticastRefreshMaterials(const TArray<AActor*>& Actors);

	UFUNCTION(Reliable, NetMulticast, Category = "HyperManage")
	void MulticastUnEquip();

	virtual void Equip(AFGCharacterPlayer* Character) override;

	virtual void UnEquip() override;

public:
};
