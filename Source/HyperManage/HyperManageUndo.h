#pragma once

#include "CoreMinimal.h"
#include "HyperManageSystem.h"
#include "FGFactoryColoringTypes.h"
#include "HyperManageLightweight.h"
#include "HyperManageUndo.generated.h"

#define MAXUNDO 1000

USTRUCT()
struct HYPERMANAGE_API FUndoTransformActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	AActor* Actor;

	UPROPERTY()
	FTransform Transform;

public:
	FORCEINLINE ~FUndoTransformActor() = default;
};

USTRUCT()
struct HYPERMANAGE_API FUndoTransformComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	USceneComponent* Component;

	UPROPERTY()
	FTransform Transform;

public:
	FORCEINLINE ~FUndoTransformComponent() = default;
};

USTRUCT()
struct HYPERMANAGE_API FUndoColorSlot
{
	GENERATED_BODY()

public:
	UPROPERTY()
	AFGBuildable* Buildable;

	UPROPERTY()
	FFactoryCustomizationData CustomizationData;

public:
	FORCEINLINE ~FUndoColorSlot() = default;
};

USTRUCT()
struct HYPERMANAGE_API FUndoSelect
{
	GENERATED_BODY()

public:
	UPROPERTY()
	AActor* Actor;

	UPROPERTY()
	bool Select;

public:
	FORCEINLINE ~FUndoSelect() = default;
};

USTRUCT()
struct FUndoLightweight
{
	GENERATED_BODY()
	UPROPERTY() AHyperManageLightweightProxy* Proxy = nullptr;
	UPROPERTY() FTransform Transform = FTransform::Identity;
	UPROPERTY() bool Paint = false;
	UPROPERTY() FFactoryCustomizationData Customization;
};
USTRUCT()
struct HYPERMANAGE_API FUndoInfo
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FUndoTransformActor> TransformActors;

	UPROPERTY()
	TArray<FUndoLightweight> Lightweights;

	UPROPERTY()
	TArray<FUndoTransformComponent> TransformComponents;

	UPROPERTY()
	TArray<FUndoColorSlot> ColorSlotItems;

	UPROPERTY()
	TArray<FUndoSelect> SelectItems;

	void Clear();

public:
	FORCEINLINE ~FUndoInfo() = default;
};

// UHyperManageUndo -------------------------------------------------------------------------------

UCLASS(BlueprintType)
class HYPERMANAGE_API UHyperManageUndo : public UHyperManageComponent
{
	GENERATED_BODY()

private:
	FUndoInfo UndoInfoArr[MAXUNDO];
	int UndoHead = 0;
	int UndoCount = 0;

	void Push();
	void Pop();

public:
	UFUNCTION()
	void PushUndoTransforms(TArray<AActor*>& Actors);

	UFUNCTION()
	void PushUndoColorSlot(TArray<AActor*>& Actors);

	UFUNCTION()
	void PushUndoSelection(TArray<AActor*>& Actors);

	UFUNCTION()
	bool PopUndo(FUndoInfo& UndoInfo);

	UFUNCTION()
	void ClearUndoStack();

public:
};