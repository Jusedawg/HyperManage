#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FGLightweightBuildableSubsystem.h"
#include "HyperManageLightweight.generated.h"

USTRUCT()
struct FHyperManageLightweightRef
{
	GENERATED_BODY()
	UPROPERTY() FGuid SelectionId;
	UPROPERTY() TSubclassOf<AFGBuildable> BuildableClass;
	UPROPERTY() int32 Index = INDEX_NONE;
	UPROPERTY() FTransform ExpectedTransform = FTransform::Identity;
	UPROPERTY() TSubclassOf<UFGRecipe> Recipe;

	bool Matches(const FRuntimeBuildableInstanceData* Data) const;
};

USTRUCT()
struct FHyperManageLightweightEdit
{
	GENERATED_BODY()
	UPROPERTY() FHyperManageLightweightRef Ref;
	UPROPERTY() FTransform Transform = FTransform::Identity;
	UPROPERTY() bool Paint = false;
	UPROPERTY() bool Accepted = false;
	UPROPERTY() FFactoryCustomizationData Customization;
};

// A local selection handle only. The actual building stays in the game's saved lightweight subsystem.
UCLASS(Transient, NotBlueprintable)
class HYPERMANAGE_API AHyperManageLightweightProxy : public AActor
{
	GENERATED_BODY()
public:
	AHyperManageLightweightProxy();
	UPROPERTY(Transient) FHyperManageLightweightRef Ref;
	UPROPERTY(Transient) FFactoryCustomizationData Customization;
	bool Available = true;
	void Initialize(TSubclassOf<AFGBuildable> Class, int32 Index, const FRuntimeBuildableInstanceData& Data);
	void ApplyAcknowledgement(const FHyperManageLightweightRef& UpdatedRef, const FFactoryCustomizationData& Data);
	bool IsAvailable() const;
	bool IsPending() const;
	void BeginRequest();
private:
	double AcknowledgedAt = -100.0;
	double RequestedAt = -100.0;
	bool Pending = false;
};

namespace HyperManageLightweight
{
	bool IsValidTransform(const FTransform& Transform);
	// Add before removing: a one-part blueprint must retain its proxy throughout replacement.
	bool Replace(AFGLightweightBuildableSubsystem* Subsystem, FHyperManageLightweightRef& Ref, const FTransform& Transform);
}
