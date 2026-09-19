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
