#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HyperManageClipboardWidget.generated.h"

UCLASS()
class HYPERMANAGE_API UHyperManageClipboardWidget : public UUserWidget
{
	GENERATED_BODY()
	friend class FHyperManageClipboardLayoutTest;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool ParentEnabled) const override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> ShortcutsText;
	UPROPERTY(Transient) TObjectPtr<class UFont> HandwrittenFont;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> AnchorText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> TargetText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> SelectionText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> CountText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> UndoText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> RedoText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> ScaleText;
	UPROPERTY(Transient) TObjectPtr<class UTextBlock> NotesText;
	void UpdateReference(const class UHyperManageConfiguration& Configuration, int32 Count);
};
