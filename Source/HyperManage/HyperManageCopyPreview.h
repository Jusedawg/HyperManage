#pragma once

#include "CoreMinimal.h"
#include "HyperManageSelection.h"
#include "HyperManageCopyPreview.generated.h"

USTRUCT()
struct FHyperManageCopySnapshot
{
	GENERATED_BODY()
	UPROPERTY(Transient) TSubclassOf<AFGBuildable> Class;
	UPROPERTY(Transient) TSubclassOf<UFGRecipe> Recipe;
	UPROPERTY(Transient) FTransform Transform = FTransform::Identity;
	UPROPERTY(Transient) FFactoryCustomizationData Customization;
	UPROPERTY(Transient) bool CanPlace = false;
};

// Preview geometry and construction metadata only; inventories and connections are never captured.
UCLASS()
class HYPERMANAGE_API UHyperManageCopyPreview : public UHyperManageComponent
{
	GENERATED_BODY()
	friend class FHyperManageCopyPreviewTest;
public:
	bool Capture(const TArray<AActor*>& Actors, const FVector& Meters, FString& Message);
	bool SetOffset(const FVector& Meters);
	void CaptureSelection();
	FVector GetPreferredOffsetMeters() const { return PreferredOffsetMeters; }
	void Clear();
	uint64 GetRevision() const { return Revision; }
	const TArray<FHyperManageCopySnapshot>& GetSnapshots() const { return Snapshots; }
	int32 GetCount() const { return Pieces.Num(); }
	FVector GetOffsetMeters() const { return OffsetCm / 100.; }
private:
	UPROPERTY(Transient) TArray<FSelectedActorInfo> Pieces;
	TArray<FTransform> OriginalMeshTransforms;
	FVector OffsetCm = FVector::ZeroVector;
	FVector PreferredOffsetMeters = FVector(8, 0, 0);
	UPROPERTY(Transient) TArray<FHyperManageCopySnapshot> Snapshots;
	uint64 Revision = 0;
};
