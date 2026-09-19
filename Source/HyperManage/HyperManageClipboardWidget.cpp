#include "HyperManageClipboardWidget.h"
#include "HyperManageSystem.h"
#include "HyperManageConfig.h"
#include "HyperManageSelection.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"

TSharedRef<SWidget> UHyperManageClipboardWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
	if (!WidgetTree->RootWidget) {
		auto* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Root;
		auto* Board = WidgetTree->ConstructWidget<UCanvasPanel>();
		auto* BoardSlot = Root->AddChildToCanvas(Board);
		BoardSlot->SetAnchors(FAnchors(0.f, 0.5f));
		BoardSlot->SetPosition(FVector2D(20, -288));
		Board->SetRenderTransformPivot(FVector2D::ZeroVector);
		Board->SetRenderScale(FVector2D(1.2f, 1.2f));
		BoardSlot->SetSize(FVector2D(320, 480));
		auto* Art = WidgetTree->ConstructWidget<UImage>();
		Art->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr, TEXT("/HyperManage/UI/Jusedawg/T_Clipboard.T_Clipboard")));
		Board->AddChildToCanvas(Art)->SetSize(FVector2D(320, 480));
		HandwrittenFont = NewObject<UFont>(this);
		HandwrittenFont->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry Face(TEXT("Regular"));
		Face.Font = FFontData(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("HyperManage"))->GetBaseDir(), TEXT("Art/Clipboard/Kalam-Regular.ttf")), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		HandwrittenFont->CompositeFont.DefaultTypeface.Fonts.Add(Face);
		auto AddText = [&](const TCHAR* Text, float Y, float Height, int32 Size) {
			auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(Text));
			Label->SetFont(FSlateFontInfo(HandwrittenFont, Size, TEXT("Regular")));
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.035f, 0.07f, 0.10f)));
			Label->SetWrapTextAt(234.f);
			auto* TextSlot = Board->AddChildToCanvas(Label);
			TextSlot->SetPosition(FVector2D(45, Y));
			TextSlot->SetSize(FVector2D(234, Height));
			return Label;
		};
		AddText(TEXT("Jusedawg's Notes"), 106, 34, 16);
		StatusText = AddText(TEXT("Selection ready"), 140, 75, 11);
		ShortcutsText = AddText(TEXT(""), 215, 100, 9);
		AddText(TEXT("Default keys"), 320, 24, 9);
		auto Caption = [&](const TCHAR* Text, float X) {
			auto* Label = AddText(Text, 399, 39, 9);
			Label->SetWrapTextAt(76);
			auto* LabelSlot = Cast<UCanvasPanelSlot>(Label->Slot);
			LabelSlot->SetPosition(FVector2D(X, 399)); LabelSlot->SetSize(FVector2D(76, 39));
		};
		Caption(TEXT("Ctrl + keys\nLift / spin"), 45);
		Caption(TEXT("Alt + keys\nMove"), 125);
		Caption(TEXT("Shift + keys\nPitch / roll"), 205);
		for (int32 Index = 0; Index < 4; ++Index) {
			const TCHAR* Keys[] = {TEXT("I"), TEXT("J"), TEXT("K"), TEXT("L")};
			auto* Label = AddText(Keys[Index], 340, 20, 9);
			auto* LabelSlot = Cast<UCanvasPanelSlot>(Label->Slot);
			LabelSlot->SetPosition(FVector2D(Index == 0 ? 229 : 205 + (Index - 1) * 24, Index == 0 ? 320 : 340)); LabelSlot->SetSize(FVector2D(20, 20));
		}
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	return Super::RebuildWidget();
}

void UHyperManageClipboardWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	auto* System = UHyperManageSystem::Get();
	if (!System || !System->Config || !System->Selection || !StatusText || !ShortcutsText) return;
	const auto& Config = System->Config->MMConfig;
	if (!Config.IncrementSettings.IsValidIndex(Config.IncrementSize)) return;
	const auto& Increment = Config.IncrementSettings[Config.IncrementSize];
	const FString Status = FString::Printf(TEXT("Selected: %d   |   %s\nMove: %gm   Rotate: %g deg\nGrid: %gm   |   %s"), System->Selection->SelectCount(),
		Config.IsGrouped ? TEXT("Group") : TEXT("Individual"), Increment.CentimetersToMove / 100.f, Increment.DegreesToRotate,
		Config.AlignmentGridCm / 100.f, Config.IsViewBased ? TEXT("View axes") : TEXT("Object axes"));
	if (Status != LastStatus) { StatusText->SetText(FText::FromString(Status)); LastStatus = Status; }
	auto Binding = [&](EActionNameIdx Action) {
		for (const auto& Key : System->Config->MMKeyConfigs.ActionKeys) {
			if (Key.ActionIndex != Action) continue;
			FString Name = Key.KeyName;
			if (Name == TEXT("LeftMouseButton")) Name = TEXT("LMB");
			else if (Name == TEXT("RightMouseButton")) Name = TEXT("RMB");
			return FString(Key.Ctrl ? TEXT("Ctrl+") : TEXT("")) + (Key.Shift ? TEXT("Shift+") : TEXT("")) + (Key.Alt ? TEXT("Alt+") : TEXT("")) + Name;
		}
		return FString(TEXT("Unbound"));
	};
	const FString Shortcuts = FString::Printf(TEXT("%s  Select\n%s  Deselect\n%s  Anchor\n%s  Target\n%s  Tools    %s  Undo"),
		*Binding(SelectTarget), *Binding(DeselectTarget), *Binding(EActionNameIdx::SetAnchor), *Binding(EActionNameIdx::SetTarget), *Binding(ShowTools), *Binding(Undo));
	if (Shortcuts != LastShortcuts) { ShortcutsText->SetText(FText::FromString(Shortcuts)); LastShortcuts = Shortcuts; }
}

// Original pen-style diagrams, drawn in logical UI coordinates over the paper.
int32 UHyperManageClipboardWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool ParentEnabled) const
{
	const int32 InkLayer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, ParentEnabled) + 1;
	const FVector2D Origin(20, Geometry.GetLocalSize().Y * 0.5f - 288);
	const FLinearColor Ink(0.035f, 0.07f, 0.10f, 0.90f);
	auto Line = [&](FVector2D A, FVector2D B) {
		TArray<FVector2D> Points = {Origin + A * 1.2f, Origin + B * 1.2f};
		FSlateDrawElement::MakeLines(Elements, InkLayer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Ink, true, 1.44f);
	};
	auto Arrow = [&](FVector2D A, FVector2D B) {
		Line(A, B);
		const FVector2D Direction = (B - A).GetSafeNormal();
		const FVector2D Side(-Direction.Y, Direction.X);
		Line(B, B - Direction * 5 + Side * 3); Line(B, B - Direction * 5 - Side * 3);
	};
	Line(FVector2D(45, 317), FVector2D(278, 318));
	for (int32 Index = 0; Index < 4; ++Index) {
		const float X = Index == 0 ? 224 : 200 + (Index - 1) * 24;
		const float Y = Index == 0 ? 319 : 339;
		Line(FVector2D(X, Y), FVector2D(X + 19, Y - 1));
		Line(FVector2D(X + 19, Y - 1), FVector2D(X + 20, Y + 20));
		Line(FVector2D(X + 20, Y + 20), FVector2D(X, Y + 21));
		Line(FVector2D(X, Y + 21), FVector2D(X, Y));
	}
	for (int32 Index = 0; Index < 3; ++Index) {
		const FVector2D C(80 + Index * 80, 377);
		const FVector2D Top = C + FVector2D(0, -12), Left = C + FVector2D(-13, -5), Right = C + FVector2D(13, -5);
		Line(Top, Left); Line(Top, Right); Line(Left, C); Line(Right, C);
		Line(Left, Left + FVector2D(0, 14)); Line(Right, Right + FVector2D(0, 14));
		Line(C, C + FVector2D(0, 14)); Line(Left + FVector2D(0, 14), C + FVector2D(0, 14));
		Line(Right + FVector2D(0, 14), C + FVector2D(0, 14));
		if (Index == 0) {
			Arrow(C + FVector2D(-23, 10), C + FVector2D(-23, -13));
			Arrow(C + FVector2D(23, -13), C + FVector2D(23, 10));
		} else if (Index == 1) {
			Arrow(C + FVector2D(-18, 4), C + FVector2D(-29, 10));
			Arrow(C + FVector2D(18, 4), C + FVector2D(29, 10));
			Arrow(C + FVector2D(0, -15), C + FVector2D(0, -24));
		} else {
			Line(C + FVector2D(-23, 4), C + FVector2D(-23, -13));
			Arrow(C + FVector2D(-23, -13), C + FVector2D(-9, -20));
			Line(C + FVector2D(23, -13), C + FVector2D(23, 4));
			Arrow(C + FVector2D(23, 4), C + FVector2D(9, 17));
		}
	}
	return InkLayer;
}
