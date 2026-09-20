#include "HyperManageActionGlyph.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Rendering/DrawElements.h"

TSharedRef<SWidget> UHyperManageActionGlyph::RebuildWidget()
{
 if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
 auto* Box = WidgetTree->ConstructWidget<USizeBox>();
 Box->SetWidthOverride(Compact ? 30 : 96); Box->SetHeightOverride(Compact ? 28 : 62);
 WidgetTree->RootWidget = Box;
 SetVisibility(ESlateVisibility::HitTestInvisible);
 return Super::RebuildWidget();
}

int32 UHyperManageActionGlyph::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
 FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& Style, bool ParentEnabled) const
{
 const int32 Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, Style, ParentEnabled) + 1;
 const FVector2D Offset = Geometry.GetLocalSize() * 0.5f;
 auto Line = [&](FVector2D A, FVector2D B, FLinearColor Color) {
  TArray<FVector2D> Points = {Offset + A * (Compact ? 0.40f : 1.f), Offset + B * (Compact ? 0.40f : 1.f)};
  FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Compact ? 1.3f : 1.8f);
 };
 const FLinearColor Ink(0.92f, 0.85f, 0.68f), Accent(0.95f, 0.57f, 0.18f);
 auto Arrow = [&](FVector2D A, FVector2D B) {
  Line(A, B, Accent); const FVector2D D = (B - A).GetSafeNormal(), S(-D.Y, D.X);
  Line(B, B - D * 6 + S * 3, Accent); Line(B, B - D * 6 - S * 3, Accent);
 };
 if (Kind >= 21 && Kind <= 23) {
  const float Direction = Kind == 22 ? -1.f : 1.f;
  Arrow(FVector2D(18 * Direction, -15), FVector2D(-23 * Direction, -15));
  Line(FVector2D(18 * Direction, -15), FVector2D(25 * Direction, 4), Ink);
  Line(FVector2D(25 * Direction, 4), FVector2D(12 * Direction, 20), Ink);
  Line(FVector2D(12 * Direction, 20), FVector2D(-12 * Direction, 20), Ink);
  if (Kind == 23) { Line(FVector2D(-5, -2), FVector2D(5, -2), Accent); Line(FVector2D(5, -2), FVector2D(5, 10), Accent); Line(FVector2D(5, 10), FVector2D(-5, 10), Accent); Line(FVector2D(-5, 10), FVector2D(-5, -2), Accent); }
  return Layer;
 }
 if (Kind >= 7) {
  auto Box = [&](float X, float Y, float W, float H) {
   Line(FVector2D(X,Y),FVector2D(X+W,Y),Ink); Line(FVector2D(X+W,Y),FVector2D(X+W,Y+H),Ink);
   Line(FVector2D(X+W,Y+H),FVector2D(X,Y+H),Ink); Line(FVector2D(X,Y+H),FVector2D(X,Y),Ink);
  };
  if (Kind == 7 || Kind == 8 || Kind == 9) {
   Box(-22,-20,44,40);
   if (Kind == 7) { Line(FVector2D(-12,-12),FVector2D(12,12),Accent); Line(FVector2D(12,-12),FVector2D(-12,12),Accent); }
   else if (Kind == 8) { Box(-29,-27,14,14); Box(15,13,14,14); }
   else { Line(FVector2D(-11,0),FVector2D(11,0),Accent); Line(FVector2D(0,-11),FVector2D(0,11),Accent); }
  } else if (Kind == 10 || Kind == 11) {
   Box(-23,-22,46,44); Box(-12,4,24,18);
   Arrow(FVector2D(0,Kind == 10 ? -30 : 0),FVector2D(0,Kind == 10 ? 0 : -30));
  } else if (Kind == 12) { Box(-28,-18,30,30); Box(-2,-8,30,30); }
  else if (Kind == 13) { Arrow(FVector2D(-22,20),FVector2D(25,20)); Arrow(FVector2D(-22,20),FVector2D(-22,-25)); Line(FVector2D(-22,20),FVector2D(15,-17),Ink); }
  else if (Kind == 14) { Box(-28,-13,20,26); Arrow(FVector2D(-5,0),FVector2D(26,0)); Line(FVector2D(28,-20),FVector2D(28,20),Ink); }
  else if (Kind == 15) { Box(-19,-21,38,13); Line(FVector2D(0,-8),FVector2D(0,1),Ink); Line(FVector2D(0,1),FVector2D(12,1),Ink); Line(FVector2D(12,1),FVector2D(12,23),Accent); }
  else if (Kind == 16 || Kind == 17) {
   Box(-30,-12,22,24); Box(8,-12,22,24);
   if (Kind == 16) Line(FVector2D(-8,0),FVector2D(8,0),Accent);
   else { Line(FVector2D(-5,-18),FVector2D(5,-8),Accent); Line(FVector2D(5,-18),FVector2D(-5,-8),Accent); }
  } else if (Kind == 18) {
   Arrow(FVector2D(19,-16),FVector2D(-22,-16)); Line(FVector2D(19,-16),FVector2D(25,6),Ink);
   Line(FVector2D(25,6),FVector2D(0,21),Ink); Line(FVector2D(-13,2),FVector2D(3,18),Accent); Line(FVector2D(3,2),FVector2D(-13,18),Accent);
  } else if (Kind == 19) {
   for (int32 I=-1; I<=1; ++I) { Line(FVector2D(I*19,-25),FVector2D(I*19,25),Ink); Line(FVector2D(-25,I*19),FVector2D(25,I*19),Ink); }
   Arrow(FVector2D(28,-28),FVector2D(4,-4));
  } else { Line(FVector2D(-27,14),FVector2D(27,14),Ink); Arrow(FVector2D(-16,-22),FVector2D(-16,7)); Arrow(FVector2D(16,-22),FVector2D(16,7)); }
  return Layer;
 }
 const FVector2D T(0,-13), L(-14,-6), R(14,-6), C(0,2), B(0,18);
 Line(T,L,Ink); Line(T,R,Ink); Line(L,C,Ink); Line(R,C,Ink); Line(C,B,Ink);
 Line(L,L+FVector2D(0,16),Ink); Line(R,R+FVector2D(0,16),Ink);
 Line(L+FVector2D(0,16),B,Ink); Line(R+FVector2D(0,16),B,Ink);
 if (Kind == 0) { Arrow(FVector2D(-25,14),FVector2D(-25,-20)); Arrow(FVector2D(25,-20),FVector2D(25,14)); }
 else if (Kind == 1) { Arrow(FVector2D(-18,2),FVector2D(-37,2)); Arrow(FVector2D(18,2),FVector2D(37,2)); }
 else if (Kind == 2) { Arrow(FVector2D(-19,13),FVector2D(-32,23)); Arrow(FVector2D(19,-10),FVector2D(32,-23)); }
 else if (Kind == 6) { Arrow(FVector2D(-18,-12),FVector2D(-31,-25)); Arrow(FVector2D(18,14),FVector2D(31,27)); }
 else {
  const float RX = Kind == 3 ? 32.f : (Kind == 4 ? 13.f : 27.f);
  const float RY = Kind == 3 ? 12.f : 26.f;
  FVector2D Previous = FVector2D::ZeroVector;
  for (int32 Step=0; Step<=15; ++Step) {
   const float Angle = -2.5f + Step * 0.31f;
   const FVector2D Point(FMath::Cos(Angle)*RX,FMath::Sin(Angle)*RY);
   if (Step > 0) { if (Step==15) Arrow(Previous,Point); else Line(Previous,Point,Accent); }
   Previous=Point;
  }
 }
 return Layer;
}
