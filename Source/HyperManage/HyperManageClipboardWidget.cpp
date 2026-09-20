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
		BoardSlot->SetPosition(FVector2D(20, -315));
		BoardSlot->SetSize(FVector2D(400, 630));
		auto* Art = WidgetTree->ConstructWidget<UImage>();
		Art->SetBrushFromTexture(LoadObject<UTexture2D>(nullptr, TEXT("/HyperManage/UI/Jusedawg/T_Clipboard.T_Clipboard")));
		Board->AddChildToCanvas(Art)->SetSize(FVector2D(400, 630));
		HandwrittenFont = NewObject<UFont>(this);
		HandwrittenFont->FontCacheType = EFontCacheType::Runtime;
		FTypefaceEntry Face(TEXT("Regular"));
		Face.Font = FFontData(FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("HyperManage"))->GetBaseDir(), TEXT("Art/Clipboard/Kalam-Regular.ttf")), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		HandwrittenFont->CompositeFont.DefaultTypeface.Fonts.Add(Face);
		auto AddText = [&](const TCHAR* Text, float X, float Y, float Width, float Height, int32 Size) {
			auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(Text));
			Label->SetFont(FSlateFontInfo(HandwrittenFont, Size, TEXT("Regular")));
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.035f, 0.07f, 0.24f)));
			Label->SetWrapTextAt(Width);
			auto* TextSlot = Board->AddChildToCanvas(Label);
			TextSlot->SetPosition(FVector2D(X, Y)); TextSlot->SetSize(FVector2D(Width, Height));
			return Label;
		};
		AddText(TEXT("HyperManage"), 114, 130, 230, 36, 16);
		ShortcutsText = AddText(TEXT("RMB  Tool menu"), 56, 171, 145, 24, 11);
		AnchorText = AddText(TEXT("Shift+LMB  Anchor"), 56, 198, 145, 22, 10);
		AnchorText->SetColorAndOpacity(FSlateColor(FLinearColor(0.04f, 0.25f, 0.05f)));
		TargetText = AddText(TEXT("Shift+RMB  Target"), 56, 221, 145, 22, 10);
		TargetText->SetColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.035f, 0.02f)));
		SelectionText = AddText(TEXT("Ctrl+LMB  Select\nCtrl+RMB  Deselect"), 214, 172, 134, 42, 9);
		CountText = AddText(TEXT("Selected: 0"), 220, 218, 126, 24, 11);
		UndoText = AddText(TEXT("Ctrl+Z  Undo"), 56, 246, 142, 24, 11);
		RedoText = AddText(TEXT("Ctrl+Y  Redo"), 215, 246, 134, 24, 11);
		AddText(TEXT("Default movement keys"), 56, 274, 250, 20, 9);
		for (int32 Index = 0; Index < 3; ++Index) {
			const TCHAR* Modifiers[] = {TEXT("Ctrl +\nkeys"), TEXT("Alt +\nkeys"), TEXT("Shift +\nkeys")};
			const TCHAR* Captions[] = {TEXT("Up / down\nSpin"), TEXT("Back / forth\nLeft / right"), TEXT("Roll\nPitch")};
			AddText(Modifiers[Index], 56, 298 + Index * 50, 52, 44, 10);
			AddText(Captions[Index], 165, 298 + Index * 50, 108, 44, 10);
		}
		AddText(TEXT("Keys"), 291, 298, 55, 26, 11);
		for (int32 Index = 0; Index < 4; ++Index) {
			const TCHAR* Keys[] = {TEXT("I"), TEXT("J"), TEXT("K"), TEXT("L")};
			const FVector2D Positions[] = {FVector2D(310, 326), FVector2D(288, 348), FVector2D(310, 370), FVector2D(332, 348)};
			AddText(Keys[Index], Positions[Index].X, Positions[Index].Y, 20, 24, 11);
		}
		ScaleText = AddText(TEXT("Ctrl+Alt+J  Shrink    Ctrl+Alt+L  Grow"), 56, 449, 296, 40, 10);
		StatusText = AddText(TEXT("Increment: Medium\nMove: 0.1m  Rotate: 5 deg\nXY: 8m Z: 1m | Group | View"), 56, 489, 296, 60, 9);
		NotesText = AddText(TEXT("Ctrl+Alt+K  Cycle notes"), 100, 552, 248, 24, 10);
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
	UpdateReference(*System->Config, System->Selection->SelectCount());
}

void UHyperManageClipboardWidget::UpdateReference(const UHyperManageConfiguration& Configuration, int32 Count)
{
	const auto& Config = Configuration.MMConfig;
	if (!Config.IncrementSettings.IsValidIndex(Config.IncrementSize)) return;
	const auto& Increment = Config.IncrementSettings[Config.IncrementSize];
	auto Set = [](UTextBlock* Label, const FString& Text) {
		if (Label && Label->GetText().ToString() != Text) Label->SetText(FText::FromString(Text));
	};
	Set(CountText, FString::Printf(TEXT("Selected: %d"), Count));
	auto Binding = [&](EActionNameIdx Action) {
		for (const auto& Key : Configuration.MMKeyConfigs.ActionKeys) {
			if (Key.ActionIndex != Action) continue;
			FString Name = Key.KeyName;
			if (Name == TEXT("LeftMouseButton")) Name = TEXT("LMB");
			else if (Name == TEXT("RightMouseButton")) Name = TEXT("RMB");
			return FString(Key.Ctrl ? TEXT("Ctrl+") : TEXT("")) + (Key.Shift ? TEXT("Shift+") : TEXT("")) + (Key.Alt ? TEXT("Alt+") : TEXT("")) + Name;
		}
		return FString(TEXT("Unbound"));
	};
	Set(ShortcutsText, Binding(ShowTools) + TEXT("  Tool menu"));
	Set(AnchorText, Binding(EActionNameIdx::SetAnchor) + TEXT("  Anchor"));
	Set(TargetText, Binding(EActionNameIdx::SetTarget) + TEXT("  Target"));
	Set(SelectionText, Binding(SelectTarget) + TEXT("  Select\n") + Binding(DeselectTarget) + TEXT("  Deselect"));
	Set(UndoText, Binding(Undo) + TEXT("  Undo"));
	Set(RedoText, Binding(Redo) + TEXT("  Redo"));
	Set(ScaleText, Binding(Shrink) + TEXT("  Shrink    ") + Binding(Grow) + TEXT("  Grow"));
	Set(NotesText, Binding(KnowNotes) + TEXT("  Cycle notes"));
	Set(StatusText, FString::Printf(TEXT("%s  Increment: %s\nMove: %gm   Rotate: %g deg\nXY: %gm Z: %gm | %s | %s"), *Binding(ChangeIncSize),
		*UEnum::GetDisplayValueAsText(Config.IncrementSize.GetValue()).ToString(), Increment.CentimetersToMove / 100.f, Increment.DegreesToRotate,
		Config.AlignmentGridCm / 100.f, Config.HeightGridCm / 100.f, Config.IsGrouped ? TEXT("Group") : TEXT("Individual"), Config.IsViewBased ? TEXT("View") : TEXT("Object")));
}

// Original pen-style diagrams, drawn in logical UI coordinates over the paper.
int32 UHyperManageClipboardWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool ParentEnabled) const
{
	const int32 InkLayer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, ParentEnabled) + 1;
	const FVector2D Origin(20, Geometry.GetLocalSize().Y * 0.5f - 315);
	FLinearColor Ink(0.035f, 0.07f, 0.24f, 0.90f);
	auto Line = [&](FVector2D A, FVector2D B) {
		TArray<FVector2D> Points = {Origin + A, Origin + B};
		FSlateDrawElement::MakeLines(Elements, InkLayer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Ink, true, 1.44f);
	};
	auto Arrow = [&](FVector2D A, FVector2D B) {
		Line(A, B);
		const FVector2D Direction = (B - A).GetSafeNormal();
		const FVector2D Side(-Direction.Y, Direction.X);
		Line(B, B - Direction * 5 + Side * 3); Line(B, B - Direction * 5 - Side * 3);
	};
	Line(FVector2D(114, 164), FVector2D(273, 165));
	Line(FVector2D(56, 270), FVector2D(348, 269));
	Ink = FLinearColor(0.35f, 0.035f, 0.02f, 0.9f);
	Line(FVector2D(210, 167), FVector2D(350, 166));
	Line(FVector2D(350, 166), FVector2D(352, 243));
	Line(FVector2D(352, 243), FVector2D(209, 242));
	Line(FVector2D(209, 242), FVector2D(210, 167));
	Ink = FLinearColor(0.035f, 0.07f, 0.24f, 0.9f);
	for (const FVector2D P : {FVector2D(306, 325), FVector2D(284, 347), FVector2D(306, 369), FVector2D(328, 347)}) {
		Line(P, P + FVector2D(20, -1)); Line(P + FVector2D(20, -1), P + FVector2D(21, 21));
		Line(P + FVector2D(21, 21), P + FVector2D(0, 22)); Line(P + FVector2D(0, 22), P);
	}
	for (int32 Index = 0; Index < 3; ++Index) {
		const FVector2D C(130, 316 + Index * 50);
		const FVector2D Top = C + FVector2D(0, -12), Left = C + FVector2D(-13, -5), Right = C + FVector2D(13, -5);
		Line(Top, Left); Line(Top, Right); Line(Left, C); Line(Right, C);
		Line(Left, Left + FVector2D(0, 14)); Line(Right, Right + FVector2D(0, 14));
		Line(C, C + FVector2D(0, 14)); Line(Left + FVector2D(0, 14), C + FVector2D(0, 14));
		Line(Right + FVector2D(0, 14), C + FVector2D(0, 14));
		if (Index == 0) {
			Line(C + FVector2D(-16, 15), C + FVector2D(0, 21));
			Arrow(C + FVector2D(0, 21), C + FVector2D(17, 14));
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
	Line(FVector2D(56, 444), FVector2D(349, 443));
	Arrow(FVector2D(200, 480), FVector2D(175, 480));
	Arrow(FVector2D(205, 480), FVector2D(230, 480));
	return InkLayer;
}
