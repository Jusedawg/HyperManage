#include "HyperManageTransform.h"
#include "HyperManageSelection.h"
#include "HyperManageConfig.h"
#include "HyperManageEquip.h"

void UHyperManageTransform::TransformComponent(USceneComponent* SceneComp, const FTransform& Transform)
{
	if (!IsValid(SceneComp) || Transform.ContainsNaN() || !Transform.GetRotation().IsNormalized()) return;
	auto SavedMobility = SceneComp->Mobility;
	SceneComp->SetMobility(EComponentMobility::Movable);
	SceneComp->SetWorldTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	if (SavedMobility != EComponentMobility::Movable) {
		SceneComp->SetMobility(SavedMobility);
	}
}

void UHyperManageTransform::TransformActor(AActor* Actor, const FTransform& Transform)
{
	if (!IsValid(Actor)) return;
	TransformComponent(Actor->GetRootComponent(), Transform);
	System->Selection->RefreshMaterial(Actor);
}

void UHyperManageTransform::CalculateTransformData(FHyperManageTransformData& TransformData)
{
	// pre-calculcate remaining parts of TransformData struct as much as possible
	TransformData.IsLoc = !TransformData.Loc.IsNearlyZero(DELTA);
	TransformData.IsRot = !TransformData.Rot.IsNearlyZero(DELTA);
	TransformData.IsScale = !TransformData.Scale.Equals(FVector::OneVector, DELTA);
	TransformData.TransformAxis = GetTransformAxis(TransformData.Loc, TransformData.Rot);
	if (TransformData.GroupMode) {
		// calculate anchor quaternion or create pseudo-anchor quaternion if no anchor defined
		if (TransformData.Anchor) {
			TransformData.AnchorQuat = TransformData.Anchor->GetActorQuat();
		} else {
			if (TransformData.ViewRelative) { // create pseudo-anchor aligned with view vector
				TransformData.AnchorQuat = TransformData.ViewVector.Rotation().Quaternion();
			} else { // create pseudo-anchor aligned with north
				TransformData.AnchorQuat = FVector::LeftVector.Rotation().Quaternion();
			}
		}
		CalculateSimplePivot(TransformData);
	}
}

EAxis::Type UHyperManageTransform::GetTransformAxis(const FVector& Loc, const FRotator& Rot)
{
	if (!FMath::IsNearlyZero(Rot.Yaw, DELTA) || !FMath::IsNearlyZero(Loc.Z, DELTA)) {
		return EAxis::Type::Z;
	} else if (!FMath::IsNearlyZero(Rot.Pitch, DELTA) || !FMath::IsNearlyZero(Loc.Y, DELTA)) {
		return EAxis::Type::Y;
	} else if (!FMath::IsNearlyZero(Rot.Roll, DELTA) || !FMath::IsNearlyZero(Loc.X, DELTA)) {
		return EAxis::Type::X;
	} else {
		return EAxis::Type::None;
	}
}

FVector UHyperManageTransform::GetQuatAxis(EAxis::Type Axis, FQuat& Quat)
{
	switch (Axis) {
		case EAxis::X: return Quat.GetAxisX();
		case EAxis::Y: return Quat.GetAxisY();
		default: return Quat.GetAxisZ();
	}
}

FVector UHyperManageTransform::CalculatePivotLoc(const TArray<AActor*>& Actors, const AActor* Anchor, const AActor* Target)
{
	if (Anchor) { // pivot location set to anchor
		return Anchor->GetActorLocation();
	} else { // pivot location set to calculated center of all selected items
		// switch to Miniball algorithm eventually
		FBox Bounds = FBox(EForceInit::ForceInit);
		for (const auto& Actor : Actors) {
			if (Actor != Target) {
				Bounds += Actor->GetActorLocation();
			}
		}
		return Bounds.GetCenter();
	}
}

FVector UHyperManageTransform::CalcPivotAxis(const EAxis::Type DesiredAxis, const FVector& ViewVector, const FQuat& ActorQuat)
{
	auto ProcessAxes = [&](const FVector& VAxis, const FVector& Axis1, const FVector& Axis2) -> FVector
	{
		int Inverted = FMath::Sign(VAxis.Z);
		if (DesiredAxis == EAxis::Z) {
			return Inverted * VAxis;
		}
		float Check1 = FVector(Axis1.X, Axis1.Y, 0.f).GetSafeNormal() | ViewVector;
		float Check2 = FVector(Axis2.X, Axis2.Y, 0.f).GetSafeNormal() | ViewVector;
		if (FMath::Abs(Check1) >= FMath::Abs(Check2)) {
			return FMath::Sign(Check1) * ((DesiredAxis == EAxis::X) ? Axis1 : (Inverted * Axis2));
		}
		return FMath::Sign(Check2) * ((DesiredAxis == EAxis::X) ? Axis2 : (Inverted * -Axis1));
	};

	FVector XAxis = ActorQuat.GetAxisX();
	FVector YAxis = ActorQuat.GetAxisY();
	FVector ZAxis = ActorQuat.GetAxisZ();
	if (FMath::Abs(ZAxis | FVector::UpVector) >= UE_HALF_SQRT_2) {
		return ProcessAxes(ZAxis, XAxis, YAxis);
	} else if (FMath::Abs(YAxis | FVector::UpVector) >= UE_HALF_SQRT_2) {
		return ProcessAxes(YAxis, ZAxis, XAxis);
	}
	return ProcessAxes(XAxis, YAxis, ZAxis);
}

void UHyperManageTransform::CalcViewAxis(const EAxis::Type DesiredAxis, const FVector& ViewVector, const FQuat& ActorQuat, EAxis::Type& FoundAxis, bool& Inverted)
{
	auto ProcessAxes = [&](const FVector& VAxis, const FVector& Axis1, const FVector& Axis2) -> FVector
	{
		int Inverted = FMath::Sign(VAxis.Z);
		if (DesiredAxis == EAxis::Z) {
			return Inverted * VAxis;
		}
		float Check1 = FVector(Axis1.X, Axis1.Y, 0.f).GetSafeNormal() | ViewVector;
		float Check2 = FVector(Axis2.X, Axis2.Y, 0.f).GetSafeNormal() | ViewVector;
		if (FMath::Abs(Check1) >= FMath::Abs(Check2)) {
			return FMath::Sign(Check1) * ((DesiredAxis == EAxis::X) ? Axis1 : (Inverted * Axis2));
		}
		return FMath::Sign(Check2) * ((DesiredAxis == EAxis::X) ? Axis2 : (Inverted * -Axis1));
	};

	FVector XAxis = ActorQuat.GetAxisX();
	FVector YAxis = ActorQuat.GetAxisY();
	FVector ZAxis = ActorQuat.GetAxisZ();
	if (FMath::Abs(ZAxis | FVector::UpVector) >= UE_HALF_SQRT_2) {
		//return ProcessAxes(ZAxis, XAxis, YAxis);
	} else if (FMath::Abs(YAxis | FVector::UpVector) >= UE_HALF_SQRT_2) {
		//return ProcessAxes(YAxis, ZAxis, XAxis);
	}
	//return ProcessAxes(XAxis, YAxis, ZAxis);
}

void UHyperManageTransform::CalculateSimplePivot(FHyperManageTransformData& TransformData)
{
	if (TransformData.IsRot || TransformData.IsLoc) {
		if (TransformData.ViewRelative) {
			TransformData.PivotAxis = CalcPivotAxis(TransformData.TransformAxis, TransformData.ViewVector, TransformData.AnchorQuat);
		} else {
			TransformData.PivotAxis = GetQuatAxis(TransformData.TransformAxis, TransformData.AnchorQuat);
		}
	}
	if (TransformData.IsRot) {
		TransformData.PivotAngle = TransformData.Rot.GetComponentForAxis(TransformData.TransformAxis);
		TransformData.PivotQuat = FQuat(TransformData.PivotAxis, FMath::DegreesToRadians(TransformData.PivotAngle));
	}
	if (TransformData.IsLoc) {
		TransformData.PivotTranslation = TransformData.PivotAxis * (TransformData.Loc | FVector::OneVector);
	}
}

void UHyperManageTransform::CalculateTargetPivot(FHyperManageTransformData& TransformData)
{
	// More generic calculation of PivotQuat, PivotAxis and PivotAngle.  Useful in move where angles are more complex.
	// Q(end) = Q(rotation) * Q(start);  pivot quat = Q(delta) = Q(end) * Q(start)^-1

	// PivotQuat = Q(delta) = Q(end) * Q(start)^-1
	TransformData.PivotQuat = TransformData.Target->GetActorQuat() * TransformData.AnchorQuat.Inverse();
	TransformData.PivotQuat.Normalize();

	// pivot axis and angle to rotate around axis
	TransformData.PivotQuat.ToAxisAndAngle(TransformData.PivotAxis, TransformData.PivotAngle);
	TransformData.PivotAxis.Normalize();
	TransformData.PivotAngle = FMath::RadiansToDegrees(TransformData.PivotAngle);

	TransformData.PivotTranslation = TransformData.Target->GetActorLocation() - TransformData.PivotLoc;
	TransformData.IsRot = true;
}

void UHyperManageTransform::TranslateAroundPivot(FTransform& Transform, const FHyperManageTransformData& TransformData)
{
	if (TransformData.IsScale) { // scale the distance to pivot location
		FVector ScaledOffset = (Transform.GetLocation() - TransformData.PivotLoc) * TransformData.Scale;
		Transform.SetLocation(TransformData.PivotLoc + ScaledOffset);
	}
	if (TransformData.IsRot) { // rotate location around pivot axis by pivot angle
		FVector RotationLoc = TransformData.PivotLoc + (TransformData.PivotAxis * ((Transform.GetLocation() - TransformData.PivotLoc) | TransformData.PivotAxis));
		FVector RotationOffset = Transform.GetLocation() - RotationLoc;
		Transform.SetLocation(RotationLoc + RotationOffset.RotateAngleAxis(TransformData.PivotAngle, TransformData.PivotAxis));
	}
}

void UHyperManageTransform::ModifyTransform(FTransform& Transform, FHyperManageTransformData& TransformData)
{
	if (TransformData.IsScale) {
		Transform.MultiplyScale3D(TransformData.Scale);
	}
	if (TransformData.IsRot) { // Q(end) = Q(delta) * Q(start)
		Transform.SetRotation(TransformData.PivotQuat * Transform.GetRotation());
	}
	if (TransformData.IsLoc) {
		Transform.AddToTranslation(TransformData.PivotTranslation);
	}
}

void UHyperManageTransform::ProcessTransform(const TArray<AActor*>& Actors, const FHyperManageTransformData& TransformData)
{
	FQuat TargetQuat = TransformData.TargetRotation;
	if (TransformData.Target) {
		TargetQuat = TransformData.Target->GetRootComponent()->GetComponentTransform().GetRotation();
	}
	FHyperManageTransformData SingleTransformData = TransformData;
	for (const auto& Actor : Actors) {
		if (System->Selection->IsValidActor(Actor)) {
			// process all "root" components
			for (const auto& ActorComp : TInlineComponentArray<USceneComponent*>(Actor)) {
				USceneComponent* SceneComp = Cast<USceneComponent>(ActorComp);
				if (SceneComp && !SceneComp->GetAttachParent()) {
					FTransform Transform = SceneComp->GetComponentTransform();
					if (TransformData.WorldAlignment) {
						Transform = ComputeTransform(Transform, TransformData);
					} else if (TransformData.SetSame) {
						if (TransformData.GroupMode) { // use precalculated pivot values
							TranslateAroundPivot(Transform, TransformData);
							ModifyTransform(Transform, SingleTransformData);
						} else { // calculate pivot values needed for this single actor
							Transform.SetRotation(TargetQuat);
						}
					} else {
						if (TransformData.GroupMode) { // use precalculated pivot values
							TranslateAroundPivot(Transform, TransformData);
						} else { // calculate pivot values needed for this single actor
							SingleTransformData.AnchorQuat = SceneComp->GetComponentQuat();
							CalculateSimplePivot(SingleTransformData);
						}
						ModifyTransform(Transform, SingleTransformData);
					}
					TransformComponent(SceneComp, Transform);
				}
			}
			System->Selection->RefreshMaterial(Actor);
		}
	}
}

void UHyperManageTransform::GetActorOriginAndSize(AActor* Actor, FVector& Origin, FVector& Size)
{
	FVector BoxExtent;
	Actor->GetActorBounds(false, Origin, BoxExtent);
	FBox LocalBox = Actor->CalculateComponentsBoundingBoxInLocalSpace();
	Size = (LocalBox.Max - LocalBox.Min) * Actor->GetActorScale3D();
}

void UHyperManageTransform::AlignToActor(AActor* Anchor, int Position, const EAxis::Type Axis)
{
	if (!Anchor) {
		return;
	}

	// Position >0 = Top, Left, Front
	// Position 0 = Center, Center, Center
	// Position <0 = Bottom, Right, Back

	// Calculate plane from Actor based on it's current rotation, using Axis and Position
	FVector Normal;
	FVector AnchorOrigin;
	FVector AnchorSize;
	GetActorOriginAndSize(Anchor, AnchorOrigin, AnchorSize);
	if (System->Config->MMConfig.IsViewBased) {
		//CalcViewAxis(const EAxis::Type DesiredAxis, const FVector& ViewVector, const FQuat& ActorQuat, EAxis::Type& FoundAxis, bool& Inverted)
		Normal = CalcPivotAxis(Axis, System->GetCameraViewVector(), Anchor->GetActorQuat());
		//!!! use custom CalcPivotAxis (CalcViewAxis) that returns non-inverted Normal and Loc



	} else {
		Normal = Anchor->GetActorQuat().GetAxisZ();
		//!!! Return natural axis of Anchor
	}

	FPlane AlignPlane = FPlane(Normal, AnchorOrigin);
	// Adjust AlignPlane based on Position
	if (Position > 0) {
		AlignPlane.W += AnchorSize.X; // !!! use X, Y or Z
	} else if (Position < 0) {

	}



	TArray<AActor*> Actors;
	System->Selection->SelectedActorsNoTarget(Actors);
	for (auto Actor : Actors) {
		if (Actor == Anchor) {
			continue;
		}
		// Calculate closest distance from other objects in selection to plane and then translate their position along that vector towards the plane
		// This set of operations should be cached somehow and passed on to server to perform the transform

		//!!!
	}




	/*
	// calculate top plane for AnchorActor
	FVector Normal;
	if (System->Config->MMConfig.IsViewBased) {
		//CalcViewAxis(const EAxis::Type DesiredAxis, const FVector& ViewVector, const FQuat& ActorQuat, EAxis::Type& FoundAxis, bool& Inverted)
		Normal = CalcPivotAxis(EAxis::Z, System->GetCameraViewVector(), System->Selection->AnchorActor->GetActorQuat());
	} else {
		Normal = System->Selection->AnchorActor->GetActorQuat().GetAxisZ();
	}
	FPlane AlignPlane = FPlane(Normal, Loc);


	// move all other actors up or down (perpendicular to the alignment plane) so that their top is on the plane
	*/
}



FTransform UHyperManageTransform::ComputeTransform(FTransform Transform, const FHyperManageTransformData& Data)
{
	if (Data.WorldAlignment) {
		if (!FMath::IsFinite(Data.AlignmentGridCm) || Data.AlignmentGridCm < 1.0 ||
			!FMath::IsFinite(Data.AlignmentAngle) || Data.AlignmentAngle < 0.1 || Data.AlignmentAngle > 180.0) return Transform;
		const FVector ReferenceLocation = Data.GroupMode ? Data.PivotLoc : Transform.GetLocation();
		const FQuat ReferenceRotation = Data.GroupMode ? Data.AnchorQuat : Transform.GetRotation();
		FVector SnappedLocation = ReferenceLocation;
		FRotator SnappedRotation = ReferenceRotation.Rotator();
		if (Data.SnapWorldPosition) {
			SnappedLocation.X = FMath::GridSnap(ReferenceLocation.X, Data.AlignmentGridCm);
			SnappedLocation.Y = FMath::GridSnap(ReferenceLocation.Y, Data.AlignmentGridCm);
		}
		if (Data.SnapWorldRotation) {
			SnappedRotation.Pitch = FMath::GridSnap(SnappedRotation.Pitch, Data.AlignmentAngle);
			SnappedRotation.Yaw = FMath::GridSnap(SnappedRotation.Yaw, Data.AlignmentAngle);
			SnappedRotation.Roll = FMath::GridSnap(SnappedRotation.Roll, Data.AlignmentAngle);
		}
		if (Data.LevelWorldRotation) { SnappedRotation.Pitch = 0.0; SnappedRotation.Roll = 0.0; }
		const FQuat Delta = SnappedRotation.Quaternion() * ReferenceRotation.Inverse();
		Transform.SetLocation(SnappedLocation + Delta.RotateVector(Transform.GetLocation() - ReferenceLocation));
		Transform.SetRotation((Delta * Transform.GetRotation()).GetNormalized());
		return Transform;
	}
	FHyperManageTransformData Local = Data;
	if (Data.SetSame) {
		if (Data.GroupMode) { TranslateAroundPivot(Transform, Data); ModifyTransform(Transform, Local); }
		else Transform.SetRotation(Data.TargetRotation);
	} else {
		if (Data.GroupMode) TranslateAroundPivot(Transform, Data);
		else { Local.AnchorQuat = Transform.GetRotation(); CalculateSimplePivot(Local); }
		ModifyTransform(Transform, Local);
	}
	Transform.NormalizeRotation();
	return Transform;
}

bool UHyperManageTransform::MakeWorldOffset(const FVector& Meters, FHyperManageTransformData& Data)
{
	if (Meters.ContainsNaN() || Meters.GetAbsMax() > 1000.0 || Meters.IsNearlyZero(0.000001)) return false;
	Data = FHyperManageTransformData();
	Data.Loc = Meters * 100.0;
	Data.PivotTranslation = Data.Loc;
	Data.IsLoc = true;
	Data.GroupMode = true;
	Data.ViewRelative = false;
	return true;
}
