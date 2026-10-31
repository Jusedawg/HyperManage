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
	void TickPlacement();
	bool RotatePreview(int32 Degrees);
	bool ResetRotation();
	FTransform GetPlacementTransform(const FTransform& Source) const;
	bool HasPlacementRotation() const { return !PlacementRotation.Equals(FQuat::Identity); }
	void SetFollowMouse(bool Enabled);
	bool IsFollowingMouse() const { return FollowMouse; }
	bool IsPlacementReady() const { return !FollowMouse || SurfaceValid; }
	static bool SurfaceOffset(const FBox& Bounds, const FVector& Point, const FVector& Normal, FVector& Meters);
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
	FBox CapturedBounds = FBox(ForceInit);
	bool FollowMouse = true;
	bool SurfaceValid = false;
	int32 RotationDegrees = 0;
	FQuat PlacementRotation = FQuat::Identity;
	UPROPERTY(Transient) TObjectPtr<class AFGHologram> SnapHologram;
	bool TryNativeSnap(const FHitResult& Hit, FTransform& Pose);
	bool ApplyOffset(const FVector& Meters, bool Remember);
	void ShowPreview(bool Visible);
};
