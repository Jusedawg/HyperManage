#pragma once

#include "CoreMinimal.h"
#include "HyperManageSystem.h"
#include "HyperManageAction.generated.h"

UCLASS(BlueprintType)
class HYPERMANAGE_API UHyperManageAction : public UHyperManageComponent
{
	GENERATED_BODY()

public:

	void PerformUndo();
	void PerformRedo();
	void PerformHistory(bool Redo);
	bool ApplyWorldRotationOffset(const FRotator& Degrees);
	bool ApplyScalePercent(const FVector& Percent);
	bool ApplyWorldOffset(const FVector& Meters, bool ObjectAxes = false);
	bool GetWorldPositionReference(FVector& ReferenceCm);
	bool ApplyWorldPosition(const FVector& Meters, uint8 AxisMask = 7);
	bool GetWorldOrientationReference(FTransform& Reference);
	bool ApplyWorldOrientation(const FRotator& Degrees, uint8 AxisMask = 7);
	void MatchAnchorOrigin(EAxis::Type Axis);
	void AlignToWorld(EActionNameIdx Action);

	void PrepareTransform(const FVector& Loc, const FRotator& Rot, const FVector& Scale);

	void MoveSelectionToTarget(bool IgnoreTranslation = false);

	UFUNCTION()
	void PerformMove(bool ConfirmClicked);

	void PrepareMove();

	void MakeActorsMovable(TArray<AActor*>& Actors);

	void MakeActorMovable(AActor* Actor);

	void MakeConnection(AActor* OutputActor, AActor* InputActor, FString& Title, FString& Body);

	void BreakConnection(AActor* OutputActor, AActor* InputActor, FString& Title, FString& Body);

	void RemoveIndicator();

	void SelectActor(AActor* Actor, bool Select);

	void SetSameScale();

	void SetSamePaint();

public:
};
