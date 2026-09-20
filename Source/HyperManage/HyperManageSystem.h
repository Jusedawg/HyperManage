#pragma once

#include "CoreMinimal.h"

#include "FGPlayerController.h"
#include "FGPlayerState.h"
#include "Buildables/FGBuildable.h"
#include "HyperManageSystem.generated.h"

UENUM(BlueprintType)
enum EIncrementSize
{
	Tiny		UMETA(DisplayName = "Tiny"),
	Medium		UMETA(DisplayName = "Medium"),
	Large		UMETA(DisplayName = "Large"),
	Huge		UMETA(DisplayName = "Huge")
};

UENUM(BlueprintType)
enum EActionNameIdx
{
	NoAction			UMETA(DisplayName = "No Action"),
	SelectTarget		UMETA(DisplayName = "Select Target"),
	DeselectTarget		UMETA(DisplayName = "Deselect Target"),
	Undo				UMETA(DisplayName = "Undo"),
	ChangeIncSize		UMETA(DisplayName = "Change Increment Size"),
	KnowNotes			UMETA(DisplayName = "Know Notes"),
	ShowTools			UMETA(DisplayName = "Show Tool UI"),
	SetAnchor			UMETA(DisplayName = "Set Anchor"),
	SetTarget			UMETA(DisplayName = "Set Target"),

	// keep SpinLeft to Grow together
	SpinLeft			UMETA(DisplayName = "Spin Left"),
	SpinRight			UMETA(DisplayName = "Spin Right"),
	MoveUp				UMETA(DisplayName = "Move Up"),
	MoveDown			UMETA(DisplayName = "Move Down"),
	MoveLeft			UMETA(DisplayName = "Move Left"),
	MoveRight			UMETA(DisplayName = "Move Right"),
	MoveAway			UMETA(DisplayName = "Move Away"),
	MoveToward			UMETA(DisplayName = "Move Toward"),
	RollLeft			UMETA(DisplayName = "Roll Left"),
	RollRight			UMETA(DisplayName = "Roll Right"),
	PitchAway			UMETA(DisplayName = "Pitch Away"),
	PitchToward			UMETA(DisplayName = "Pitch Toward"),
	Shrink				UMETA(DisplayName = "Shrink"),
	Grow				UMETA(DisplayName = "Grow"),
	// keep SpinLeft to Grow together

	// toolbar actions
	AlignLeft			UMETA(DisplayName = "Align Left Edges To Anchor"),
	AlignLRCenter		UMETA(DisplayName = "Align Left-Right Centers To Anchor"),
	AlignRight			UMETA(DisplayName = "Align Right Edges To Anchor"),
	SpaceLR				UMETA(DisplayName = "Space Objects Equally Left To Right"),
	StackLR				UMETA(DisplayName = "Stack Objects Left To Right"),
	LockScaleLR			UMETA(DisplayName = "Lock Scaling Left To Right"),
	AlignTop			UMETA(DisplayName = "Align Tops To Anchor"),
	AlignTBCenter		UMETA(DisplayName = "Align Top-Bottom Centers To Anchor"),
	AlignBottom			UMETA(DisplayName = "Align Bottoms To Anchor"),
	SpaceTB				UMETA(DisplayName = "Space Objects Equally Top To Bottom"),
	StackTB				UMETA(DisplayName = "Stack Objects Top To Bottom"),
	LockScaleTB			UMETA(DisplayName = "Lock Scaling Top To Bottom"),
	AlignFront			UMETA(DisplayName = "Align Front Edges To Anchor"),
	AlignFBCenter		UMETA(DisplayName = "Align Front-Back Centers To Anchor"),
	AlignBack			UMETA(DisplayName = "Align Back Edges To Anchor"),
	SpaceFB				UMETA(DisplayName = "Space Objects Equally Front To Back"),
	StackFB				UMETA(DisplayName = "Stack Objects Front To Back"),
	LockScaleFB			UMETA(DisplayName = "Lock Scaling Front To Back"),

	SameRotation		UMETA(DisplayName = "Set Selection to Same Rotation as Target"),
	SameScale			UMETA(DisplayName = "Set Selection to Same Scale as Target"),
	SamePaint			UMETA(DisplayName = "Set Selection to Same Paint as Target"),

	Connect				UMETA(DisplayName = "Connect Anchor Output to Target Input"),
	Disconnect			UMETA(DisplayName = "Disconnect Anchor Outputs from Target Inputs"),
	SelectBoxSides		UMETA(DisplayName = "Select items between Anchor and Target Sides"),
	SelectBoxPivot		UMETA(DisplayName = "Select items between Anchor and Target Centers"),
	MoveSelection		UMETA(DisplayName = "Move Selection from Anchor to Target"),
	CopySelection		UMETA(DisplayName = "Copy Selection from Anchor to Target"),
	NewSelection		UMETA(DisplayName = "Start New Selection"),
	DeleteSelection		UMETA(DisplayName = "Delete Selection"),
	SaveSelection		UMETA(DisplayName = "Save Selection"),
	LoadSelection		UMETA(DisplayName = "Load Selection"),
	ClearUndo			UMETA(DisplayName = "Clear Saved Undo Information"),

	IsGrouped			UMETA(DisplayName = "Group Selection"),
	IsViewBased			UMETA(DisplayName = "Use View Based Actions"),
	NextHologram		UMETA(DisplayName = "Switch To Next Hologram Style"),
	Settings			UMETA(DisplayName = "Settings"),
	SnapWorldXY UMETA(DisplayName = "Snap To World Grid XY"),
	SnapWorldRotation UMETA(DisplayName = "Snap World Rotation"),
	LevelWorldRotation UMETA(DisplayName = "Level Pitch And Roll"),
	Redo UMETA(DisplayName = "Redo" ),
	MatchAnchorX UMETA(DisplayName = "Match Anchor X"),
	MatchAnchorY UMETA(DisplayName = "Match Anchor Y"),
	MatchAnchorZ UMETA(DisplayName = "Match Anchor Z"),
	SnapWorldZ UMETA(DisplayName = "Snap To Height Grid")
};

class AHyperManageEquip;
class UHyperManageRCO;

// UHyperManageSystem -----------------------------------------------------------------------------

UCLASS(BlueprintType)
class HYPERMANAGE_API UHyperManageSystem : public UObject
{
	friend class FHyperManageSelectionHistoryTest;
	GENERATED_BODY()

private:
	UPROPERTY(Transient)
	TArray<AHyperManageEquip*> ActiveEquipment;

	UPROPERTY(Transient)
	AFGPlayerController* LocalController;

	UPROPERTY(Transient)
	UWorld* CurrentWorld;

	UPROPERTY(Transient)
	UHyperManageRCO* MMRCO;

	UFUNCTION()
	void Initialize(UGameInstance* GameInstance, UWorld* World);

public:
	static UHyperManageSystem* HyperManageSystemSingleton;

	FGuid SystemId;

	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	static UHyperManageSystem* Get();

	static UHyperManageSystem* GetForWorld(UWorld* World);
	static void ReleaseWorld(UWorld* World);

	UFUNCTION()
	AHyperManageEquip* GetEquip();

	UFUNCTION()
	void AddActiveEquipment(AHyperManageEquip* Equip);

	UFUNCTION()
	void RemoveActiveEquipment(AHyperManageEquip* Equip);

	UFUNCTION()
	UHyperManageRCO* GetMMRCO();

	UFUNCTION()
	AFGPlayerController* GetLocalController();

	UFUNCTION()
	UWorld* GetWorld() const override;

	UFUNCTION()
	bool IsIdMatch(const FGuid& CheckId);

	UFUNCTION()
	FVector GetCameraViewVector();

public: // Components =============================================================================
	UPROPERTY(BlueprintReadOnly, Category = "HyperManage Component")
	class UHyperManageUndo* Undo;

	UPROPERTY(BlueprintReadOnly, Category = "HyperManage Component")
	class UHyperManageConfiguration* Config;

	UPROPERTY(BlueprintReadOnly, Category = "HyperManage Component")
	class UHyperManageSelection* Selection;

	UPROPERTY(BlueprintReadOnly, Category = "HyperManage Component")
	class UHyperManageTransform* Transform;

	UPROPERTY(BlueprintReadOnly, Category = "HyperManage Component")
	class UHyperManageUI* UI;

	UPROPERTY(BlueprintReadOnly, Category = "HyperManage Component")
	class UHyperManageAction* Action;

	UPROPERTY(BlueprintReadOnly, Category = "HyperManage Component")
	class UHyperManageInput* Input;

private:
	void BasicTransform(EActionNameIdx ActionIndex);

public:
	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	void ExecuteAction(EActionNameIdx ActionIndex);

public:
};



// UHyperManageComponent --------------------------------------------------------------------------

UCLASS()
class HYPERMANAGE_API UHyperManageComponent : public UObject
{
	GENERATED_BODY()

protected:
	class UHyperManageSystem* System;

public:
	virtual void Init() { System = Cast<UHyperManageSystem>(GetOuter()); }

public:
};

template<typename C>
C* InitComponent(UHyperManageSystem* System) {
	UHyperManageComponent* Comp = NewObject<C>(System);
	Comp->Init();
	return Cast<C>(Comp);
}
