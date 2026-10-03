#pragma once

#include "CoreMinimal.h"
#include "HyperManageSelection.h"
#include "HyperManageCopyPreview.generated.h"

// A local geometry snapshot only. No construction, inventory, recipe or connection state is duplicated.
UCLASS()
class HYPERMANAGE_API UHyperManageCopyPreview : public UHyperManageComponent
{
	GENERATED_BODY()
	friend class FHyperManageCopyPreviewTest;
public:
	bool Capture(const TArray<AActor*>& Actors, const FVector& Meters, FString& Message);
	bool SetOffset(const FVector& Meters);
	void Clear();
	int32 GetCount() const { return Pieces.Num(); }
	FVector GetOffsetMeters() const { return OffsetCm / 100.; }
private:
	UPROPERTY(Transient) TArray<FSelectedActorInfo> Pieces;
	TArray<FTransform> OriginalMeshTransforms;
	FVector OffsetCm = FVector::ZeroVector;
};
