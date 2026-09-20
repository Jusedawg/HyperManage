#pragma once

#include "CoreMinimal.h"
#include "HyperManageSystem.h"
#include "HyperManageTransform.generated.h"

USTRUCT()
struct HYPERMANAGE_API FHyperManageTransformData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FVector Loc = FVector::ZeroVector;

	UPROPERTY()
	bool IsLoc = false;

	UPROPERTY()
	FRotator Rot = FRotator::ZeroRotator;

	UPROPERTY()
	bool IsRot = false;

	UPROPERTY()
	FVector Scale = FVector::OneVector;

	UPROPERTY()
	bool IsScale = false;

	UPROPERTY()
	TEnumAsByte<EAxis::Type> TransformAxis = EAxis::None;

	UPROPERTY()
	FQuat PivotQuat = FQuat::Identity;

	UPROPERTY()
	FVector PivotLoc = FVector::ZeroVector;

	UPROPERTY()
	FVector PivotAxis = FVector::UpVector;

	UPROPERTY()
	double PivotAngle = 0.0;

	UPROPERTY()
	FVector PivotTranslation = FVector::ZeroVector;

	UPROPERTY()
	AActor* Anchor = nullptr;

	UPROPERTY()
	AActor* Target = nullptr;

	UPROPERTY()
	FVector ViewVector = FVector::ForwardVector;

	UPROPERTY()
	FQuat AnchorQuat = FQuat::Identity;

	UPROPERTY()
	FQuat TargetRotation = FQuat::Identity;

	UPROPERTY()
	bool ViewRelative = true;

	UPROPERTY()
	bool GroupMode = true;

	UPROPERTY()
	bool SetSame = false;
	UPROPERTY() bool WorldAlignment = false;
	UPROPERTY() bool SnapWorldPosition = false;
	UPROPERTY() bool SnapWorldRotation = false;
	UPROPERTY() bool LevelWorldRotation = false;
	UPROPERTY() double AlignmentGridCm = 800.0;
	UPROPERTY() double AlignmentAngle = 5.0;
	UPROPERTY() bool WorldRotationOffset = false;
	UPROPERTY() bool WorldOriginAlignment = false;
	UPROPERTY() bool SnapWorldHeight = false;

public:
	FORCEINLINE ~FHyperManageTransformData() = default;
};

UCLASS(BlueprintType)
class HYPERMANAGE_API UHyperManageTransform : public UHyperManageComponent
{
	GENERATED_BODY()

private:
	// Modifies the location component of Transform for a grouped component.
	//
	// If there is scaling:
	//    The location is adjusted towards or away from the pivot location to match the
	//    scale change.
	//
	// If there is rotation:
	//    The location is adjusted based on the perpendicular rotation around PivotAxis
	//    by the angle PivotAngle.
	void TranslateAroundPivot(FTransform& Transform, const FHyperManageTransformData& TransformData);

	// Applies the scaling (Scale), rotation (PivotQuat) and translation (PivotTranslation)
	// values from TransformData to Transform for final placement of the component.
	//
	void ModifyTransform(FTransform& Transform, FHyperManageTransformData& TransformData);

public:
	// Applies Transform to the component, SceneComp
	//
	void TransformComponent(USceneComponent* SceneComp, const FTransform& Transform);

	// Applies Transform on the root component of Actor

	void TransformActor(AActor* Actor, const FTransform& Transform);
	static bool MakeWorldOriginAlignment(const FVector& Reference, EAxis::Type Axis, FHyperManageTransformData& Data);
	static FVector OriginAlignmentDelta(const FVector& Origin, const FVector& Reference, EAxis::Type Axis);
	static bool IsValidScalePercent(const FVector& Percent);
	static bool MakeAbsoluteScale(const FTransform& Original, const FVector& Scale, FTransform& Result);
	static bool IsValidRotationOffset(const FRotator& Degrees);
	static bool MakeWorldRotationOffset(const FRotator& Degrees, bool Grouped, const FVector& Pivot, FHyperManageTransformData& Data);
	static bool MakeWorldOrientation(const FRotator& Degrees, const FTransform& Reference, FHyperManageTransformData& Data);
	static bool MakeWorldPositionOffset(const FVector& Meters, const FVector& ReferenceCm, FHyperManageTransformData& Data);
	static bool MakeWorldOffset(const FVector& Meters, FHyperManageTransformData& Data);
	FTransform ComputeTransform(FTransform Transform, const FHyperManageTransformData& Data);

	// Calculates remaining fields of an initialized TransformData for further processing

	void CalculateTransformData(FHyperManageTransformData& TransformData);

	// Assumes a single non-zero field in Loc and Rot and returns the axis of that field

	EAxis::Type GetTransformAxis(const FVector& Loc, const FRotator& Rot);

	// Returns the axis of Quat designated by Axis

	FVector GetQuatAxis(EAxis::Type Axis, FQuat& Quat);

	// Returns the location of Anchor if defined.  If Anchor is not defined then returns
	// the calculated center of all selected objects.

	FVector CalculatePivotLoc(const TArray<AActor*>& Actors, const AActor* Anchor, const AActor* Target);

	// The X, Y & Z axes are calculated from the provided ActorQuat.  Then those axes
	// are reoriented based on the natural up direction and the provided ViewVector to
	// be front/back, left/right, and up/down normalized.  The pivot axis, based on the
	// rotation DesiredAxis, is then returned.

	FVector CalcPivotAxis(const EAxis::Type DesiredAxis, const FVector& ViewVector, const FQuat& ActorQuat);

	void CalcViewAxis(const EAxis::Type DesiredAxis, const FVector& ViewVector, const FQuat& ActorQuat, EAxis::Type& FoundAxis, bool& Inverted);

	// Calculates PivotAxis based on the TransformData values: TransformAxis, ViewVector, 
	// and AnchorQuat.  
	//
	// If the transform is a rotation:
	//    PivotAngle is retrieved from the non-zero component of TransformData.Rot and 
	//    PivotQuat is calculated as the rotation around PivotAxis by PivotAngle.
	//
	// If the transform is a translation:
	//    PivotTranslation is calculated as the desired movement, TransformData.Loc, along
	//    PivotAxis.
	void CalculateSimplePivot(FHyperManageTransformData& TransformData);

	// Calculates PivotQuat, PivotAxis, PivotAngle, and PivotTranslation based on the
	// difference between TransformData.Target and the (pseudo)Anchor values.  Used with
	// MoveSelection and SetSameRotation type functions to perform complex rotations.
	void CalculateTargetPivot(FHyperManageTransformData& TransformData);

	// Transforms all "root" components of the supplied Actors based on TransformData.
	//
	// If grouped:
	//    The precalculated transform values based on the (pseudo)Anchor are used.
	//
	// If ungrouped:
	//    Transform values are calculated for each component relative to itself.
	void ProcessTransform(const TArray<AActor*>& Actors, const FHyperManageTransformData& TransformData);


	// Calculated the center and size of an actor's bounding box.  Size is local world values, not rotated world values.
	void GetActorOriginAndSize(AActor* Actor, FVector& Origin, FVector& Size);

	void AlignToActor(AActor* Anchor, int Position, const EAxis::Type DesiredAxis);

public:
};