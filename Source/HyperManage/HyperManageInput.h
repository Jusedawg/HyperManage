#pragma once

#include "CoreMinimal.h"
#include "HyperManageSystem.h"
#include "HyperManageConfig.h"
#include "HyperManageInput.generated.h"

UCLASS(BlueprintType)
class HYPERMANAGE_API UHyperManageInput : public UHyperManageComponent
{
	GENERATED_BODY()

private:
	TMap<FKey, FTimerHandle> KeyTimerHandleMap;

	UPROPERTY(Transient)
	AHyperManageEquip* Equip;

	UPROPERTY(Transient)
	UInputComponent* HyperManageInputComponent;

	void SetupKeyBinding(FHyperManageKeyConfig KeyConfig, EInputEvent InputEvent);

	void SetupInputComponent();

	void ClearAllKeyTimers();

	void PerformIndexedAction(FKey Key, EActionNameIdx ActionIndex, EInputEvent InputEvent);

public:
	void Attach(AHyperManageEquip* Equipment);

	void Detach();

public:
};
