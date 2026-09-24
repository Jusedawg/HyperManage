#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Blueprint/UserWidget.h"
#include "UI/FGInteractWidget.h"
#include "HyperManageSystem.h"
#include "HyperManageToolWidget.generated.h"

UCLASS()
class HYPERMANAGE_API UButtonProxy : public UUserWidget
{
	GENERATED_BODY()

public:
	static UButtonProxy* Create(EActionNameIdx AAction)
	{
		UButtonProxy* BtnProxy = NewObject<UButtonProxy>();
		BtnProxy->ToolAction = AAction;
		return BtnProxy;
	}

	EActionNameIdx ToolAction;

	UFUNCTION()
	void ClickEvent();
 UPROPERTY(Transient) TObjectPtr<UButton> SplitButton;
 EActionNameIdx DecreaseAction = EActionNameIdx::NoAction;
 UFUNCTION() void ClickSplitEvent();

public:
};

UCLASS(config = HyperManage)
class HYPERMANAGE_API UHyperManageToolWidget : public UFGInteractWidget
{
	friend class FHyperManageToolbarLayoutTest;
	GENERATED_BODY()
	
protected:
	UPROPERTY()
	TArray<UButtonProxy*> ButtonProxyArray;

	void CompactApplyButton(UButton* Button, bool Toolbar = false);
	void HookWidget(EActionNameIdx ToolAction, UButton* Button, FString ToolTip);

	virtual void NativeConstruct() override;
	void RepairToolbarLayout();
	void RepairQuickActions();
	UPROPERTY(Transient) TObjectPtr<class UVerticalBox> QuickActionHost;
	UPROPERTY(Transient) TObjectPtr<class UNamedSlot> DockedTray;
	UPROPERTY(Transient) TObjectPtr<UButton> RemoveBoxEdgesButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RemoveBoxCentersButton;
	UFUNCTION() void RemoveBoxEdges();
	UFUNCTION() void RemoveBoxCenters();
 UFUNCTION() void ChangeAutoAnchor(bool Enabled);
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> SelectionSlotPicker;
	UPROPERTY(Transient) TObjectPtr<class UEditableTextBox> SlotNameField;
	int32 EditingSlotName = 0;
	void RefreshSlotNames();
	UFUNCTION() void CommitSlotName(const FText& Value, ETextCommit::Type CommitMethod);
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> SelectionSlotStatus;
	UFUNCTION() void ChangeSelectionSlot(FString Value, ESelectInfo::Type SelectionType);
	UPROPERTY(Transient) TObjectPtr<UButton> AddSlotButton;
	UFUNCTION() void AddSelectionSlot();
	UPROPERTY(Transient) TObjectPtr<UButton> RemoveSlotButton;
	UFUNCTION() void RemoveSelectionSlot();
 UPROPERTY(Transient) TObjectPtr<UButton> ForgetSlotButton;
 UFUNCTION() void ForgetSelectionSlot();
 UPROPERTY(Transient) TObjectPtr<UButton> BlueprintSlotButton;
 UFUNCTION() void RememberBlueprintSlot();
 UFUNCTION() void ReviewDismantleRefunds();
 UPROPERTY(Transient) TObjectPtr<class UCanvasPanel> RefundDrawerHost;
 UPROPERTY(Transient) TObjectPtr<class UBorder> RefundDrawerPanel;
 bool RefundDrawerOpen = false;
 float RefundDrawerProgress = 0.f;
 void BuildRefundDrawer(class UNamedSlot* Window);
 void UpdateRefundDrawer(float DeltaTime, float ViewportWidth);
 UFUNCTION() void CloseRefundDrawer();
 UPROPERTY(Transient) TObjectPtr<class UScrollBox> RefundReviewScroll;
 UPROPERTY(Transient) TObjectPtr<class UTextBlock> RefundReviewText;
 void SetRefundReviewReport(const FString& Report);
 TSet<TWeakObjectPtr<AActor>> ReviewedSelection;
 TWeakObjectPtr<AActor> ReviewedTarget;
 bool TrackRefundSelection = false;
 void CaptureRefundSelection(const TArray<AActor*>& Actors, AActor* Target);
 void CheckRefundSelection(const TArray<AActor*>& Actors, AActor* Target, bool Pending);
	UPROPERTY(Transient) TObjectPtr<UButton> UndoButton;
	UPROPERTY(Transient) TObjectPtr<UButton> RedoButton;
	UPROPERTY(Transient) TObjectPtr<class UExpandableArea> HistoryArea;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> HistoryDetails;
	uint64 HistoryRevision = MAX_uint64;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> OffsetAxes;
	UPROPERTY(Transient) TObjectPtr<USpinBox> OffsetX;
	UPROPERTY(Transient) TObjectPtr<USpinBox> OffsetY;
	UPROPERTY(Transient) TObjectPtr<USpinBox> OffsetZ;
	UPROPERTY(Transient) TObjectPtr<UButton> ApplyOffsetButton;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> OffsetStatus;
	UFUNCTION() void ApplyWorldOffset();
	UFUNCTION() void ClearWorldOffset();
 UPROPERTY(Transient) TArray<TObjectPtr<class UCheckBox>> PositionAxes;
 uint8 GetPositionAxisMask() const;
 UPROPERTY(Transient) TObjectPtr<USpinBox> PositionX;
 UPROPERTY(Transient) TObjectPtr<USpinBox> PositionY;
 UPROPERTY(Transient) TObjectPtr<USpinBox> PositionZ;
 UPROPERTY(Transient) TObjectPtr<UButton> ApplyPositionButton;
 UPROPERTY(Transient) TObjectPtr<UButton> ReadPositionButton;
 UPROPERTY(Transient) TObjectPtr<class UTextBlock> PositionStatus;
 UFUNCTION() void ApplyWorldPosition();
 UFUNCTION() void ReadWorldPosition();
	UPROPERTY(Transient) TObjectPtr<USpinBox> OffsetYaw;
	UPROPERTY(Transient) TObjectPtr<USpinBox> OffsetPitch;
	UPROPERTY(Transient) TObjectPtr<USpinBox> OffsetRoll;
	UPROPERTY(Transient) TObjectPtr<UButton> ApplyRotationButton;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> RotationStatus;
	UFUNCTION() void ApplyWorldRotationOffset();
	UFUNCTION() void ClearRotationOffset();
 UPROPERTY(Transient) TArray<TObjectPtr<class UCheckBox>> OrientationAxes;
 uint8 GetOrientationAxisMask() const;
 UPROPERTY(Transient) TObjectPtr<USpinBox> OrientationYaw;
 UPROPERTY(Transient) TObjectPtr<USpinBox> OrientationPitch;
 UPROPERTY(Transient) TObjectPtr<USpinBox> OrientationRoll;
 UPROPERTY(Transient) TObjectPtr<UButton> ApplyOrientationButton;
 UPROPERTY(Transient) TObjectPtr<UButton> ReadOrientationButton;
 UPROPERTY(Transient) TObjectPtr<class UTextBlock> OrientationStatus;
 UFUNCTION() void ApplyWorldOrientation();
 UFUNCTION() void ReadWorldOrientation();
	UPROPERTY(Transient) TObjectPtr<USpinBox> ScaleX;
	UPROPERTY(Transient) TObjectPtr<USpinBox> ScaleY;
	UPROPERTY(Transient) TObjectPtr<USpinBox> ScaleZ;
	UPROPERTY(Transient) TObjectPtr<UButton> ApplyScaleButton;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> ScalePreset;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> ScaleStatus;
	UFUNCTION() void ApplyScalePercent();
	UPROPERTY(Transient) TObjectPtr<UButton> ReadScaleButton;
	UFUNCTION() void ReadScale();
	UFUNCTION() void ResetScaleFields();
 UPROPERTY(Transient) TArray<TObjectPtr<UButton>> DistributionButtons;
 UPROPERTY(Transient) TObjectPtr<class UTextBlock> ReferenceMeasurements;
 UPROPERTY(Transient) TArray<TObjectPtr<UButton>> TypeFilterButtons;
 UFUNCTION() void KeepAnchorType();
 UFUNCTION() void RemoveAnchorType();
 UFUNCTION() void DistributeX();
 UFUNCTION() void DistributeY();
 UFUNCTION() void DistributeZ();
	UFUNCTION() void ClearScalePreset(float Value);
	UFUNCTION() void ChangeScalePreset(FString Value, ESelectInfo::Type SelectionType);
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> AnchorAlignmentButtons;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> AnchorAlignmentStatus;
	UPROPERTY(Transient) TObjectPtr<USpinBox> HeightGridValue;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> HeightGridPreset;
	UFUNCTION() void CommitHeightGridValue(float Value, ETextCommit::Type CommitMethod);
	UFUNCTION() void ChangeHeightGridPreset(FString Value, ESelectInfo::Type SelectionType);
	float TrayOpenTime = 0.f;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	UPROPERTY(Transient) TObjectPtr<USpinBox> MovementValue;
	UPROPERTY(Transient) TObjectPtr<USpinBox> RotationValue;
	UPROPERTY(Transient) TObjectPtr<USpinBox> GridValue;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> PrecisionProfileLabel;
	UFUNCTION() void CommitMovementValue(float Value, ETextCommit::Type CommitMethod);
	UFUNCTION() void CommitRotationValue(float Value, ETextCommit::Type CommitMethod);
	UFUNCTION() void CommitGridValue(float Value, ETextCommit::Type CommitMethod);
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> RotationPreset;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> MovementPreset;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> GridPreset;
	UFUNCTION() void ChangeRotationPreset(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void ChangeMovementPreset(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION() void ChangeGridPreset(FString Value, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void CloseTools();

public:
	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignLeft;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignLRCenter;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignRight;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSpaceLR;

	UPROPERTY(meta = (BindWidget))
	UButton* btnStackLR;

	UPROPERTY(meta = (BindWidget))
	UButton* btnLockScaleLR;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignTop;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignTBCenter;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignBottom;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSpaceTB;

	UPROPERTY(meta = (BindWidget))
	UButton* btnStackTB;

	UPROPERTY(meta = (BindWidget))
	UButton* btnLockScaleTB;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignFront;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignFBCenter;

	UPROPERTY(meta = (BindWidget))
	UButton* btnAlignBack;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSpaceFB;

	UPROPERTY(meta = (BindWidget))
	UButton* btnStackFB;

	UPROPERTY(meta = (BindWidget))
	UButton* btnLockScaleFB;

	UPROPERTY(meta = (BindWidget))
	UButton* btnIsGrouped;

	UPROPERTY(meta = (BindWidget))
	UButton* btnIsViewBased;

	UPROPERTY(meta = (BindWidget))
	UButton* btnNextHologram;
	
	UPROPERTY(meta = (BindWidget))
	UButton* btnSettings;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSameRotation;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSameScale;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSamePaint;

	UPROPERTY(meta = (BindWidget))
	UButton* btnConnect;

	UPROPERTY(meta = (BindWidget))
	UButton* btnDisconnect;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSelectBoxSides;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSelectBoxPivot;

	UPROPERTY(meta = (BindWidget))
	UButton* btnMoveSelection;

	UPROPERTY(meta = (BindWidget))
	UButton* btnCopySelection;

	UPROPERTY(meta = (BindWidget))
	UButton* btnNewSelection;

	UPROPERTY(meta = (BindWidget))
	UButton* btnDeleteSelection;

	UPROPERTY(meta = (BindWidget))
	UButton* btnSaveSelection;

	UPROPERTY(meta = (BindWidget))
	UButton* btnLoadSelection;

	UPROPERTY(meta = (BindWidget))
	UButton* btnClearUndo;

public:
};
