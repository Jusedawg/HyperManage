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
	FString LastStatus;
	FString LastShortcuts;
};
