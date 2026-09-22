#include "HyperManageToolWidget.h"
#include "HyperManageConfig.h"
#include "HyperManageUndo.h"
#include "HyperManageAction.h"
#include "HyperManageTransform.h"
#include "HyperManageSelection.h"
#include "HyperManageActionGlyph.h"
#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/NamedSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/ScrollBox.h"
#include "Components/ExpandableArea.h"
#include "Components/CheckBox.h"

namespace {
void StyleExpansionArrow(UExpandableArea* Area)
{
 auto Style = Area->GetStyle();
 Style.CollapsedImage.TintColor = FSlateColor(FLinearColor::White);
 Style.ExpandedImage.TintColor = FSlateColor(FLinearColor::White);
 Area->SetStyle(Style);
}
void StyleNumericInput(USpinBox* Input)
{
 auto Style = Input->GetWidgetStyle();
 Style.BackgroundBrush = FSlateColorBrush(FLinearColor(0.07f, 0.08f, 0.09f));
 Style.HoveredBackgroundBrush = FSlateColorBrush(FLinearColor(0.10f, 0.115f, 0.13f));
 Style.ActiveBackgroundBrush = FSlateColorBrush(FLinearColor(0.085f, 0.10f, 0.12f));
 Style.InactiveFillBrush = FSlateColorBrush(FLinearColor::Transparent);
 Style.HoveredFillBrush = FSlateColorBrush(FLinearColor::Transparent);
 Style.ActiveFillBrush = FSlateColorBrush(FLinearColor::Transparent);
 Style.ForegroundColor = FSlateColor(FLinearColor(0.92f, 0.94f, 0.96f));
 Style.ArrowsImage.TintColor = Style.ForegroundColor;
 Input->SetWidgetStyle(Style); Input->SetForegroundColor(Style.ForegroundColor);
}

void StylePreset(UComboBoxString* Combo)
{
 Combo->ForegroundColor = FSlateColor(FLinearColor(0.94f, 0.95f, 0.97f));
 Combo->Font.Size = 11;
 auto Style = Combo->GetWidgetStyle();
 Style.ComboButtonStyle.ButtonStyle.Normal = FSlateColorBrush(FLinearColor(0.07f, 0.08f, 0.09f));
 Style.ComboButtonStyle.ButtonStyle.Hovered = FSlateColorBrush(FLinearColor(0.12f, 0.14f, 0.16f));
 Style.ComboButtonStyle.ButtonStyle.Pressed = Style.ComboButtonStyle.ButtonStyle.Hovered;
 Style.ComboButtonStyle.DownArrowImage.TintColor = Combo->ForegroundColor;
 Combo->SetWidgetStyle(Style);
 auto ItemStyle = Combo->GetItemStyle();
 ItemStyle.TextColor = Combo->ForegroundColor; ItemStyle.SelectedTextColor = FSlateColor(FLinearColor::White);
 ItemStyle.EvenRowBackgroundBrush = FSlateColorBrush(FLinearColor(0.045f, 0.055f, 0.065f));
 ItemStyle.OddRowBackgroundBrush = ItemStyle.EvenRowBackgroundBrush;
 ItemStyle.EvenRowBackgroundHoveredBrush = FSlateColorBrush(FLinearColor(0.12f, 0.16f, 0.19f));
 ItemStyle.OddRowBackgroundHoveredBrush = ItemStyle.EvenRowBackgroundHoveredBrush;
 Combo->SetItemStyle(ItemStyle);
}
void StyleFieldButton(UButton* Button)
{
 auto Style = Button->GetStyle();
 Style.Normal = FSlateColorBrush(FLinearColor(0.085f, 0.095f, 0.10f));
 Style.Hovered = FSlateColorBrush(FLinearColor(0.24f, 0.18f, 0.105f));
 Style.Pressed = FSlateColorBrush(FLinearColor(0.38f, 0.25f, 0.12f));
 Style.NormalPadding = FMargin(4, 3); Style.PressedPadding = FMargin(4, 3); Button->SetStyle(Style);
}
UBorder* FieldBezel(UWidgetTree* Tree, UWidget* Content)
{
 auto* Bezel = Tree->ConstructWidget<UBorder>();
 Bezel->SetBrushColor(FLinearColor(0.34f, 0.23f, 0.13f)); Bezel->SetPadding(FMargin(2)); Bezel->SetContent(Content);
 return Bezel;
}
void AddFieldIcon(UWidgetTree* Tree, UButton* Button, UTextBlock* Text, int32 Kind)
{
 auto* Stack = Tree->ConstructWidget<UVerticalBox>();
 auto* Glyph = Tree->ConstructWidget<UHyperManageActionGlyph>(); Glyph->Kind = Kind; Glyph->Compact = true;
 Stack->AddChildToVerticalBox(Glyph)->SetHorizontalAlignment(HAlign_Center);
 auto Font = Text->GetFont(); Font.Size = 10; Text->SetFont(Font); Text->SetJustification(ETextJustify::Center);
 Stack->AddChildToVerticalBox(Text)->SetHorizontalAlignment(HAlign_Center);
 auto* Tile = Tree->ConstructWidget<USizeBox>(); Tile->SetWidthOverride(76); Tile->SetHeightOverride(46); Tile->SetContent(Stack);
 Button->SetContent(Tile); StyleFieldButton(Button);
}
UTextBlock* FieldButtonLabel(UButton* Button)
{
 auto* Tile = Button ? Cast<USizeBox>(Button->GetContent()) : nullptr;
 auto* Stack = Tile ? Cast<UVerticalBox>(Tile->GetContent()) : nullptr;
 return Stack && Stack->GetChildrenCount() > 1 ? Cast<UTextBlock>(Stack->GetChildAt(1)) : nullptr;
}
}

void UHyperManageToolWidget::CompactApplyButton(UButton* Button, bool Toolbar)
{
 auto* Tile = Cast<USizeBox>(Button->GetContent());
 auto* Stack = Tile ? Cast<UVerticalBox>(Tile->GetContent()) : nullptr;
 auto* Glyph = Stack && Stack->GetChildrenCount() ? Cast<UHyperManageActionGlyph>(Stack->GetChildAt(0)) : nullptr;
 if (!Glyph) return;
 Glyph->RemoveFromParent();
 auto* Box = WidgetTree->ConstructWidget<USizeBox>(); Box->SetWidthOverride(Toolbar ? 36 : 26); Box->SetHeightOverride(Toolbar ? 30 : 24); Box->SetContent(Glyph);
 Button->SetContent(Box);
}

void UButtonProxy::ClickSplitEvent()
{
 if (!SplitButton || !FSlateApplication::IsInitialized()) return;
 const auto& Geometry = SplitButton->GetCachedGeometry();
 const float X = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos()).X;
 if (auto* System = UHyperManageSystem::Get()) System->ExecuteAction(X < Geometry.GetLocalSize().X * 0.5f ? DecreaseAction : ToolAction);
}

void UButtonProxy::ClickEvent()
{
	if (auto* System = UHyperManageSystem::Get()) System->ExecuteAction(ToolAction);
}

void UHyperManageToolWidget::HookWidget(EActionNameIdx ToolAction, UButton* Button, FString ToolTip)
{
	if (!Button) return;
	Button->SetIsEnabled(!ToolTip.StartsWith(TEXT("(Coming Soon)")));
	UButtonProxy* ButtonProxy = UButtonProxy::Create(ToolAction);
	ButtonProxyArray.Add(ButtonProxy); // for persistence
	Button->OnClicked.AddDynamic(ButtonProxy, &UButtonProxy::ClickEvent);
	Button->SetToolTipText(FText::FromString(ToolTip));
}

void UHyperManageToolWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RepairToolbarLayout();
	RepairQuickActions();
 WidgetTree->ForEachWidgetAndDescendants([](UWidget* Widget) {
  if (auto* Text = Cast<UTextBlock>(Widget)) { auto Font = Text->GetFont(); Font.Size = FMath::Min(Font.Size, 12); Text->SetFont(Font); }
  if (auto* Input = Cast<USpinBox>(Widget)) { auto Font = Input->GetFont(); Font.Size = 12; Input->SetFont(Font); }
  if (auto* Combo = Cast<UComboBoxString>(Widget)) { Combo->Font.Size = 12; }
 });
	StopAllAnimations();
	TrayOpenTime = 0.f;
	mUseKeyboard = true;
	//mUseMouse = true;
	//mCaptureInput = true;

	HookWidget(EActionNameIdx::AlignLeft, btnAlignLeft, "(Coming Soon) Align Left Edges To Anchor");
	HookWidget(EActionNameIdx::AlignLRCenter, btnAlignLRCenter, "(Coming Soon) Align Left-Right Centers To Anchor");
	HookWidget(EActionNameIdx::AlignRight, btnAlignRight, "(Coming Soon) Align Right Edges To Anchor");
	HookWidget(EActionNameIdx::SpaceLR, btnSpaceLR, "(Coming Soon) Space Objects Equally Left To Right");
	HookWidget(EActionNameIdx::StackLR, btnStackLR, "(Coming Soon) Stack Objects Left To Right");
	HookWidget(EActionNameIdx::LockScaleLR, btnLockScaleLR, "(Work In Progress) Lock Scaling Left To Right");

	HookWidget(EActionNameIdx::AlignTop, btnAlignTop, "(Coming Soon) Align Tops To Anchor");
	HookWidget(EActionNameIdx::AlignTBCenter, btnAlignTBCenter, "(Coming Soon) Align Top-Bottom Centers To Anchor");
	HookWidget(EActionNameIdx::AlignBottom, btnAlignBottom, "(Coming Soon) Align Bottoms To Anchor");
	HookWidget(EActionNameIdx::SpaceTB, btnSpaceTB, "(Coming Soon) Space Objects Equally Top To Bottom");
	HookWidget(EActionNameIdx::StackTB, btnStackTB, "(Coming Soon) Stack Objects Top To Bottom");
	HookWidget(EActionNameIdx::LockScaleTB, btnLockScaleTB, "(Work In Progress) Lock Scaling Top To Bottom");

	HookWidget(EActionNameIdx::AlignFront, btnAlignFront, "(Coming Soon) Align Front Edges To Anchor");
	HookWidget(EActionNameIdx::AlignFBCenter, btnAlignFBCenter, "(Coming Soon) Align Front-Back Centers To Anchor");
	HookWidget(EActionNameIdx::AlignBack, btnAlignBack, "(Coming Soon) Align Back Edges To Anchor");
	HookWidget(EActionNameIdx::SpaceFB, btnSpaceFB, "(Coming Soon) Space Objects Equally Front To Back");
	HookWidget(EActionNameIdx::StackFB, btnStackFB, "(Coming Soon) Stack Objects Front To Back");
	HookWidget(EActionNameIdx::LockScaleFB, btnLockScaleFB, "(Work In Progress) Lock Scaling Front To Back");

	HookWidget(EActionNameIdx::SameRotation, btnSameRotation, "Set Anchor (with Selection) to Target Rotation");
	HookWidget(EActionNameIdx::SameScale, btnSameScale, "Set Selection to Target Scale");
	HookWidget(EActionNameIdx::SamePaint, btnSamePaint, "Set Selection to Target Paint Color");

	HookWidget(EActionNameIdx::Connect, btnConnect, "Connect Anchor Output to Target Input");
	HookWidget(EActionNameIdx::Disconnect, btnDisconnect, "Disconnect Anchor Outputs from Target Inputs");

	HookWidget(EActionNameIdx::SelectBoxSides, btnSelectBoxSides, "Select items between Anchor and Target Sides");
	HookWidget(EActionNameIdx::SelectBoxPivot, btnSelectBoxPivot, "Select items between Anchor and Target Centers");
	HookWidget(EActionNameIdx::MoveSelection, btnMoveSelection, "Move Selection from Anchor to Target");
	HookWidget(EActionNameIdx::CopySelection, btnCopySelection, "(Coming Soon) Copy Selection from Anchor to Target");
	HookWidget(EActionNameIdx::NewSelection, btnNewSelection, "Clear Selection (Ctrl+Z restores it)");
	HookWidget(EActionNameIdx::DeleteSelection, btnDeleteSelection, "(Coming Soon) Delete Selection");
	HookWidget(EActionNameIdx::SaveSelection, btnSaveSelection, "Remember Selection for This Session");
	HookWidget(EActionNameIdx::LoadSelection, btnLoadSelection, "Restore Remembered Selection (Ctrl+Z restores the previous selection)");

	HookWidget(EActionNameIdx::IsGrouped, btnIsGrouped, "Grouped or Ungrouped Selection");
	HookWidget(EActionNameIdx::IsViewBased, btnIsViewBased, "View or Object Relative Actions");
	HookWidget(EActionNameIdx::NextHologram, btnNextHologram, "Refresh Selection Highlight");
	HookWidget(EActionNameIdx::Settings, btnSettings, "(Coming Soon) Settings");
	HookWidget(EActionNameIdx::ClearUndo, btnClearUndo, "Clear undo and redo history for this session");
}

void UHyperManageToolWidget::CloseTools()
{
	OnEscapePressed();
}

void UHyperManageToolWidget::RepairToolbarLayout()
{
	// The old Blueprint inserts a game window around a stretch-only canvas. Its desired size collapses in current UMG.
	if (!WidgetTree) return;
	// Blueprint Construct reparents Content into a foreign UUserWidget. Tree traversal stops at that boundary,
	// but both widgets still belong to this WidgetTree as UObjects.
	auto* Window = FindObjectFast<UNamedSlot>(WidgetTree, TEXT("ToolbarWindow"));
	auto* Content = FindObjectFast<UWidget>(WidgetTree, TEXT("Content"));
	if (!ensureMsgf(Window && Content, TEXT("HyperManage tools: toolbar widgets missing after Blueprint construction"))) return;

	WidgetTree->ForEachWidgetAndDescendants([](UWidget* Widget) {
		if (auto* Image = Cast<UImage>(Widget)) {
			const FString Path = Image->GetBrush().GetResourceObject() ? Image->GetBrush().GetResourceObject()->GetPathName() : FString();
			if (Path.StartsWith(TEXT("/HyperManage/Textures/Buttons/"))) Image->SetDesiredSizeOverride(FVector2D(40, 40));
		}
	});

	Content->RemoveFromParent();
	Window->ClearChildren();
	auto* Frame = WidgetTree->ConstructWidget<UBorder>();
	Frame->SetBrushColor(FLinearColor::Transparent);
	Frame->SetPadding(FMargin(8));
	auto* Rows = WidgetTree->ConstructWidget<UVerticalBox>();
	auto* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto* Title = WidgetTree->ConstructWidget<UTextBlock>();
	Title->SetText(FText::FromString(TEXT("HyperManage | dev.49")));
	auto TitleFont = Title->GetFont(); TitleFont.Size = 17; Title->SetFont(TitleFont);
	Header->AddChildToHorizontalBox(Title)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	auto* Close = WidgetTree->ConstructWidget<UButton>();
	auto* CloseText = WidgetTree->ConstructWidget<UTextBlock>();
	CloseText->SetText(FText::FromString(TEXT("X")));
	Close->SetContent(CloseText); Close->SetToolTipText(FText::FromString(TEXT("Close tool tray [Esc]")));
	StyleFieldButton(Close);
	Close->OnClicked.AddDynamic(this, &UHyperManageToolWidget::CloseTools);
	Header->AddChildToHorizontalBox(Close);
	auto* HeaderPlate = WidgetTree->ConstructWidget<UBorder>();
	HeaderPlate->SetBrushColor(FLinearColor(0.11f, 0.085f, 0.055f)); HeaderPlate->SetPadding(FMargin(8, 6));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.86f, 0.65f)));
	HeaderPlate->SetContent(Header); Rows->AddChildToVerticalBox(HeaderPlate);
	auto* Presets = WidgetTree->ConstructWidget<UVerticalBox>();
	PrecisionProfileLabel = WidgetTree->ConstructWidget<UTextBlock>();
	PrecisionProfileLabel->SetText(FText::FromString(TEXT("Exact steps | type a value or choose a preset")));
	auto ProfileFont = PrecisionProfileLabel->GetFont(); ProfileFont.Size = 13; PrecisionProfileLabel->SetFont(ProfileFont);
	Presets->AddChildToVerticalBox(PrecisionProfileLabel);
	auto* PresetGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	PresetGrid->SetSlotPadding(FMargin(2)); Presets->AddChildToVerticalBox(PresetGrid);
	int32 PresetIndex = 0;
	auto AddPreset = [&](const TCHAR* Label, const TArray<FString>& Options, TObjectPtr<USpinBox>& Input, float Minimum, float Maximum) {
		auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Label));
		auto* PresetRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto* Cell = WidgetTree->ConstructWidget<UVerticalBox>();
 Cell->AddChildToVerticalBox(Text); Cell->AddChildToVerticalBox(PresetRow);
 PresetGrid->AddChildToUniformGrid(Cell, PresetIndex / 2, PresetIndex % 2); ++PresetIndex;

		Input = WidgetTree->ConstructWidget<USpinBox>();
		StyleNumericInput(Input);
		Input->SetMinValue(Minimum); Input->SetMaxValue(Maximum); Input->SetValue(Minimum);
		Input->SetEnableSlider(false); Input->SetMinDesiredWidth(78.f);
		Input->SetMinFractionalDigits(0); Input->SetMaxFractionalDigits(3);
		Input->SetToolTipText(FText::FromString(TEXT("Type an exact value. Enter or leaving the field saves it. Movement and rotation use the active increment profile; the XY and Z grids are shared.")));
		PresetRow->AddChildToHorizontalBox(Input)->SetPadding(FMargin(4));
		auto* Combo = WidgetTree->ConstructWidget<UComboBoxString>(); StylePreset(Combo);
		for (const FString& Option : Options) Combo->AddOption(Option);
		PresetRow->AddChildToHorizontalBox(Combo)->SetPadding(FMargin(4));
		return Combo;
	};
	MovementPreset = AddPreset(TEXT("Move (m)"), {TEXT("0.01"), TEXT("0.1"), TEXT("0.25"), TEXT("0.5"), TEXT("1"), TEXT("2"), TEXT("4"), TEXT("8")}, MovementValue, 0.01f, 1000.f);
	RotationPreset = AddPreset(TEXT("Rotate (deg)"), {TEXT("1"), TEXT("5"), TEXT("10"), TEXT("15"), TEXT("30"), TEXT("45"), TEXT("90")}, RotationValue, 0.1f, 180.f);
	GridPreset = AddPreset(TEXT("Grid XY (m)"), {TEXT("0.1"), TEXT("0.5"), TEXT("1"), TEXT("2"), TEXT("4"), TEXT("8")}, GridValue, 0.01f, 1000.f);
	HeightGridPreset = AddPreset(TEXT("Grid Z (m)"), {TEXT("0.1"), TEXT("0.25"), TEXT("0.5"), TEXT("1"), TEXT("2"), TEXT("4"), TEXT("8")}, HeightGridValue, 0.01f, 1000.f);
	HeightGridValue->OnValueCommitted.AddDynamic(this, &UHyperManageToolWidget::CommitHeightGridValue);
	HeightGridPreset->OnSelectionChanged.AddDynamic(this, &UHyperManageToolWidget::ChangeHeightGridPreset);
	MovementValue->OnValueCommitted.AddDynamic(this, &UHyperManageToolWidget::CommitMovementValue);
	RotationValue->OnValueCommitted.AddDynamic(this, &UHyperManageToolWidget::CommitRotationValue);
	GridValue->OnValueCommitted.AddDynamic(this, &UHyperManageToolWidget::CommitGridValue);
	MovementPreset->OnSelectionChanged.AddDynamic(this, &UHyperManageToolWidget::ChangeMovementPreset);
	RotationPreset->OnSelectionChanged.AddDynamic(this, &UHyperManageToolWidget::ChangeRotationPreset);
	GridPreset->OnSelectionChanged.AddDynamic(this, &UHyperManageToolWidget::ChangeGridPreset);
	Rows->AddChildToVerticalBox(Presets);
	auto* Alignments = WidgetTree->ConstructWidget<UWrapBox>();
	Alignments->SetExplicitWrapSize(true); Alignments->SetWrapSize(360);
	auto AddAlignment = [&](UWrapBox* Wrap, const TCHAR* Label, EActionNameIdx Action, const TCHAR* Tip) {
		auto* Button = WidgetTree->ConstructWidget<UButton>();
		auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Label));
		AddFieldIcon(WidgetTree, Button, Text, Action == EActionNameIdx::SnapWorldXY ? 19 : Action == EActionNameIdx::SnapWorldZ ? 24 : Action == EActionNameIdx::MatchAnchorX ? 25 : Action == EActionNameIdx::MatchAnchorY ? 26 : Action == EActionNameIdx::MatchAnchorZ ? 27 : Action == EActionNameIdx::LevelWorldRotation ? 20 : 3);
		CompactApplyButton(Button, true);
		HookWidget(Action, Button, FString(Label) + TEXT("\n") + Tip);
		Wrap->AddChildToWrapBox(Button)->SetPadding(FMargin(3, 3));
		return Button;
	};
	AddAlignment(Alignments, TEXT("Snap XY"), EActionNameIdx::SnapWorldXY, TEXT("Snap to the world-origin XY grid. 8m is foundation spacing. Height and scale stay unchanged. Group mode preserves spacing; set an anchor to choose the reference."));
	AddAlignment(Alignments, TEXT("Snap Z"), EActionNameIdx::SnapWorldZ, TEXT("Snap origins to the world-zero height grid using Grid Z. X/Y, rotation and scale stay unchanged. Group mode preserves relative heights using the anchor or first selected origin. Individual mode snaps each origin independently. Ctrl+Z undoes alignment."));
	AddAlignment(Alignments, TEXT("Snap angle"), EActionNameIdx::SnapWorldRotation, TEXT("Round world pitch/yaw/roll to the selected rotation step. Uses group mode and anchor; scale is preserved. Ctrl+Z undoes alignment."));
	AddAlignment(Alignments, TEXT("Level"), EActionNameIdx::LevelWorldRotation, TEXT("Set world pitch and roll to zero, retaining yaw. Group mode levels around the anchor/reference."));
	Rows->AddChildToVerticalBox(Alignments);
	AnchorAlignmentStatus = WidgetTree->ConstructWidget<UTextBlock>();
	AnchorAlignmentStatus->SetText(FText::FromString(TEXT("Set an anchor to align selected origins.")));
	auto AnchorFont = AnchorAlignmentStatus->GetFont(); AnchorFont.Size = 13; AnchorAlignmentStatus->SetFont(AnchorFont);
	AnchorAlignmentStatus->SetAutoWrapText(true); Rows->AddChildToVerticalBox(AnchorAlignmentStatus);
	auto* AnchorAlignments = WidgetTree->ConstructWidget<UWrapBox>();
	AnchorAlignments->SetExplicitWrapSize(true); AnchorAlignments->SetWrapSize(360);
	AnchorAlignmentButtons = {
		AddAlignment(AnchorAlignments, TEXT("Match X"), EActionNameIdx::MatchAnchorX, TEXT("Move selected origins to the anchor's world X coordinate. Y/Z, rotation and scale stay unchanged. Anchor and target stay in place. This aligns origins, not mesh edges; group mode is ignored.")),
		AddAlignment(AnchorAlignments, TEXT("Match Y"), EActionNameIdx::MatchAnchorY, TEXT("Move selected origins to the anchor's world Y coordinate. X/Z, rotation and scale stay unchanged. Anchor and target stay in place. Ctrl+Z undoes alignment.")),
		AddAlignment(AnchorAlignments, TEXT("Match Z"), EActionNameIdx::MatchAnchorZ, TEXT("Move selected origins to the anchor's world height. X/Y, rotation and scale stay unchanged. Different models can have different origin offsets, so their visible surfaces may not line up."))
	};
	for (const auto& Button : AnchorAlignmentButtons) Button->SetIsEnabled(false);
	Rows->AddChildToVerticalBox(AnchorAlignments);

	auto* HistoryRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto AddHistory = [&](TObjectPtr<UButton>& Button, const TCHAR* Label, EActionNameIdx Action, const TCHAR* Tip) {
		Button = WidgetTree->ConstructWidget<UButton>();
		auto* Text = WidgetTree->ConstructWidget<UTextBlock>(); Text->SetText(FText::FromString(Label));
		AddFieldIcon(WidgetTree, Button, Text, Action == EActionNameIdx::Redo ? 22 : 21);
		CompactApplyButton(Button, true);
		HookWidget(Action, Button, Tip); Button->SetIsEnabled(false);
		HistoryRow->AddChildToHorizontalBox(Button)->SetPadding(FMargin(3, 3));
	};
	AddHistory(UndoButton, TEXT("Undo (0)"), EActionNameIdx::Undo, TEXT("Undo the last recorded edit [Ctrl+Z]. Waiting for a lightweight edit to finish temporarily disables history."));
	AddHistory(RedoButton, TEXT("Redo (0)"), EActionNameIdx::Redo, TEXT("Restore an undone edit [Ctrl+Y]. A new recorded edit clears redo history."));
	Rows->AddChildToVerticalBox(HistoryRow);
 HistoryArea = WidgetTree->ConstructWidget<UExpandableArea>();
 StyleExpansionArrow(HistoryArea);
 auto* HistoryHeading = WidgetTree->ConstructWidget<UTextBlock>();
 HistoryHeading->SetText(FText::FromString(TEXT("Recent history")));
 auto HistoryFont = HistoryHeading->GetFont(); HistoryFont.Size = 11; HistoryHeading->SetFont(HistoryFont);
 HistoryHeading->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.68f, 0.40f)));
 HistoryDetails = WidgetTree->ConstructWidget<UTextBlock>(); HistoryDetails->SetFont(HistoryFont); HistoryDetails->SetAutoWrapText(true);
 HistoryDetails->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.90f)));
 HistoryDetails->SetText(FText::FromString(TEXT("No recorded edits yet.")));
 HistoryArea->SetContentForSlot(TEXT("Header"), HistoryHeading); HistoryArea->SetContentForSlot(TEXT("Body"), HistoryDetails);
 HistoryArea->SetBorderBrush(FSlateColorBrush(FLinearColor(0.045f, 0.05f, 0.055f)));
 HistoryArea->SetHeaderPadding(FMargin(4)); HistoryArea->SetAreaPadding(FMargin(8, 4)); HistoryArea->SetIsExpanded(false);
 HistoryArea->SetToolTipText(FText::FromString(TEXT("Most recent records first. Undo/Redo replay one step at a time. Unavailable objects are skipped; history lasts for this session only.")));
 Rows->AddChildToVerticalBox(HistoryArea)->SetPadding(FMargin(3, 2, 3, 6));
 auto AddCollapsedSection = [&](UTextBlock* Heading, UVerticalBox* ContentBody) {
  auto* Area = WidgetTree->ConstructWidget<UExpandableArea>();
  StyleExpansionArrow(Area);
  Area->SetContentForSlot(TEXT("Header"), Heading); Area->SetContentForSlot(TEXT("Body"), ContentBody);
  Area->SetBorderBrush(FSlateColorBrush(FLinearColor::Transparent)); Area->SetIsExpanded(false);
  Area->SetHeaderPadding(FMargin(0, 4)); Area->SetAreaPadding(FMargin(0, 2)); Rows->AddChildToVerticalBox(Area);
 };
	auto* OffsetHeading = WidgetTree->ConstructWidget<UTextBlock>();
	OffsetHeading->SetText(FText::FromString(TEXT("OFFSET (m)")));
	auto OffsetFont = OffsetHeading->GetFont(); OffsetFont.Size = 14; OffsetHeading->SetFont(OffsetFont);
	OffsetHeading->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.68f, 0.40f)));
	auto* OffsetBody = WidgetTree->ConstructWidget<UVerticalBox>();
 OffsetAxes = WidgetTree->ConstructWidget<UComboBoxString>(); StylePreset(OffsetAxes);
 OffsetAxes->AddOption(TEXT("World axes")); OffsetAxes->AddOption(TEXT("Object axes")); OffsetAxes->SetSelectedOption(TEXT("World axes"));
 OffsetAxes->SetToolTipText(FText::FromString(TEXT("World: fixed X/Y/Z. Object: selected anchor's local directions, or the only selected object. All selected objects move together. Scale does not multiply the distance. This affects only the offset fields below.")));
 OffsetBody->AddChildToVerticalBox(OffsetAxes)->SetPadding(FMargin(2, 2, 2, 4));
	auto* OffsetRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto AddOffset = [&](const TCHAR* Axis, TObjectPtr<USpinBox>& Input) {
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Axis));
		OffsetRow->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
		Input = WidgetTree->ConstructWidget<USpinBox>();
		StyleNumericInput(Input);
		Input->SetMinValue(-1000.f); Input->SetMaxValue(1000.f); Input->SetValue(0.f);
		Input->SetEnableSlider(false); Input->SetMinDesiredWidth(54.f); Input->SetMinFractionalDigits(0); Input->SetMaxFractionalDigits(3);
		Input->SetToolTipText(FText::FromString(TEXT("Offset in meters along the chosen axes, from -1000 to 1000. Object Z may be tilted; use World axes for height. Zero leaves this axis unchanged. Apply moves the selection; typing alone does not.")));
		OffsetRow->AddChildToHorizontalBox(Input)->SetPadding(FMargin(2));
	};
	AddOffset(TEXT("X"), OffsetX); AddOffset(TEXT("Y"), OffsetY); AddOffset(TEXT("Z"), OffsetZ);
	OffsetBody->AddChildToVerticalBox(OffsetRow);
	auto* OffsetActions = OffsetRow;
	ApplyOffsetButton = WidgetTree->ConstructWidget<UButton>();
	auto* ApplyText = WidgetTree->ConstructWidget<UTextBlock>(); ApplyText->SetText(FText::FromString(TEXT("Apply")));
	AddFieldIcon(WidgetTree, ApplyOffsetButton, ApplyText, 14); CompactApplyButton(ApplyOffsetButton);
	ApplyOffsetButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ApplyWorldOffset);
	ApplyOffsetButton->SetToolTipText(FText::FromString(TEXT("Move selected objects together along the chosen axes. Object axes require one selected object or a selected anchor. Target is excluded. Rotation, scale and spacing are preserved. Ctrl+Z undoes the whole move.")));
	ApplyOffsetButton->SetIsEnabled(false);
	OffsetActions->AddChildToHorizontalBox(ApplyOffsetButton)->SetPadding(FMargin(2));
	auto* ClearOffset = WidgetTree->ConstructWidget<UButton>();
	auto* ClearText = WidgetTree->ConstructWidget<UTextBlock>(); ClearText->SetText(FText::FromString(TEXT("Zero fields")));
	AddFieldIcon(WidgetTree, ClearOffset, ClearText, 23); CompactApplyButton(ClearOffset);
	ClearOffset->SetToolTipText(FText::FromString(TEXT("Reset the three input values without moving any objects.")));
	ClearOffset->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ClearWorldOffset);
	OffsetActions->AddChildToHorizontalBox(ClearOffset)->SetPadding(FMargin(2));

	OffsetStatus = WidgetTree->ConstructWidget<UTextBlock>(); OffsetStatus->SetFont(OffsetFont);
	OffsetStatus->SetText(FText::FromString(TEXT("Select objects, then enter an offset. Z changes height.")));
	OffsetStatus->SetAutoWrapText(true); OffsetBody->AddChildToVerticalBox(OffsetStatus);
 AddCollapsedSection(OffsetHeading, OffsetBody);
 auto* PositionArea = WidgetTree->ConstructWidget<UExpandableArea>();
 StyleExpansionArrow(PositionArea);
 auto* PositionHeading = WidgetTree->ConstructWidget<UTextBlock>();
 PositionHeading->SetText(FText::FromString(TEXT("WORLD POSITION (m)"))); PositionHeading->SetFont(OffsetFont);
 PositionHeading->SetColorAndOpacity(OffsetHeading->GetColorAndOpacity());
 auto* PositionBody = WidgetTree->ConstructWidget<UVerticalBox>();
 auto* PositionRow = WidgetTree->ConstructWidget<UHorizontalBox>();
 PositionAxes.Reset();
 auto AddPosition = [&](const TCHAR* Axis, TObjectPtr<USpinBox>& Input) {
  auto* Enabled = WidgetTree->ConstructWidget<UCheckBox>(); Enabled->SetIsChecked(true);
  Enabled->SetToolTipText(FText::FromString(TEXT("Checked: apply this world coordinate. Unchecked: keep this axis fixed. These switches affect only World position.")));
  PositionAxes.Add(Enabled); PositionRow->AddChildToHorizontalBox(Enabled)->SetVerticalAlignment(VAlign_Center);
  auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Axis));
  PositionRow->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
  Input = WidgetTree->ConstructWidget<USpinBox>(); StyleNumericInput(Input);
  Input->SetMinValue(-10000.f); Input->SetMaxValue(10000.f); Input->SetValue(0.f);
  Input->SetEnableSlider(false); Input->SetMinDesiredWidth(46.f); Input->SetMinFractionalDigits(0); Input->SetMaxFractionalDigits(3);
  Input->SetToolTipText(FText::FromString(TEXT("Absolute world coordinate in meters. Read position fills current values. Zero means world zero when this axis is checked. Uncheck an axis to hold it fixed. Apply moves at most 1000 m per axis.")));
  PositionRow->AddChildToHorizontalBox(Input)->SetPadding(FMargin(2));
 };
 AddPosition(TEXT("X"), PositionX); AddPosition(TEXT("Y"), PositionY); AddPosition(TEXT("Z"), PositionZ);
 ApplyPositionButton = WidgetTree->ConstructWidget<UButton>();
 auto* PositionApplyText = WidgetTree->ConstructWidget<UTextBlock>(); PositionApplyText->SetText(FText::FromString(TEXT("Apply position")));
 AddFieldIcon(WidgetTree, ApplyPositionButton, PositionApplyText, 14); CompactApplyButton(ApplyPositionButton);
 ApplyPositionButton->SetToolTipText(FText::FromString(TEXT("Move the selected anchor, or selection center, to the checked coordinates. Unchecked axes stay fixed. All selected objects move together; the target stays in place. Undoable. Group/individual mode does not change this operation.")));
 ApplyPositionButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ApplyWorldPosition); ApplyPositionButton->SetIsEnabled(false);
 PositionRow->AddChildToHorizontalBox(ApplyPositionButton)->SetPadding(FMargin(2));
 ReadPositionButton = WidgetTree->ConstructWidget<UButton>();
 auto* PositionReadText = WidgetTree->ConstructWidget<UTextBlock>(); PositionReadText->SetText(FText::FromString(TEXT("Read position")));
 AddFieldIcon(WidgetTree, ReadPositionButton, PositionReadText, 11); CompactApplyButton(ReadPositionButton);
 ReadPositionButton->SetToolTipText(FText::FromString(TEXT("Fill X/Y/Z from the selected anchor or selection center without moving anything. For one object, use its origin.")));
 ReadPositionButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ReadWorldPosition); ReadPositionButton->SetIsEnabled(false);
 PositionRow->AddChildToHorizontalBox(ReadPositionButton)->SetPadding(FMargin(2));
 PositionBody->AddChildToVerticalBox(PositionRow);
 PositionStatus = WidgetTree->ConstructWidget<UTextBlock>(); PositionStatus->SetFont(OffsetFont); PositionStatus->SetAutoWrapText(true);
 PositionStatus->SetText(FText::FromString(TEXT("Select objects, then read their current position."))); PositionBody->AddChildToVerticalBox(PositionStatus);
 PositionArea->SetContentForSlot(TEXT("Header"), PositionHeading); PositionArea->SetContentForSlot(TEXT("Body"), PositionBody);
 PositionArea->SetBorderBrush(FSlateColorBrush(FLinearColor::Transparent)); PositionArea->SetIsExpanded(false);
 PositionArea->SetHeaderPadding(FMargin(0, 4)); PositionArea->SetAreaPadding(FMargin(0, 2)); Rows->AddChildToVerticalBox(PositionArea);
	auto* RotationHeading = WidgetTree->ConstructWidget<UTextBlock>();
	RotationHeading->SetText(FText::FromString(TEXT("WORLD ROTATION OFFSET (deg)")));
	RotationHeading->SetFont(OffsetFont); RotationHeading->SetColorAndOpacity(OffsetHeading->GetColorAndOpacity());
	auto* RotationBody = WidgetTree->ConstructWidget<UVerticalBox>();
	auto* RotationFields = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto AddRotation = [&](const TCHAR* Axis, TObjectPtr<USpinBox>& Input) {
		auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Axis));
		Row->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
		Input = WidgetTree->ConstructWidget<USpinBox>();
		StyleNumericInput(Input);
		Input->SetMinValue(-180.f); Input->SetMaxValue(180.f); Input->SetValue(0.f);
		Input->SetEnableSlider(false); Input->SetMinDesiredWidth(54.f); Input->SetMinFractionalDigits(0); Input->SetMaxFractionalDigits(3);
		Input->SetToolTipText(FText::FromString(TEXT("Rotation offset in degrees, from -180 to 180. Zero leaves this component unchanged. Apply combines roll, pitch and yaw into one world-space rotation.")));
		Row->AddChildToHorizontalBox(Input)->SetPadding(FMargin(2));
		RotationFields->AddChildToHorizontalBox(Row);
	};
	AddRotation(TEXT("Y"), OffsetYaw);
	AddRotation(TEXT("P"), OffsetPitch);
	AddRotation(TEXT("R"), OffsetRoll);
	RotationBody->AddChildToVerticalBox(RotationFields);
	auto* RotationActions = RotationFields;
	ApplyRotationButton = WidgetTree->ConstructWidget<UButton>();
	auto* RotateText = WidgetTree->ConstructWidget<UTextBlock>(); RotateText->SetText(FText::FromString(TEXT("Apply")));
	AddFieldIcon(WidgetTree, ApplyRotationButton, RotateText, 3); CompactApplyButton(ApplyRotationButton);
	ApplyRotationButton->SetIsEnabled(false);
	ApplyRotationButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ApplyWorldRotationOffset);
	ApplyRotationButton->SetToolTipText(FText::FromString(TEXT("Rotate the selection using world axes. Group mode uses the selected anchor, or the center of selected origins. Individual mode rotates in place. Target is excluded; scale is preserved. Ctrl+Z undoes the edit.")));
	RotationActions->AddChildToHorizontalBox(ApplyRotationButton)->SetPadding(FMargin(2));
	auto* ClearRotation = WidgetTree->ConstructWidget<UButton>();
	auto* ResetText = WidgetTree->ConstructWidget<UTextBlock>(); ResetText->SetText(FText::FromString(TEXT("Zero fields")));
	AddFieldIcon(WidgetTree, ClearRotation, ResetText, 23); CompactApplyButton(ClearRotation);
	ClearRotation->SetToolTipText(FText::FromString(TEXT("Clear the rotation inputs without changing any objects.")));
	ClearRotation->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ClearRotationOffset);
	RotationActions->AddChildToHorizontalBox(ClearRotation)->SetPadding(FMargin(2));

	RotationStatus = WidgetTree->ConstructWidget<UTextBlock>(); RotationStatus->SetFont(OffsetFont); RotationStatus->SetAutoWrapText(true);
	RotationStatus->SetText(FText::FromString(TEXT("Select objects, then enter rotation offsets.")));
	RotationBody->AddChildToVerticalBox(RotationStatus);
 AddCollapsedSection(RotationHeading, RotationBody);
 auto* OrientationArea = WidgetTree->ConstructWidget<UExpandableArea>();
 StyleExpansionArrow(OrientationArea);
 auto* OrientationHeading = WidgetTree->ConstructWidget<UTextBlock>();
 OrientationHeading->SetText(FText::FromString(TEXT("WORLD ORIENTATION (deg)"))); OrientationHeading->SetFont(OffsetFont);
 OrientationHeading->SetColorAndOpacity(OffsetHeading->GetColorAndOpacity());
 auto* OrientationBody = WidgetTree->ConstructWidget<UVerticalBox>();
 auto* OrientationRow = WidgetTree->ConstructWidget<UHorizontalBox>();
 OrientationAxes.Reset();
 auto AddOrientation = [&](const TCHAR* Axis, TObjectPtr<USpinBox>& Input) {
  auto* Enabled = WidgetTree->ConstructWidget<UCheckBox>(); Enabled->SetIsChecked(true);
  Enabled->SetToolTipText(FText::FromString(TEXT("Checked: set this reference angle. Unchecked: retain its current Euler angle. Applies only to World orientation; a group follows its anchor.")));
  OrientationAxes.Add(Enabled); OrientationRow->AddChildToHorizontalBox(Enabled)->SetVerticalAlignment(VAlign_Center);
  auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Axis));
  OrientationRow->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
  Input = WidgetTree->ConstructWidget<USpinBox>(); StyleNumericInput(Input);
  Input->SetMinValue(-180.f); Input->SetMaxValue(180.f); Input->SetValue(0.f);
  Input->SetEnableSlider(false); Input->SetMinDesiredWidth(36.f); Input->SetMinFractionalDigits(0); Input->SetMaxFractionalDigits(3);
  Input->SetToolTipText(FText::FromString(TEXT("Absolute world angle in degrees, from -180 to 180. Read orientation fills current values. Zero sets this angle to zero when checked. Uncheck to retain the reference angle.")));
  OrientationRow->AddChildToHorizontalBox(Input)->SetPadding(FMargin(2));
 };
 AddOrientation(TEXT("Y"), OrientationYaw); AddOrientation(TEXT("P"), OrientationPitch); AddOrientation(TEXT("R"), OrientationRoll);
 ApplyOrientationButton = WidgetTree->ConstructWidget<UButton>();
 auto* OrientationApplyText = WidgetTree->ConstructWidget<UTextBlock>(); OrientationApplyText->SetText(FText::FromString(TEXT("Apply orientation")));
 AddFieldIcon(WidgetTree, ApplyOrientationButton, OrientationApplyText, 3); CompactApplyButton(ApplyOrientationButton);
 ApplyOrientationButton->SetToolTipText(FText::FromString(TEXT("Set the checked reference angles; retain unchecked Euler angles. A single object rotates in place; multiple objects require a selected anchor and rotate together around it. Target stays in place. Scale and relative orientations are preserved. Undoable; independent of group mode.")));
 ApplyOrientationButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ApplyWorldOrientation); ApplyOrientationButton->SetIsEnabled(false);
 OrientationRow->AddChildToHorizontalBox(ApplyOrientationButton)->SetPadding(FMargin(2));
 ReadOrientationButton = WidgetTree->ConstructWidget<UButton>();
 auto* OrientationReadText = WidgetTree->ConstructWidget<UTextBlock>(); OrientationReadText->SetText(FText::FromString(TEXT("Read orientation")));
 AddFieldIcon(WidgetTree, ReadOrientationButton, OrientationReadText, 11); CompactApplyButton(ReadOrientationButton);
 ReadOrientationButton->SetToolTipText(FText::FromString(TEXT("Read yaw/pitch/roll from the selected anchor, or the only selected object, without rotating anything.")));
 ReadOrientationButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ReadWorldOrientation); ReadOrientationButton->SetIsEnabled(false);
 OrientationRow->AddChildToHorizontalBox(ReadOrientationButton)->SetPadding(FMargin(2));
 OrientationBody->AddChildToVerticalBox(OrientationRow);
 OrientationStatus = WidgetTree->ConstructWidget<UTextBlock>(); OrientationStatus->SetFont(OffsetFont); OrientationStatus->SetAutoWrapText(true);
 OrientationStatus->SetText(FText::FromString(TEXT("Select objects, then read their current orientation."))); OrientationBody->AddChildToVerticalBox(OrientationStatus);
 OrientationArea->SetContentForSlot(TEXT("Header"), OrientationHeading); OrientationArea->SetContentForSlot(TEXT("Body"), OrientationBody);
 OrientationArea->SetBorderBrush(FSlateColorBrush(FLinearColor::Transparent)); OrientationArea->SetIsExpanded(false);
 OrientationArea->SetHeaderPadding(FMargin(0, 4)); OrientationArea->SetAreaPadding(FMargin(0, 2)); Rows->AddChildToVerticalBox(OrientationArea);
	auto* ScaleHeading = WidgetTree->ConstructWidget<UTextBlock>();
	ScaleHeading->SetText(FText::FromString(TEXT("EXACT LOCAL SCALE (%)")));
	ScaleHeading->SetFont(OffsetFont); ScaleHeading->SetColorAndOpacity(OffsetHeading->GetColorAndOpacity());
	auto* ScaleBody = WidgetTree->ConstructWidget<UVerticalBox>();
	auto* ScaleRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto AddScale = [&](const TCHAR* Axis, TObjectPtr<USpinBox>& Input) {
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Axis));
		ScaleRow->AddChildToHorizontalBox(Label)->SetVerticalAlignment(VAlign_Center);
		Input = WidgetTree->ConstructWidget<USpinBox>();
		StyleNumericInput(Input);
		Input->SetMinValue(1.f); Input->SetMaxValue(1000.f); Input->SetValue(100.f);
		Input->SetEnableSlider(false); Input->SetMinDesiredWidth(76.f); Input->SetMinFractionalDigits(0); Input->SetMaxFractionalDigits(2);
		Input->SetToolTipText(FText::FromString(TEXT("Absolute local-axis scale: 100% is original size, 50% is half, 200% is double. Range 1-1000%. Apply changes size without moving object origins.")));
		Input->OnValueChanged.AddDynamic(this, &UHyperManageToolWidget::ClearScalePreset);
		ScaleRow->AddChildToHorizontalBox(Input)->SetPadding(FMargin(4));
	};
	AddScale(TEXT("X"), ScaleX); AddScale(TEXT("Y"), ScaleY); AddScale(TEXT("Z"), ScaleZ);
	ScaleBody->AddChildToVerticalBox(ScaleRow);
	auto* ScalePresetRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto* PresetLabel = WidgetTree->ConstructWidget<UTextBlock>(); PresetLabel->SetText(FText::FromString(TEXT("Uniform preset")));
	ScalePresetRow->AddChildToHorizontalBox(PresetLabel)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ScalePreset = WidgetTree->ConstructWidget<UComboBoxString>(); StylePreset(ScalePreset);
	for (const TCHAR* Option : {TEXT("25"), TEXT("50"), TEXT("75"), TEXT("100"), TEXT("125"), TEXT("150"), TEXT("200")}) ScalePreset->AddOption(Option);
	ScalePreset->SetToolTipText(FText::FromString(TEXT("Fill all three fields with a percentage, then click Apply scale. Choosing a preset alone does not change objects.")));
	ScalePreset->OnSelectionChanged.AddDynamic(this, &UHyperManageToolWidget::ChangeScalePreset);
	ScalePresetRow->AddChildToHorizontalBox(ScalePreset)->SetPadding(FMargin(4));
	ScaleBody->AddChildToVerticalBox(ScalePresetRow);
	auto* ScaleActions = WidgetTree->ConstructWidget<UHorizontalBox>();
	ApplyScaleButton = WidgetTree->ConstructWidget<UButton>();
	auto* ScaleText = WidgetTree->ConstructWidget<UTextBlock>(); ScaleText->SetText(FText::FromString(TEXT("Apply")));
	AddFieldIcon(WidgetTree, ApplyScaleButton, ScaleText, 6); CompactApplyButton(ApplyScaleButton); ApplyScaleButton->SetIsEnabled(false);
	ApplyScaleButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ApplyScalePercent);
	ApplyScaleButton->SetToolTipText(FText::FromString(TEXT("Set the selected objects to these absolute local scales. Target is excluded. Position and rotation stay unchanged regardless of group mode. Ctrl+Z undoes the edit.")));
	ScaleActions->AddChildToHorizontalBox(ApplyScaleButton)->SetPadding(FMargin(4));
	auto* ResetScale = WidgetTree->ConstructWidget<UButton>();
	auto* ResetScaleText = WidgetTree->ConstructWidget<UTextBlock>(); ResetScaleText->SetText(FText::FromString(TEXT("100%")));
	AddFieldIcon(WidgetTree, ResetScale, ResetScaleText, 23); CompactApplyButton(ResetScale);
	ResetScale->SetToolTipText(FText::FromString(TEXT("Fill X/Y/Z with 100%. Click Apply scale to restore original size.")));
	ResetScale->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ResetScaleFields);
	ScaleActions->AddChildToHorizontalBox(ResetScale)->SetPadding(FMargin(4));
 ReadScaleButton = WidgetTree->ConstructWidget<UButton>();
 auto* ReadScaleText = WidgetTree->ConstructWidget<UTextBlock>(); ReadScaleText->SetText(FText::FromString(TEXT("Read scale")));
 AddFieldIcon(WidgetTree, ReadScaleButton, ReadScaleText, 11); CompactApplyButton(ReadScaleButton); ReadScaleButton->SetIsEnabled(false);
 ReadScaleButton->SetToolTipText(FText::FromString(TEXT("Read scale: fill X/Y/Z from the selected anchor, or the only selected object. Nothing is resized until Apply. Requires values within 1-1000% and no pending edits. For a group, Apply sets every member to the entered scales.")));
 ReadScaleButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::ReadScale);
 ScaleActions->AddChildToHorizontalBox(ReadScaleButton)->SetPadding(FMargin(4));
	ScaleBody->AddChildToVerticalBox(ScaleActions);
	ScaleStatus = WidgetTree->ConstructWidget<UTextBlock>(); ScaleStatus->SetFont(OffsetFont); ScaleStatus->SetAutoWrapText(true);
	ScaleStatus->SetText(FText::FromString(TEXT("Select objects to resize. 100% is original size.")));
	ScaleBody->AddChildToVerticalBox(ScaleStatus);
 AddCollapsedSection(ScaleHeading, ScaleBody);
	QuickActionHost = WidgetTree->ConstructWidget<UVerticalBox>();
	Rows->AddChildToVerticalBox(QuickActionHost);
	auto* Body = WidgetTree->ConstructWidget<USizeBox>();
	Body->SetWidthOverride(360);
	Body->SetHeightOverride(430);
	// Replace the overflowing legacy icon strips, retaining the existing buttons and their action bindings.
	if (auto* Canvas = Cast<UCanvasPanel>(Content)) {
		Canvas->ClearChildren();
		auto* Groups = WidgetTree->ConstructWidget<UVerticalBox>();
		auto* GroupSlot = Canvas->AddChildToCanvas(Groups);
		GroupSlot->SetAnchors(FAnchors(0, 0, 1, 1));
		GroupSlot->SetOffsets(FMargin(0));
		auto AddGroup = [&](const TCHAR* Heading, const TArray<TPair<UButton*, FString>>& Buttons) {
			auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(FText::FromString(Heading));
			auto Font = Label->GetFont(); Font.Size = 14; Label->SetFont(Font);
			Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.68f, 0.40f)));
			Groups->AddChildToVerticalBox(Label);
			auto* Wrap = WidgetTree->ConstructWidget<UWrapBox>();
			Wrap->SetWrapSize(360);
			Wrap->SetExplicitWrapSize(true);
			Wrap->SetInnerSlotPadding(FVector2D(6, 5));
			for (const auto& Entry : Buttons) {
				if (!Entry.Key) continue;
				Entry.Key->RemoveFromParent();
				auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
				Text->SetText(FText::FromString(Entry.Value));
				auto ButtonFont = Text->GetFont(); ButtonFont.Size = 16; Text->SetFont(ButtonFont);
				Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.90f, 0.76f)));
                int32 Kind = 7;
                if (Entry.Key == btnSelectBoxSides) Kind = 8;
                else if (Entry.Key == btnSelectBoxPivot) Kind = 9;
                else if (Entry.Key == RemoveBoxEdgesButton) Kind = 28;
                else if (Entry.Key == RemoveBoxCentersButton) Kind = 29;
                else if (Entry.Key == btnSaveSelection) Kind = 10;
                else if (Entry.Key == btnLoadSelection) Kind = 11;
                else if (Entry.Key == AddSlotButton) Kind = 30;
                else if (Entry.Key == RemoveSlotButton) Kind = 31;
                else if (Entry.Key == btnIsGrouped) Kind = 12;
                else if (Entry.Key == btnIsViewBased) Kind = 13;
                else if (Entry.Key == btnMoveSelection) Kind = 14;
                else if (Entry.Key == btnSameRotation) Kind = 3;
                else if (Entry.Key == btnSameScale) Kind = 6;
                else if (Entry.Key == btnSamePaint) Kind = 15;
                else if (Entry.Key == btnConnect) Kind = 16;
                else if (Entry.Key == btnDisconnect) Kind = 17;
                else if (Entry.Key == btnClearUndo) Kind = 18;
                AddFieldIcon(WidgetTree, Entry.Key, Text, Kind);
				Wrap->AddChildToWrapBox(Entry.Key);
			}
			Groups->AddChildToVerticalBox(Wrap);
		};
  auto* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>();
  SelectionSlotPicker = WidgetTree->ConstructWidget<UComboBoxString>();
  for (int32 Index = 1; Index <= 10; ++Index) SelectionSlotPicker->AddOption(FString::Printf(TEXT("Slot %d"), Index));
  StylePreset(SelectionSlotPicker);
  auto* CurrentSystem = UHyperManageSystem::Get();
  const int32 ActiveSlot = CurrentSystem && CurrentSystem->Selection ? CurrentSystem->Selection->GetSelectionSlot() : 0;
  SelectionSlotPicker->SetSelectedIndex(ActiveSlot);
  SelectionSlotPicker->OnSelectionChanged.AddDynamic(this, &UHyperManageToolWidget::ChangeSelectionSlot);
  SelectionSlotPicker->SetToolTipText(FText::FromString(TEXT("Choose one of ten session-only selection slots. Changing slots does not change your current selection; use Remember or Recall.")));
  SlotRow->AddChildToHorizontalBox(SelectionSlotPicker)->SetPadding(FMargin(2));
  SelectionSlotStatus = WidgetTree->ConstructWidget<UTextBlock>();
  auto SlotFont = SelectionSlotStatus->GetFont(); SlotFont.Size = 11; SelectionSlotStatus->SetFont(SlotFont);
  SelectionSlotStatus->SetText(FText::FromString(TEXT("Empty | this session")));
  SlotRow->AddChildToHorizontalBox(SelectionSlotStatus)->SetVerticalAlignment(VAlign_Center);
  Groups->AddChildToVerticalBox(SlotRow);
  RemoveBoxEdgesButton = WidgetTree->ConstructWidget<UButton>();
  RemoveBoxEdgesButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::RemoveBoxEdges);
  RemoveBoxEdgesButton->SetToolTipText(FText::FromString(TEXT("Deselect objects inside the anchor/target edge-bounded region. Keeps anchor and target selected. Does not dismantle or move anything. Undo restores the removed selection.")));
  RemoveBoxCentersButton = WidgetTree->ConstructWidget<UButton>();
  RemoveBoxCentersButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::RemoveBoxCenters);
  RemoveBoxCentersButton->SetToolTipText(FText::FromString(TEXT("Deselect objects whose bounds centers lie between the anchor and target centers. Keeps the two reference objects selected. Does not dismantle objects. Undoable.")));
  RemoveBoxEdgesButton->SetIsEnabled(false); RemoveBoxCentersButton->SetIsEnabled(false);
  AddSlotButton = WidgetTree->ConstructWidget<UButton>();
  AddSlotButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::AddSelectionSlot); AddSlotButton->SetIsEnabled(false);
  AddSlotButton->SetToolTipText(FText::FromString(TEXT("Add remembered objects to this selection, excluding the saved target. Keep current anchor and target. Undoable; already-selected and unavailable objects are skipped.")));
  RemoveSlotButton = WidgetTree->ConstructWidget<UButton>();
  RemoveSlotButton->OnClicked.AddDynamic(this, &UHyperManageToolWidget::RemoveSelectionSlot); RemoveSlotButton->SetIsEnabled(false);
  RemoveSlotButton->SetToolTipText(FText::FromString(TEXT("Deselect this slot's objects, excluding its saved target. Keep current anchor and target. Does not dismantle objects or erase the slot. Undoable.")));
		AddGroup(TEXT("SELECTION"), {{btnNewSelection, TEXT("Clear")}, {btnSelectBoxSides, TEXT("Edges")}, {btnSelectBoxPivot, TEXT("Centers")}, {RemoveBoxEdgesButton, TEXT("Remove edges")}, {RemoveBoxCentersButton, TEXT("Remove centers")}, {btnSaveSelection, TEXT("Remember")}, {btnLoadSelection, TEXT("Recall")}, {AddSlotButton, TEXT("Add slot")}, {RemoveSlotButton, TEXT("Remove slot")}});
		AddGroup(TEXT("TRANSFORM"), {{btnIsGrouped, TEXT("Group mode")}, {btnIsViewBased, TEXT("View axes")}, {btnMoveSelection, TEXT("To target")}, {btnSameRotation, TEXT("Rotation")}, {btnSameScale, TEXT("Size")}, {btnSamePaint, TEXT("Paint")}});
		AddGroup(TEXT("CONNECTIONS & HISTORY"), {{btnConnect, TEXT("Connect")}, {btnDisconnect, TEXT("Disconnect")}, {btnClearUndo, TEXT("Clear history")}});
	}
	Body->SetContent(Content);
	Rows->AddChildToVerticalBox(Body);
	Frame->SetContent(Rows);
	auto* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Scroll->SetAlwaysShowScrollbar(true);
	// Wheel input belongs to the hovered transform buttons; use the scroll rail to navigate the tray.
	Scroll->SetWheelScrollMultiplier(0.f); Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
	Scroll->AddChild(Frame);
	// One continuous background covers the scroll area and footer without stacking opacity.
	auto* Rail = WidgetTree->ConstructWidget<UBorder>();
 Rail->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Transparent, FVector4(12, 0, 0, 12), FLinearColor(0.055f, 0.058f, 0.05f), 5.f));
 Rail->SetPadding(FMargin(5, 5, 0, 5)); Rail->SetContent(Scroll);
 auto* Face = WidgetTree->ConstructWidget<UVerticalBox>();
 Face->AddChildToVerticalBox(Rail)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 auto* Nameplate = WidgetTree->ConstructWidget<UTextBlock>();
 Nameplate->SetText(FText::FromString(TEXT("HYPERMANAGE  /  FIELD TOOLS"))); Nameplate->SetJustification(ETextJustify::Center);
 Face->AddChildToVerticalBox(Nameplate)->SetPadding(FMargin(4, 10, 4, 6));
 auto* Rim = WidgetTree->ConstructWidget<UBorder>();
 Rim->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.025f, 0.032f, 0.035f, 0.86f), FVector4(24, 0, 0, 24), FLinearColor(0.10f, 0.11f, 0.12f), 14.f));
 Rim->SetPadding(FMargin(14, 14, 0, 6)); Rim->SetContent(Face);
 auto* Edge = WidgetTree->ConstructWidget<UBorder>();
 Edge->SetBrush(FSlateRoundedBoxBrush(FLinearColor::Transparent, FVector4(27, 0, 0, 27), FLinearColor(0.045f, 0.05f, 0.055f), 3.f));
 Edge->SetPadding(FMargin(3, 3, 0, 3)); Edge->SetContent(Rim);
	auto* DockRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	auto* Handle = WidgetTree->ConstructWidget<UButton>(); StyleFieldButton(Handle);
	auto* Grip = WidgetTree->ConstructWidget<UTextBlock>(); Grip->SetText(FText::FromString(TEXT("||")));
	Handle->SetContent(Grip); Handle->SetToolTipText(FText::FromString(TEXT("Retract tool tray [Esc]")));
	Handle->OnClicked.AddDynamic(this, &UHyperManageToolWidget::CloseTools);
	DockRow->AddChildToHorizontalBox(Handle)->SetVerticalAlignment(VAlign_Center);
	DockRow->AddChildToHorizontalBox(Edge)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Window->AddChild(DockRow); DockedTray = Window;
	Window->SetRenderTranslation(FVector2D(430, 0));
	if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(Window->Slot)) {
		CanvasSlot->SetAnchors(FAnchors(1.f, 0.06f, 1.f, 0.93f));
		CanvasSlot->SetAlignment(FVector2D(1.f, 0.f));
		CanvasSlot->SetOffsets(FMargin(0, 0, 430, 0));
	}

}

void UHyperManageToolWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	TrayOpenTime = FMath::Min(TrayOpenTime + DeltaTime, 0.2f);
	if (DockedTray) DockedTray->SetRenderTranslation(FVector2D(430.f * FMath::Square(1.f - TrayOpenTime / 0.2f), 0));
	auto* System = UHyperManageSystem::Get();
	if (!System || !System->Config) return;
 if (System->Selection && SelectionSlotStatus) {
  const bool Saved = System->Selection->HasSavedSelection();
  const bool Pending = System->Selection->HasPendingOperations();
  const int32 SlotNumber = System->Selection->GetSelectionSlot() + 1;
  SelectionSlotStatus->SetText(FText::FromString(Saved ? FString::Printf(TEXT("%d objects | this session"), System->Selection->GetSavedSelectionCount()) : TEXT("Empty | this session")));
  if (btnSaveSelection) {
   btnSaveSelection->SetIsEnabled(!Pending);
   btnSaveSelection->SetToolTipText(FText::FromString(FString::Printf(TEXT("Remember current selection, anchor and target in Slot %d. Replaces this slot only; not saved with the game."), SlotNumber)));
  }
  if (RemoveSlotButton) {
   RemoveSlotButton->SetIsEnabled(Saved && !Pending && System->Selection->GetSavedSelectionCount() > 0 && System->Selection->SelectCount() > 0);
   RemoveSlotButton->SetToolTipText(FText::FromString(FString::Printf(TEXT("Remove Slot %d objects from the current selection. Keeps current anchor/target and excludes the saved target. Does not dismantle objects or erase the slot. Undo restores removed selection."), SlotNumber)));
  }
  if (AddSlotButton) {
   AddSlotButton->SetIsEnabled(Saved && !Pending && System->Selection->GetSavedSelectionCount() > 0);
   AddSlotButton->SetToolTipText(FText::FromString(FString::Printf(TEXT("Add Slot %d to the current selection. Excludes the saved target; preserves current anchor and target. Already-selected or unavailable objects are skipped. Undo removes only newly added objects."), SlotNumber)));
  }
  if (btnLoadSelection) {
   btnLoadSelection->SetIsEnabled(Saved && !Pending);
   btnLoadSelection->SetToolTipText(FText::FromString(FString::Printf(TEXT("Recall Slot %d, skipping unavailable objects. Undo restores your previous selection."), SlotNumber)));
  }
 }
 if (System->Selection && RemoveBoxEdgesButton && RemoveBoxCentersButton) {
  const bool Ready = !System->Selection->HasPendingOperations() && System->Selection->IsValidActor(System->Selection->AnchorActor) &&
   System->Selection->IsValidActor(System->Selection->TargetActor);
  RemoveBoxEdgesButton->SetIsEnabled(Ready); RemoveBoxCentersButton->SetIsEnabled(Ready);
 }
	if (System->Undo) {
		const bool Pending = System->Selection && System->Selection->HasPendingOperations();
  if (HistoryDetails && HistoryRevision != System->Undo->GetRevision()) {
   HistoryRevision = System->Undo->GetRevision();
   const auto UndoEntries = System->Undo->GetRecentDescriptions(false);
   const auto RedoEntries = System->Undo->GetRecentDescriptions(true);
   FString Details;
   auto Append = [&](const TCHAR* Heading, const TArray<FString>& Entries) {
    if (Entries.IsEmpty()) return;
    if (!Details.IsEmpty()) Details += TEXT("\n\n");
    Details += Heading;
    for (int32 Index = 0; Index < Entries.Num(); ++Index) Details += FString::Printf(TEXT("\n%d. %s"), Index + 1, *Entries[Index]);
   };
   Append(TEXT("UNDO - newest first"), UndoEntries); Append(TEXT("REDO - next first"), RedoEntries);
   HistoryDetails->SetText(FText::FromString(Details.IsEmpty() ? TEXT("No recorded edits yet.") : Details));
   if (UndoButton) UndoButton->SetToolTipText(FText::FromString(UndoEntries.IsEmpty() ? TEXT("Nothing to undo") : FString::Printf(TEXT("Undo (%d): %s"), System->Undo->GetUndoCount(), *UndoEntries[0])));
   if (RedoButton) RedoButton->SetToolTipText(FText::FromString(RedoEntries.IsEmpty() ? TEXT("Nothing to redo") : FString::Printf(TEXT("Redo (%d): %s"), System->Undo->GetRedoCount(), *RedoEntries[0])));
  }
		if (UndoButton) UndoButton->SetIsEnabled(!Pending && System->Undo->CanUndo());
		if (RedoButton) RedoButton->SetIsEnabled(!Pending && System->Undo->CanRedo());
	}
	if (ApplyOffsetButton && OffsetX && OffsetY && OffsetZ && System->Selection) {
		FHyperManageTransformData OffsetData;
  const bool ObjectAxes = OffsetAxes && OffsetAxes->GetSelectedIndex() == 1;
  FTransform Reference;
  const bool HasReference = !ObjectAxes || (System->Action && System->Action->GetWorldOrientationReference(Reference));
  const FVector Meters(OffsetX->GetValue(), OffsetY->GetValue(), OffsetZ->GetValue());
  const bool HasOffset = HasReference && (ObjectAxes ? UHyperManageTransform::MakeObjectOffset(Meters, Reference, OffsetData) : UHyperManageTransform::MakeWorldOffset(Meters, OffsetData));
		const int32 Count = System->Selection->SelectCount();
		const bool Pending = System->Selection->HasPendingOperations();
		ApplyOffsetButton->SetIsEnabled(Count > 0 && HasOffset && !Pending);
		if (OffsetStatus) OffsetStatus->SetText(FText::FromString(Pending ? TEXT("Waiting for the previous building edit...") :
			Count <= 0 ? TEXT("Select objects to move. The target stays in place.") :
			!HasReference ? TEXT("Object axes: select one object or select an anchor for the group.") :
   ObjectAxes && !HasOffset && !Meters.IsNearlyZero(0.000001) ? TEXT("Offset exceeds 1000 m on an input or world axis.") :
   ObjectAxes ? TEXT("Object directions | group spacing preserved | Z may tilt") : FString::Printf(TEXT("%d objects | world axes | Z changes height"), Count)));
	}
 if (ApplyPositionButton && ReadPositionButton && PositionX && PositionY && PositionZ && System->Action) {
  FVector Reference;
  const bool Available = System->Action->GetWorldPositionReference(Reference);
  FHyperManageTransformData PositionData;
  const FVector Destination(PositionX->GetValue(), PositionY->GetValue(), PositionZ->GetValue());
  const uint8 AxisMask = GetPositionAxisMask();
  const bool CanApply = Available && UHyperManageTransform::MakeWorldPositionOffset(Destination, Reference, PositionData, AxisMask);
  PositionX->SetIsEnabled((AxisMask & 1) != 0); PositionY->SetIsEnabled((AxisMask & 2) != 0); PositionZ->SetIsEnabled((AxisMask & 4) != 0);
  FVector Offset = Available ? Destination - Reference / 100.0 : FVector::ZeroVector;
  for (int32 Axis = 0; Axis < 3; ++Axis) if (!(AxisMask & (1 << Axis))) Offset[Axis] = 0;
  ReadPositionButton->SetIsEnabled(Available); ApplyPositionButton->SetIsEnabled(CanApply);
  const bool HasAnchor = System->Selection && System->Selection->AnchorActor != System->Selection->TargetActor && System->Selection->Contains(System->Selection->AnchorActor);
  if (PositionStatus) PositionStatus->SetText(FText::FromString(!Available ? TEXT("Select objects; wait for pending edits to finish.") :
   AxisMask == 0 ? TEXT("Check at least one axis to apply a position.") :
   Offset.GetAbsMax() > 1000.0 ? TEXT("Destination too far: maximum 1000 m per axis per apply.") :
   HasAnchor ? TEXT("Reference: selected anchor | preserve group spacing") : TEXT("Reference: selection center | preserve group spacing")));
 }
	if (ApplyRotationButton && OffsetYaw && OffsetPitch && OffsetRoll && System->Selection) {
		const bool HasRotation = UHyperManageTransform::IsValidRotationOffset(FRotator(OffsetPitch->GetValue(), OffsetYaw->GetValue(), OffsetRoll->GetValue()));
		const int32 Count = System->Selection->SelectCount();
		const bool Pending = System->Selection->HasPendingOperations();
		ApplyRotationButton->SetIsEnabled(Count > 0 && HasRotation && !Pending);
		const bool HasAnchor = System->Selection->AnchorActor != System->Selection->TargetActor && System->Selection->Contains(System->Selection->AnchorActor);
		if (RotationStatus) RotationStatus->SetText(FText::FromString(Pending ? TEXT("Waiting for the previous building edit...") :
			Count <= 0 ? TEXT("Select objects to rotate. The target stays in place.") :
			!System->Config->MMConfig.IsGrouped ? TEXT("Individual: rotate each object in place") :
			HasAnchor ? TEXT("Group: rotate around the selected anchor") : TEXT("Group: rotate around the selection center")));
	}
 if (ApplyOrientationButton && ReadOrientationButton && OrientationYaw && OrientationPitch && OrientationRoll && System->Action) {
  FTransform Reference;
  const bool Available = System->Action->GetWorldOrientationReference(Reference);
  FHyperManageTransformData Data;
  const FRotator Degrees(OrientationPitch->GetValue(), OrientationYaw->GetValue(), OrientationRoll->GetValue());
  ReadOrientationButton->SetIsEnabled(Available);
  const uint8 AxisMask = GetOrientationAxisMask();
  OrientationYaw->SetIsEnabled((AxisMask & 1) != 0); OrientationPitch->SetIsEnabled((AxisMask & 2) != 0); OrientationRoll->SetIsEnabled((AxisMask & 4) != 0);
  ApplyOrientationButton->SetIsEnabled(Available && UHyperManageTransform::MakeWorldOrientation(Degrees, Reference, Data, AxisMask));
  if (OrientationStatus) OrientationStatus->SetText(FText::FromString(!Available ? TEXT("Select one object, or select an anchor for a group. Wait for pending edits.") :
   AxisMask == 0 ? TEXT("Check at least one angle to apply an orientation.") : TEXT("Reference stays in place | group rotates around it")));
 }
 if (ReadScaleButton) {
  FTransform Reference;
  const bool Available = System->Action && System->Action->GetWorldOrientationReference(Reference);
  ReadScaleButton->SetIsEnabled(Available && UHyperManageTransform::IsValidScalePercent(Reference.GetScale3D() * 100.0));
 }
	if (ApplyScaleButton && ScaleX && ScaleY && ScaleZ && System->Selection) {
		const FVector Percent(ScaleX->GetValue(), ScaleY->GetValue(), ScaleZ->GetValue());
		const int32 Count = System->Selection->SelectCount();
		const bool Pending = System->Selection->HasPendingOperations();
		ApplyScaleButton->SetIsEnabled(Count > 0 && UHyperManageTransform::IsValidScalePercent(Percent) && !Pending);
		if (ScaleStatus) ScaleStatus->SetText(FText::FromString(Pending ? TEXT("Waiting for the previous building edit...") :
			Count <= 0 ? TEXT("Select objects to resize. The target stays in place.") : TEXT("Absolute local scale | origins and rotation unchanged")));
	}
	if (System->Selection && AnchorAlignmentStatus) {
		const bool HasAnchor = System->Selection->IsValidActor(System->Selection->AnchorActor) && System->Selection->Contains(System->Selection->AnchorActor);
		const bool Pending = System->Selection->HasPendingOperations();
		const int32 Count = System->Selection->SelectCount() - (HasAnchor ? 1 : 0);
		for (const auto& Button : AnchorAlignmentButtons) if (Button) Button->SetIsEnabled(HasAnchor && Count > 0 && !Pending);
		AnchorAlignmentStatus->SetText(FText::FromString(Pending ? TEXT("Waiting for the previous building edit...") :
			!HasAnchor ? TEXT("Set an anchor to align selected origins.") :
			Count <= 0 ? TEXT("Select another object to align to the anchor.") :
			FString::Printf(TEXT("Match %d origins to anchor | world axes"), Count)));
	}
	const auto& Config = System->Config->MMConfig;
	if (!Config.IncrementSettings.IsValidIndex(Config.IncrementSize)) return;
	const auto& Increment = Config.IncrementSettings[Config.IncrementSize];
	auto Sync = [](UComboBoxString* Combo, USpinBox* Input, float Value) {
		if (Input && !Input->HasKeyboardFocus() && !Input->HasFocusedDescendants() && Input->GetValue() != Value) Input->SetValue(Value);
		if (!Combo) return;
		const FString Text = FString::Printf(TEXT("%g"), Value);
		if (Combo->FindOptionIndex(Text) == INDEX_NONE) Combo->AddOption(Text);
		if (Combo->GetSelectedOption() != Text) Combo->SetSelectedOption(Text);
	};
	Sync(MovementPreset, MovementValue, Increment.CentimetersToMove / 100.f);
	Sync(RotationPreset, RotationValue, Increment.DegreesToRotate);
	Sync(GridPreset, GridValue, Config.AlignmentGridCm / 100.f);
	Sync(HeightGridPreset, HeightGridValue, Config.HeightGridCm / 100.f);
	if (PrecisionProfileLabel) PrecisionProfileLabel->SetText(FText::FromString(FString::Printf(TEXT("%s steps | shared grids"), *UEnum::GetDisplayValueAsText(Config.IncrementSize.GetValue()).ToString())));
	if (btnIsGrouped) {
		if (auto* Text = FieldButtonLabel(btnIsGrouped)) Text->SetText(FText::FromString(Config.IsGrouped ? TEXT("Together") : TEXT("Individual")));
	}
	if (btnIsViewBased) {
		if (auto* Text = FieldButtonLabel(btnIsViewBased)) Text->SetText(FText::FromString(Config.IsViewBased ? TEXT("View") : TEXT("Object")));
	}
}

void UHyperManageToolWidget::CommitMovementValue(float Value, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnCleared) return;
	if (auto* System = UHyperManageSystem::Get(); System && System->Config) {
		if (System->Config->SetPrecisionValue(EHyperManagePrecisionSetting::Movement, Value)) System->Config->SaveHyperManageConfig();
	}
}

void UHyperManageToolWidget::CommitRotationValue(float Value, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnCleared) return;
	if (auto* System = UHyperManageSystem::Get(); System && System->Config) {
		if (System->Config->SetPrecisionValue(EHyperManagePrecisionSetting::Rotation, Value)) System->Config->SaveHyperManageConfig();
	}
}

void UHyperManageToolWidget::CommitGridValue(float Value, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnCleared) return;
	if (auto* System = UHyperManageSystem::Get(); System && System->Config) {
		if (System->Config->SetPrecisionValue(EHyperManagePrecisionSetting::Grid, Value)) System->Config->SaveHyperManageConfig();
	}
}

void UHyperManageToolWidget::ChangeRotationPreset(FString Value, ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct) CommitRotationValue(FCString::Atof(*Value), ETextCommit::OnEnter);
}

void UHyperManageToolWidget::ChangeMovementPreset(FString Value, ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct) CommitMovementValue(FCString::Atof(*Value), ETextCommit::OnEnter);
}

void UHyperManageToolWidget::ChangeGridPreset(FString Value, ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct) CommitGridValue(FCString::Atof(*Value), ETextCommit::OnEnter);
}
void UHyperManageToolWidget::RepairQuickActions()
{
 if (!WidgetTree) return;
 auto* Scale = FindObjectFast<UScaleBox>(WidgetTree, TEXT("QuickActionScaleBox"));
 if (!Scale || !QuickActionHost) return;
 Scale->SetVisibility(ESlateVisibility::Collapsed);
 const TCHAR* Names[] = {TEXT("btnUpDown"), TEXT("btnLeftRight"), TEXT("btnFrontBack"), TEXT("btnSpin"), TEXT("btnPitch"), TEXT("btnRoll"), TEXT("btnGrowShrink")};
 const EActionNameIdx Decrease[] = {MoveDown, MoveLeft, MoveToward, SpinLeft, PitchToward, RollLeft, Shrink};
 const EActionNameIdx Increase[] = {MoveUp, MoveRight, MoveAway, SpinRight, PitchAway, RollRight, Grow};
 const TCHAR* Directions[] = {TEXT("down / up"), TEXT("left / right"), TEXT("toward / away"), TEXT("spin left / right"), TEXT("pitch toward / away"), TEXT("roll left / right"), TEXT("shrink / grow")};
 const TCHAR* Labels[] = {TEXT("Lift"), TEXT("Sideways"), TEXT("Forward"), TEXT("Yaw"), TEXT("Pitch"), TEXT("Roll"), TEXT("Scale")};
 auto* Panel = WidgetTree->ConstructWidget<UBorder>();
 Panel->SetBrushColor(FLinearColor::Transparent); Panel->SetPadding(FMargin(2));
 auto* Rows = WidgetTree->ConstructWidget<UVerticalBox>();
 auto AddLabel = [&](const TCHAR* Value, int32 Size) {
  auto* Label = WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Value));
  auto Font=Label->GetFont(); Font.Size=Size; Label->SetFont(Font);
  Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f,0.85f,0.68f)));
  if (Size == 16) {
   auto* Plate = WidgetTree->ConstructWidget<UBorder>(); Plate->SetBrushColor(FLinearColor(0.11f,0.085f,0.055f));
   Plate->SetPadding(FMargin(8,6)); Plate->SetContent(Label); Rows->AddChildToVerticalBox(Plate);
  } else Rows->AddChildToVerticalBox(Label);
 };
 AddLabel(TEXT("FIELD ADJUSTMENTS"),16);
 AddLabel(TEXT("Click - / + sides | Wheel: adjust"),12);
 auto* Grid=WidgetTree->ConstructWidget<UUniformGridPanel>(); Grid->SetSlotPadding(FMargin(3));
 for (int32 Index=0; Index<7; ++Index) {
  auto* Button=FindObjectFast<UButton>(WidgetTree,FName(Names[Index]));
  if (!Button) continue;
  Button->RemoveFromParent();
  auto* Label=WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Labels[Index]));
  AddFieldIcon(WidgetTree, Button, Label, Index);
  auto* Tile = CastChecked<USizeBox>(Button->GetContent());
  auto* Center = Tile->GetContent(); Center->RemoveFromParent();
  auto* Sides = WidgetTree->ConstructWidget<UHorizontalBox>();
  auto AddSign = [&](const TCHAR* Sign) {
   auto* Text = WidgetTree->ConstructWidget<UTextBlock>(); Text->SetText(FText::FromString(Sign));
   auto Font = Text->GetFont(); Font.Size = 13; Text->SetFont(Font);
   Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.82f, 0.5f)));
   Sides->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);
  };
  AddSign(TEXT("-")); Sides->AddChildToHorizontalBox(Center)->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); AddSign(TEXT("+"));
  Sides->SetVisibility(ESlateVisibility::HitTestInvisible); Tile->SetContent(Sides);
  auto* Proxy = UButtonProxy::Create(Increase[Index]); Proxy->DecreaseAction = Decrease[Index]; Proxy->SplitButton = Button;
  ButtonProxyArray.Add(Proxy); Button->OnPressed.Clear(); Button->OnPressed.AddDynamic(Proxy, &UButtonProxy::ClickSplitEvent);
  Button->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s: left - / right + = %s. Hover and scroll also adjusts. Uses the current step and reference frame. Drag the right scroll rail to navigate."), Labels[Index], Directions[Index])));
  Grid->AddChildToUniformGrid(Button,Index/4,Index%4);
 }
 Rows->AddChildToVerticalBox(Grid); Panel->SetContent(Rows); QuickActionHost->AddChildToVerticalBox(Panel);
 Scale->SetStretch(EStretch::ScaleToFit); Scale->SetStretchDirection(EStretchDirection::DownOnly);
}


void UHyperManageToolWidget::ApplyWorldOffset()
{
	if (!OffsetX || !OffsetY || !OffsetZ) return;
	if (auto* System = UHyperManageSystem::Get(); System && System->Action) {
		System->Action->ApplyWorldOffset(FVector(OffsetX->GetValue(), OffsetY->GetValue(), OffsetZ->GetValue()), OffsetAxes && OffsetAxes->GetSelectedIndex() == 1);
	}
}

void UHyperManageToolWidget::ClearWorldOffset()
{
	if (OffsetX) OffsetX->SetValue(0.f);
	if (OffsetY) OffsetY->SetValue(0.f);
	if (OffsetZ) OffsetZ->SetValue(0.f);
}


void UHyperManageToolWidget::ApplyWorldRotationOffset()
{
	if (!OffsetYaw || !OffsetPitch || !OffsetRoll) return;
	if (auto* System = UHyperManageSystem::Get(); System && System->Action) {
		System->Action->ApplyWorldRotationOffset(FRotator(OffsetPitch->GetValue(), OffsetYaw->GetValue(), OffsetRoll->GetValue()));
	}
}

void UHyperManageToolWidget::ClearRotationOffset()
{
	if (OffsetYaw) OffsetYaw->SetValue(0.f);
	if (OffsetPitch) OffsetPitch->SetValue(0.f);
	if (OffsetRoll) OffsetRoll->SetValue(0.f);
}


void UHyperManageToolWidget::ApplyScalePercent()
{
	if (!ScaleX || !ScaleY || !ScaleZ) return;
	if (auto* System = UHyperManageSystem::Get(); System && System->Action) {
		System->Action->ApplyScalePercent(FVector(ScaleX->GetValue(), ScaleY->GetValue(), ScaleZ->GetValue()));
	}
}

void UHyperManageToolWidget::ReadScale()
{
 if (!ScaleX || !ScaleY || !ScaleZ) return;
 FTransform Reference;
 if (auto* System = UHyperManageSystem::Get(); System && System->Action && System->Action->GetWorldOrientationReference(Reference)) {
  const FVector Percent = Reference.GetScale3D() * 100.0;
  if (!UHyperManageTransform::IsValidScalePercent(Percent)) return;
  if (ScalePreset) ScalePreset->ClearSelection();
  ScaleX->SetValue(Percent.X); ScaleY->SetValue(Percent.Y); ScaleZ->SetValue(Percent.Z);
 }
}

void UHyperManageToolWidget::ResetScaleFields()
{
	ClearScalePreset(100.f);
	if (ScaleX) ScaleX->SetValue(100.f);
	if (ScaleY) ScaleY->SetValue(100.f);
	if (ScaleZ) ScaleZ->SetValue(100.f);
}

void UHyperManageToolWidget::ChangeScalePreset(FString Value, ESelectInfo::Type SelectionType)
{
	float Percent = 0.f;
	if (LexTryParseString(Percent, *Value) && UHyperManageTransform::IsValidScalePercent(FVector(Percent))) {
		if (ScaleX) ScaleX->SetValue(Percent);
		if (ScaleY) ScaleY->SetValue(Percent);
		if (ScaleZ) ScaleZ->SetValue(Percent);
	}
}


void UHyperManageToolWidget::ClearScalePreset(float Value)
{
	if (ScalePreset && !ScalePreset->GetSelectedOption().IsEmpty()) ScalePreset->ClearSelection();
}


void UHyperManageToolWidget::CommitHeightGridValue(float Value, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnCleared) return;
	if (auto* System = UHyperManageSystem::Get(); System && System->Config) {
		if (System->Config->SetPrecisionValue(EHyperManagePrecisionSetting::HeightGrid, Value)) System->Config->SaveHyperManageConfig();
	}
}

void UHyperManageToolWidget::ChangeHeightGridPreset(FString Value, ESelectInfo::Type SelectionType)
{
	if (SelectionType != ESelectInfo::Direct) CommitHeightGridValue(FCString::Atof(*Value), ETextCommit::OnEnter);
}

uint8 UHyperManageToolWidget::GetPositionAxisMask() const
{
 uint8 Mask = 0;
 for (int32 Axis = 0; Axis < FMath::Min(PositionAxes.Num(), 3); ++Axis) if (PositionAxes[Axis] && PositionAxes[Axis]->IsChecked()) Mask |= 1 << Axis;
 return Mask;
}

void UHyperManageToolWidget::ReadWorldPosition()
{
 if (!PositionX || !PositionY || !PositionZ) return;
 FVector Reference;
 if (auto* System = UHyperManageSystem::Get(); System && System->Action && System->Action->GetWorldPositionReference(Reference)) {
  PositionX->SetValue(Reference.X / 100.0); PositionY->SetValue(Reference.Y / 100.0); PositionZ->SetValue(Reference.Z / 100.0);
 }
}

void UHyperManageToolWidget::ApplyWorldPosition()
{
 if (!PositionX || !PositionY || !PositionZ) return;
 if (auto* System = UHyperManageSystem::Get(); System && System->Action) {
  System->Action->ApplyWorldPosition(FVector(PositionX->GetValue(), PositionY->GetValue(), PositionZ->GetValue()), GetPositionAxisMask());
 }
}

uint8 UHyperManageToolWidget::GetOrientationAxisMask() const
{
 uint8 Mask = 0;
 for (int32 Axis = 0; Axis < FMath::Min(OrientationAxes.Num(), 3); ++Axis) if (OrientationAxes[Axis] && OrientationAxes[Axis]->IsChecked()) Mask |= 1 << Axis;
 return Mask;
}

void UHyperManageToolWidget::ReadWorldOrientation()
{
 if (!OrientationYaw || !OrientationPitch || !OrientationRoll) return;
 FTransform Reference;
 if (auto* System = UHyperManageSystem::Get(); System && System->Action && System->Action->GetWorldOrientationReference(Reference)) {
  const FRotator Degrees = Reference.Rotator();
  OrientationYaw->SetValue(Degrees.Yaw); OrientationPitch->SetValue(Degrees.Pitch); OrientationRoll->SetValue(Degrees.Roll);
 }
}

void UHyperManageToolWidget::ApplyWorldOrientation()
{
 if (!OrientationYaw || !OrientationPitch || !OrientationRoll) return;
 if (auto* System = UHyperManageSystem::Get(); System && System->Action) {
  System->Action->ApplyWorldOrientation(FRotator(OrientationPitch->GetValue(), OrientationYaw->GetValue(), OrientationRoll->GetValue()), GetOrientationAxisMask());
 }
}

void UHyperManageToolWidget::RemoveSelectionSlot()
{
 if (auto* System = UHyperManageSystem::Get(); System && System->Selection) System->Selection->RemoveSavedSelection();
}

void UHyperManageToolWidget::AddSelectionSlot()
{
 if (auto* System = UHyperManageSystem::Get(); System && System->Selection) System->Selection->AddSavedSelection();
}

void UHyperManageToolWidget::ChangeSelectionSlot(FString Value, ESelectInfo::Type SelectionType)
{
 if (!SelectionSlotPicker) return;
 if (auto* System = UHyperManageSystem::Get(); System && System->Selection) {
  System->Selection->SetSelectionSlot(SelectionSlotPicker->FindOptionIndex(Value));
 }
}

void UHyperManageToolWidget::RemoveBoxEdges()
{
 if (auto* System = UHyperManageSystem::Get(); System && System->Selection) System->Selection->ChangeAnchorTargetBoxSelection(true, true);
}

void UHyperManageToolWidget::RemoveBoxCenters()
{
 if (auto* System = UHyperManageSystem::Get(); System && System->Selection) System->Selection->ChangeAnchorTargetBoxSelection(false, true);
}
