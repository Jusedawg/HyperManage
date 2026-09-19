#pragma once

#include "CoreMinimal.h"
#include "FGRemoteCallObject.h"
#include "HyperManageEquip.h"
#include "HyperManageUndo.h"
#include "HyperManageRCO.generated.h"

UCLASS()
class HYPERMANAGE_API UHyperManageRCO : public UFGRemoteCallObject
{
	GENERATED_BODY()

private:
	void RemoveDuplicateWires(TArray<class AFGBuildableWire*>& Wires);

	void ProcessWires(const TArray<class AFGBuildableWire*>& Wires);
	AHyperManageEquip* GetRequestEquipment() const;
	UPROPERTY(Transient) TMap<FGuid, FHyperManageLightweightRef> LightweightHandles;
	FHyperManageLightweightRef* ResolveLightweight(const FHyperManageLightweightRef& Ref, AFGLightweightBuildableSubsystem* Subsystem);

public:
	UPROPERTY(Replicated)
	bool Dummy = true;

		void RequestTransform(const TArray<AActor*>& Actors, FHyperManageTransformData Data);
	void RequestPaint(const TArray<AActor*>& Actors, const FFactoryCustomizationData& Paint);
	void RequestUndo(const FUndoInfo& Undo);
	void RequestAbsoluteTransforms(const TArray<AActor*>& Actors, const FVector& Scale);

	UFUNCTION(Server, Reliable)
	void ServerTransformLightweights(const TArray<FHyperManageLightweightRef>& Refs, FHyperManageTransformData Data);
	UFUNCTION(Server, Reliable)
	void ServerEditLightweights(const TArray<FHyperManageLightweightEdit>& Edits);
	UFUNCTION(Client, Reliable)
	void ClientLightweightResults(const TArray<FHyperManageLightweightEdit>& Results);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Reliable, Server, WithValidation, Category = "HyperManage")
	void ServerTransformActors(const TArray<AActor*>& Actors, FHyperManageTransformData TransformData);

	UFUNCTION(Reliable, Server, WithValidation, Category = "HyperManage")
	void ServerUndoAction(const FUndoInfo& UndoInfo);

	UFUNCTION(Reliable, Server, WithValidation, Category = "HyperManage")
	void ServerPrepareActors(const TArray<AActor*>& Actors);

	UFUNCTION(Reliable, Server, WithValidation, Category = "HyperManage")
	void ServerPaintActors(const TArray<AActor*>& Actors, const FFactoryCustomizationData& PaintData);

	UFUNCTION(Reliable, Server, WithValidation, Category = "HyperManage")
	void ServerHandleConnect(const FGuid& Id, bool IsConnection, AActor* OutputActor, AActor* InputActor);

public:
};

