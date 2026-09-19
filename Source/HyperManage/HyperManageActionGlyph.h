#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HyperManageActionGlyph.generated.h"

UCLASS()
class HYPERMANAGE_API UHyperManageActionGlyph : public UUserWidget
{
 GENERATED_BODY()
public:
 int32 Kind = 0;
 bool Compact = false;
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool ParentEnabled) const override;
};
