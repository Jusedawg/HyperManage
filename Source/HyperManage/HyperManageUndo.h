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
	AActor* Actor = nullptr;

	UPROPERTY()
	FTransform Transform = FTransform::Identity;

public:
	FORCEINLINE ~FUndoTransformActor() = default;
};

USTRUCT()
struct HYPERMANAGE_API FUndoTransformComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	USceneComponent* Component = nullptr;

	UPROPERTY()
	FTransform Transform = FTransform::Identity;

public:
	FORCEINLINE ~FUndoTransformComponent() = default;
};

USTRUCT()
struct HYPERMANAGE_API FUndoColorSlot
{
	GENERATED_BODY()

public:
	UPROPERTY()
	AFGBuildable* Buildable = nullptr;

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
	AActor* Actor = nullptr;

	UPROPERTY()
	bool Select = false;

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

	UPROPERTY() FString Description;

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
	UPROPERTY(Transient) TArray<FUndoInfo> UndoStack;
	UPROPERTY(Transient) TArray<FUndoInfo> RedoStack;
	uint64 Revision = 0;
	void Push(FUndoInfo&& Info);
	bool Transfer(TArray<FUndoInfo>& From, TArray<FUndoInfo>& To, FUndoInfo& Info);
	bool HasPending(const FUndoInfo& Info) const;
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
	bool PopRedo(FUndoInfo& UndoInfo);
	void PushNamedTransforms(TArray<AActor*>& Actors, const FString& Description);
	TArray<FString> GetRecentDescriptions(bool Redo, int32 Limit = 5) const;
	uint64 GetRevision() const { return Revision; }
	int32 GetUndoCount() const { return UndoStack.Num(); }
	int32 GetRedoCount() const { return RedoStack.Num(); }
	bool CanUndo() const { return !UndoStack.IsEmpty() && !HasPending(UndoStack.Last()); }
	bool CanRedo() const { return !RedoStack.IsEmpty() && !HasPending(RedoStack.Last()); }

public:
};