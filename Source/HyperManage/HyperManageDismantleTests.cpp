#include "HyperManageRefundCapacity.h"
#include "HyperManageDismantlePlan.h"
#include "HyperManageDismantleReview.h"
#include "Buildables/FGBuildable.h"
#include "FGRecipe.h"
#include "HyperManageDismantleRefund.h"
#include "FGPlayerState.h"
#include "Resources/FGItemDescriptor.h"
#include "UObject/UnrealType.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageDismantlePlanTest, "HyperManage.Dismantle.DependencyPlan", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageDismantlePlanTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 UWorld* OtherWorld = UWorld::CreateWorld(EWorldType::Game, false);
 AActor* Parent = World->SpawnActor<AActor>();
 AActor* Child = World->SpawnActor<AActor>();
 AActor* Dependency = World->SpawnActor<AActor>();
 AActor* Target = World->SpawnActor<AActor>();
 AActor* Foreign = OtherWorld->SpawnActor<AActor>();
 TMap<AActor*, FHyperManageDismantlePlanner::FLinks> Links;
 Links.Add(Parent); Links.Add(Child); Links.Add(Dependency); Links.Add(Target);
 auto Resolve = [&](AActor* Actor, FHyperManageDismantlePlanner::FLinks& Out)
 {
  const auto* Found = Links.Find(Actor);
  if (!Found) return false;
  Out = *Found;
  return true;
 };
 auto Build = [&](const TArray<AActor*>& Selection)
 {
  return FHyperManageDismantlePlanner::BuildWithResolver(World, Selection, Target, Resolve);
 };
 auto CheckFailure = [&](const FHyperManageDismantlePlan& Plan, EHyperManageDismantlePlanStatus Status)
 {
  TestTrue(TEXT("Expected rejection reason"), Plan.Status == Status);
  TestTrue(TEXT("Failure exposes no partial execution list"), Plan.OrderedActors.IsEmpty());
  TestEqual(TEXT("Failure exposes no partial child count"), Plan.AddedChildren, 0);
 };
 CheckFailure(Build({}), EHyperManageDismantlePlanStatus::EmptySelection);
 CheckFailure(Build({Parent, nullptr}), EHyperManageDismantlePlanStatus::InvalidActor);
 CheckFailure(Build({Foreign}), EHyperManageDismantlePlanStatus::InvalidActor);
 CheckFailure(Build({Parent, Target}), EHyperManageDismantlePlanStatus::ProtectedTarget);
 CheckFailure(FHyperManageDismantlePlanner::Build(nullptr, {Parent}, Target), EHyperManageDismantlePlanStatus::NotAuthority);
 CheckFailure(FHyperManageDismantlePlanner::Build(World, {Parent}, Target), EHyperManageDismantlePlanStatus::UnsupportedActor);

 Links[Parent].Children = {Child, Child};
 Links[Parent].Dependencies = {Dependency, Dependency};
 auto Plan = Build({Parent});
 CheckFailure(Plan, EHyperManageDismantlePlanStatus::MissingDependency);
 TestTrue(TEXT("Reports the unselected dependency without including it"), Plan.RequiredActor.Get() == Dependency);
 Plan = Build({Parent, Dependency, Parent});
 TestTrue(TEXT("Complete closure accepted"), Plan.Status == EHyperManageDismantlePlanStatus::Ready);
 TestEqual(TEXT("Roots and shared children deduplicated"), Plan.OrderedActors.Num(), 3);
 TestEqual(TEXT("Added child count excludes explicit selection"), Plan.AddedChildren, 1);
 if (Plan.OrderedActors.Num() == 3)
 {
  TestTrue(TEXT("Dependency and child precede parent"), Plan.OrderedActors.Last().Get() == Parent);
  TestTrue(TEXT("Independent nodes preserve discovery order"), Plan.OrderedActors[0].Get() == Dependency);
 }
 Plan = Build({Parent, Child, Dependency});
 TestEqual(TEXT("Explicit child is not counted as an addition"), Plan.AddedChildren, 0);
 Links[Child].Dependencies = {Dependency};
 Plan = Build({Parent, Dependency});
 TestTrue(TEXT("Shared dependency works"), Plan.Status == EHyperManageDismantlePlanStatus::Ready);
 Links[Dependency].Dependencies = {Parent};
 CheckFailure(Build({Parent, Dependency}), EHyperManageDismantlePlanStatus::DependencyCycle);
 Links[Dependency].Dependencies.Empty();
 Links[Child].Dependencies = {Child};
 CheckFailure(Build({Parent, Dependency}), EHyperManageDismantlePlanStatus::DependencyCycle);
 Links[Child].Dependencies.Empty();
 Links[Child].Children = {Target};
 CheckFailure(Build({Parent, Dependency}), EHyperManageDismantlePlanStatus::ProtectedTarget);
 Links[Child].Children.Empty();
 Links[Child].Dependencies = {Target};
 CheckFailure(Build({Parent, Dependency}), EHyperManageDismantlePlanStatus::ProtectedTarget);
 Links[Child].Dependencies.Empty();
 Links[Child].Children = {Foreign};
 CheckFailure(Build({Parent, Dependency}), EHyperManageDismantlePlanStatus::InvalidActor);
 Links[Child].Children.Empty();
 Links.Remove(Child);
 CheckFailure(Build({Parent, Dependency}), EHyperManageDismantlePlanStatus::UnsupportedActor);
 Links.Add(Child);
 TestTrue(TEXT("Planning never destroys input actors"), IsValid(Parent) && IsValid(Child) && IsValid(Dependency) && IsValid(Target));

 TArray<AActor*> Chain;
 for (int32 Index = 0; Index < FHyperManageDismantlePlanner::MaxActors + 1; ++Index)
 {
  AActor* Actor = World->SpawnActor<AActor>();
  Chain.Add(Actor); Links.Add(Actor);
  if (Index > 0) Links[Chain[Index - 1]].Children.Add(Actor);
 }
 CheckFailure(Build({Chain[0]}), EHyperManageDismantlePlanStatus::TooManyActors);
 Links[Chain[FHyperManageDismantlePlanner::MaxActors - 1]].Children.Empty();
 Plan = Build({Chain[0]});
 TestTrue(TEXT("Maximum length chain accepted without recursion"), Plan.Status == EHyperManageDismantlePlanStatus::Ready);
 TestEqual(TEXT("Full bounded chain retained"), Plan.OrderedActors.Num(), FHyperManageDismantlePlanner::MaxActors);
 if (!Plan.OrderedActors.IsEmpty()) TestTrue(TEXT("Deepest child is first"), Plan.OrderedActors[0].Get() == Chain[FHyperManageDismantlePlanner::MaxActors - 1]);
 Child->Destroy();
 CheckFailure(Build({Parent, Dependency}), EHyperManageDismantlePlanStatus::InvalidActor);
 OtherWorld->DestroyWorld(false);
 World->DestroyWorld(false);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageDismantleRefundTest, "HyperManage.Dismantle.RefundPreview", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageDismantleRefundTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 UWorld* OtherWorld = UWorld::CreateWorld(EWorldType::Game, false);
 AActor* First = World->SpawnActor<AActor>();
 AActor* Second = World->SpawnActor<AActor>();
 AActor* Foreign = OtherWorld->SpawnActor<AActor>();
 auto* Player = World->SpawnActor<AFGPlayerState>();
 FHyperManageDismantlePlan Plan;
 Plan.Status = EHyperManageDismantlePlanStatus::Ready;
 Plan.OrderedActors = {First, Second};
 auto MakeStack = [&](int32 Count)
 {
  FInventoryStack Stack;
  // The SDK's item constructors/setters are stubs; initialize reflected fixture data.
  FInventoryStack::StaticStruct()->InitializeStruct(&Stack);
  auto* ClassProperty = FindFProperty<FClassProperty>(FInventoryItem::StaticStruct(), TEXT("ItemClass"));
  ClassProperty->SetObjectPropertyValue_InContainer(&Stack.Item, UFGItemDescriptor::StaticClass());
  Stack.NumItems = Count;
  Stack.Item.LegacyItemStateActor = First;
  Stack.Item.CachedStackSize = 17;
  return Stack;
 };
 int32 Reads = 0;
 bool SawNoCost = false;
 auto Read = [&](AActor* Actor, bool NoCost, TArray<FInventoryStack>& Stacks)
 {
  ++Reads; SawNoCost = NoCost;
  Stacks.Add(MakeStack(NoCost ? 2 : 12));
  Stacks.Add(MakeStack(3));
  Stacks.Add(MakeStack(0));
  return true;
 };
 auto CheckFailure = [&](const FHyperManageRefundPreview& Preview, EHyperManageRefundStatus Status)
 {
  TestTrue(TEXT("Refund rejection reason"), Preview.Status == Status);
  TestTrue(TEXT("No partial refund exposed"), Preview.Actors.IsEmpty());
 };
 auto Preview = FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, Read);
 TestTrue(TEXT("Refund snapshot succeeds"), Preview.Status == EHyperManageRefundStatus::Ready);
 TestEqual(TEXT("Each actor queried once"), Reads, 2);
 TestEqual(TEXT("Actor attribution retained"), Preview.Actors.Num(), 2);
 if (Preview.Actors.Num() == 2)
 {
  TestTrue(TEXT("Dismantle ordering retained"), Preview.Actors[0].Actor.Get() == First && Preview.Actors[1].Actor.Get() == Second);
  TestEqual(TEXT("Same-descriptor stacks remain separate, zero entry omitted"), Preview.Actors[0].Stacks.Num(), 2);
  if (Preview.Actors[0].Stacks.Num() == 2)
  {
   TestEqual(TEXT("Quantity unchanged"), Preview.Actors[0].Stacks[0].NumItems, 12);
   TestTrue(TEXT("Legacy state reference preserved"), Preview.Actors[0].Stacks[0].Item.LegacyItemStateActor == First);
   TestEqual(TEXT("Item metadata preserved"), Preview.Actors[0].Stacks[0].Item.CachedStackSize, 17);
  }
 }
 Preview = FHyperManageDismantleRefunds::BuildWithReader(World, Plan, true, Read);
 TestTrue(TEXT("No-cost rule passed to reader and recorded"), SawNoCost && Preview.NoBuildCost);
 if (Preview.Actors.Num() == 2) TestEqual(TEXT("Contents-only result retained"), Preview.Actors[0].Stacks[0].NumItems, 2);
 Reads = 0;
 Plan.OrderedActors.Add(First);
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, Read), EHyperManageRefundStatus::InvalidPlan);
 TestEqual(TEXT("Duplicate plan rejected before querying any refund"), Reads, 0);
 Plan.OrderedActors = {First, Foreign};
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, Read), EHyperManageRefundStatus::InvalidActor);
 TestEqual(TEXT("Foreign plan rejected before querying any refund"), Reads, 0);
 Plan.OrderedActors = {First, Second};
 auto BadStack = [&](AActor*, bool, TArray<FInventoryStack>& Stacks) { Stacks.Add(MakeStack(-1)); return true; };
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, BadStack), EHyperManageRefundStatus::InvalidStack);
 auto MissingClass = [&](AActor*, bool, TArray<FInventoryStack>& Stacks)
 {
  FInventoryStack Stack = MakeStack(1);
  FindFProperty<FClassProperty>(FInventoryItem::StaticStruct(), TEXT("ItemClass"))->SetObjectPropertyValue_InContainer(&Stack.Item, nullptr);
  Stacks.Add(Stack); return true;
 };
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, MissingClass), EHyperManageRefundStatus::InvalidStack);
 auto Unsupported = [&](AActor* Actor, bool NoCost, TArray<FInventoryStack>& Stacks)
 { return Actor == First ? Read(Actor, NoCost, Stacks) : false; };
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, Unsupported), EHyperManageRefundStatus::UnsupportedActor);
 auto TooMany = [&](AActor*, bool, TArray<FInventoryStack>& Stacks)
 { Stacks.Init(MakeStack(1), FHyperManageDismantleRefunds::MaxStacks / 2 + 1); return true; };
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, TooMany), EHyperManageRefundStatus::TooManyStacks);
 auto Empty = [](AActor*, bool, TArray<FInventoryStack>&) { return true; };
 TestTrue(TEXT("No refund is valid for a dismantlable actor"),
  FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, Empty).Status == EHyperManageRefundStatus::Ready);
 CheckFailure(FHyperManageDismantleRefunds::Build(World, Plan, nullptr), EHyperManageRefundStatus::InvalidPlayer);
 CheckFailure(FHyperManageDismantleRefunds::Build(OtherWorld, Plan, Player), EHyperManageRefundStatus::InvalidPlayer);
 CheckFailure(FHyperManageDismantleRefunds::Build(World, Plan, Player), EHyperManageRefundStatus::UnsupportedActor);
 Plan.Status = EHyperManageDismantlePlanStatus::DependencyCycle;
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, Read), EHyperManageRefundStatus::InvalidPlan);
 Plan.Status = EHyperManageDismantlePlanStatus::Ready;
 TestTrue(TEXT("Refund queries do not destroy buildings"), IsValid(First) && IsValid(Second));
 Second->Destroy();
 CheckFailure(FHyperManageDismantleRefunds::BuildWithReader(World, Plan, false, Read), EHyperManageRefundStatus::InvalidActor);
 OtherWorld->DestroyWorld(false); World->DestroyWorld(false);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageLightweightRefundTest, "HyperManage.Dismantle.LightweightRefund", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageLightweightRefundTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 FRuntimeBuildableInstanceData Data;
 Data.Transform = FTransform::Identity;
 Data.BuiltWithRecipe = UFGRecipe::StaticClass();
 Data.Handles.AddDefaulted();
 FHyperManageLightweightRef Ref;
 Ref.BuildableClass = AFGBuildable::StaticClass(); Ref.Index = 0; Ref.Recipe = Data.BuiltWithRecipe;
 auto Other = Ref; Other.Index = 1;
 TArray<FHyperManageLightweightRef> Refs = {Ref, Other};
 auto Resolve = [&](const FHyperManageLightweightRef&) { return &Data; };
 int32 Reads = 0;
 auto Read = [&](const FHyperManageLightweightRef&, const FRuntimeBuildableInstanceData& Actual, TArray<FInventoryStack>& Stacks)
 {
  ++Reads;
  TestTrue(TEXT("Reader receives the original record, including type-specific data"), &Actual == &Data);
  FInventoryStack Stack;
  FInventoryStack::StaticStruct()->InitializeStruct(&Stack);
  FindFProperty<FClassProperty>(FInventoryItem::StaticStruct(), TEXT("ItemClass"))->SetObjectPropertyValue_InContainer(&Stack.Item, UFGItemDescriptor::StaticClass());
  Stack.NumItems = 7;
  Stacks.Add(Stack);
 };
 auto Build = [&](bool NoCost = false) { return FHyperManageDismantleRefunds::BuildLightweightsWithReader(World, Refs, NoCost, Resolve, Read); };
 auto Reject = [&](const FHyperManageRefundPreview& Preview, EHyperManageRefundStatus Status)
 {
  TestTrue(TEXT("Expected lightweight refund rejection"), Preview.Status == Status);
  TestTrue(TEXT("Failure exposes no instance refunds"), Preview.Instances.IsEmpty());
 };
 auto Preview = Build();
 TestTrue(TEXT("Live identity accepted"), Preview.Status == EHyperManageRefundStatus::Ready);
 TestEqual(TEXT("Each instance read once"), Reads, 2);
 TestEqual(TEXT("Separate instance attribution"), Preview.Instances.Num(), 2);
 if (Preview.Instances.Num() == 2)
 {
  TestEqual(TEXT("Second instance index retained"), Preview.Instances[1].Ref.Index, 1);
  if (Preview.Instances[0].Stacks.Num() == 1) TestEqual(TEXT("Refund amount retained"), Preview.Instances[0].Stacks[0].NumItems, 7);
  else AddError(TEXT("Expected one refund stack"));
 }
 Reads = 0;
 Preview = Build(true);
 TestTrue(TEXT("No-cost preview succeeds"), Preview.Status == EHyperManageRefundStatus::Ready && Preview.NoBuildCost);
 TestEqual(TEXT("No-cost does not call construction refund API"), Reads, 0);
 for (const auto& Entry : Preview.Instances) TestTrue(TEXT("No free construction materials"), Entry.Stacks.IsEmpty());
 Refs.Add(Ref);
 Reject(Build(), EHyperManageRefundStatus::InvalidPlan);
 TestEqual(TEXT("Duplicate instance rejected before refund queries"), Reads, 0);
 Refs = {Ref, Other};
 Data.Transform.SetLocation(FVector(100, 0, 0));
 Reject(Build(), EHyperManageRefundStatus::InvalidInstance);
 Reject(Build(true), EHyperManageRefundStatus::InvalidInstance);
 Data.Transform = FTransform::Identity;
 Data.BuiltWithRecipe = nullptr;
 Reject(Build(), EHyperManageRefundStatus::InvalidInstance);
 Data.BuiltWithRecipe = UFGRecipe::StaticClass();
 Data.Handles.Empty();
 Reject(Build(), EHyperManageRefundStatus::InvalidInstance);
 Data.Handles.AddDefaulted();
 Refs[1].Index = -1;
 Reject(Build(), EHyperManageRefundStatus::InvalidInstance);
 Refs[1] = Other; Refs[1].BuildableClass = nullptr;
 Reject(Build(), EHyperManageRefundStatus::InvalidInstance);
 Refs = {Ref, Other};
 auto Missing = [](const FHyperManageLightweightRef&) -> const FRuntimeBuildableInstanceData* { return nullptr; };
 Reject(FHyperManageDismantleRefunds::BuildLightweightsWithReader(World, Refs, false, Missing, Read), EHyperManageRefundStatus::InvalidInstance);
 auto InvalidStack = [&](const FHyperManageLightweightRef& Item, const FRuntimeBuildableInstanceData& Actual, TArray<FInventoryStack>& Stacks)
 { Read(Item, Actual, Stacks); Stacks[0].NumItems = -1; };
 Reject(FHyperManageDismantleRefunds::BuildLightweightsWithReader(World, Refs, false, Resolve, InvalidStack), EHyperManageRefundStatus::InvalidStack);
 auto Changed = [&](const FHyperManageLightweightRef& Item, const FRuntimeBuildableInstanceData& Actual, TArray<FInventoryStack>& Stacks)
 { Read(Item, Actual, Stacks); if (Item.Index == 1) Data.Transform.SetLocation(FVector(100, 0, 0)); };
 Reject(FHyperManageDismantleRefunds::BuildLightweightsWithReader(World, Refs, false, Resolve, Changed), EHyperManageRefundStatus::InvalidInstance);
 Data.Transform = FTransform::Identity;
 Refs.Init(Ref, FHyperManageDismantlePlanner::MaxActors + 1);
 Reject(Build(), EHyperManageRefundStatus::InvalidPlan);
 Refs.Empty(); Reject(Build(), EHyperManageRefundStatus::InvalidPlan);
 Reject(FHyperManageDismantleRefunds::BuildLightweights(World, {Ref}, nullptr), EHyperManageRefundStatus::InvalidPlayer);
 World->DestroyWorld(false);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageDismantleReviewTest, "HyperManage.Dismantle.SelectionReview", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageDismantleReviewTest::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
 AActor* Actor = World->SpawnActor<AActor>();
 AActor* Child = World->SpawnActor<AActor>();
 AActor* Target = World->SpawnActor<AActor>();
 auto* Proxy = World->SpawnActor<AHyperManageLightweightProxy>();
 auto* Alias = World->SpawnActor<AHyperManageLightweightProxy>();
 Proxy->Ref.BuildableClass = AFGBuildable::StaticClass(); Proxy->Ref.Index = 3;
 Alias->Ref = Proxy->Ref;
 FHyperManageDismantlePlan Plan;
 Plan.Status = EHyperManageDismantlePlanStatus::Ready; Plan.OrderedActors = {Child, Actor}; Plan.AddedChildren = 1;
 FHyperManageRefundPreview Native, Lightweight;
 Native.Status = Lightweight.Status = EHyperManageRefundStatus::Ready;
 Native.Actors.AddDefaulted(2); Native.Actors[0].Actor = Child; Native.Actors[1].Actor = Actor;
 Lightweight.Instances.AddDefaulted(); Lightweight.Instances[0].Ref = Proxy->Ref;
 int32 Plans = 0, NativeReads = 0, LightReads = 0;
 auto PlanSource = [&](const TArray<AActor*>& Input) {
  ++Plans; TestEqual(TEXT("Native input deduplicated"), Input.Num(), 1); return Plan;
 };
 auto NativeSource = [&](const FHyperManageDismantlePlan&) { ++NativeReads; return Native; };
 auto LightSource = [&](const TArray<FHyperManageLightweightRef>& Input) {
  ++LightReads; TestEqual(TEXT("One lightweight input"), Input.Num(), 1); return Lightweight;
 };
 int32 EligibilityReads = 0;
 bool BlockChild = false, WarnChild = false, EligibilityAvailable = true;
 auto EligibilitySource = [&](AActor* Item, const TArray<AActor*>& Group, FHyperManageDismantleEligibility& Out) {
  ++EligibilityReads;
  TestTrue(TEXT("Eligibility sees the complete native group, including added children"), Group.Contains(Actor) && Group.Contains(Child));
  Out.CanDismantle = !(BlockChild && Item == Child);
  if (WarnChild && Item == Child) Out.Reasons = {TEXT("Example game warning"), TEXT("Example game warning")};
  return EligibilityAvailable;
 };
 auto Build = [&](const TArray<AActor*>& Input, AActor* Protected = nullptr) {
  return FHyperManageDismantleReviewer::BuildWithSources(World, Input, Protected, PlanSource, NativeSource, LightSource, EligibilitySource);
 };
 auto Reject = [&](const FHyperManageDismantleReview& Review) {
  TestFalse(TEXT("Failure explains why"), Review.Error.IsEmpty());
  TestTrue(TEXT("Failure exposes no partial refunds"), Review.Refunds.Actors.IsEmpty() && Review.Refunds.Instances.IsEmpty());
  TestTrue(TEXT("Failure is not ready"), Review.Refunds.Status != EHyperManageRefundStatus::Ready);
 };
 auto Review = Build({Actor, Proxy, Actor}, Target);
 TestTrue(TEXT("Mixed selection reviewed"), Review.Error.IsEmpty() && Review.Refunds.Status == EHyperManageRefundStatus::Ready);
 TestEqual(TEXT("Native child included"), Review.Refunds.Actors.Num(), 2);
 TestEqual(TEXT("Lightweight included"), Review.Refunds.Instances.Num(), 1);
 TestEqual(TEXT("Added children disclosed"), Review.AddedChildren, 1);
 TestEqual(TEXT("One plan call"), Plans, 1);
 TestEqual(TEXT("All native actors checked"), Review.NativeChecked, 2);
 TestEqual(TEXT("No blocked actors by default"), Review.NativeBlocked, 0);
 BlockChild = WarnChild = true;
 Review = Build({Actor, Proxy});
 TestTrue(TEXT("Blocked eligibility still allows read-only refund estimate"), Review.Error.IsEmpty());
 TestEqual(TEXT("Blocked child counted"), Review.NativeBlocked, 1);
 TestEqual(TEXT("Warning actor counted once"), Review.NativeWarnings, 1);
 TestEqual(TEXT("Repeated reasons deduplicated"), Review.EligibilityReasons.Num(), 1);
 TestEqual(TEXT("Refunds retained with eligibility diagnostics"), Review.Refunds.Actors.Num(), 2);
 BlockChild = false;
 Review = Build({Actor, Proxy});
 TestEqual(TEXT("Advisory warning does not claim a hard block"), Review.NativeBlocked, 0);
 TestEqual(TEXT("Advisory warning still shown"), Review.NativeWarnings, 1);
 WarnChild = false; EligibilityAvailable = false;
 NativeReads = LightReads = 0;
 Reject(Build({Actor, Proxy}));
 TestEqual(TEXT("Failed eligibility query prevents partial refunds"), NativeReads + LightReads, 0);
 EligibilityAvailable = true;
 EligibilityReads = 0;
 NativeReads = LightReads = Plans = 0;
 Review = Build({Proxy});
 TestTrue(TEXT("Lightweight-only review works"), Review.Error.IsEmpty());
 TestEqual(TEXT("Lightweight eligibility is not claimed"), EligibilityReads, 0);
 TestEqual(TEXT("No native eligibility count for lightweight-only selection"), Review.NativeChecked, 0);
 TestEqual(TEXT("No empty native plan query"), Plans, 0);
 TestEqual(TEXT("No empty native refund query"), NativeReads, 0);
 LightReads = 0; Review = Build({Actor});
 TestTrue(TEXT("Native-only review works"), Review.Error.IsEmpty());
 TestEqual(TEXT("No empty lightweight query"), LightReads, 0);
 Reject(Build({})); Reject(Build({nullptr})); Reject(Build({Actor}, Actor)); Reject(Build({Proxy}, Alias));
 Plan.Status = EHyperManageDismantlePlanStatus::MissingDependency;
 NativeReads = LightReads = 0; Reject(Build({Actor, Proxy}));
 TestEqual(TEXT("Bad plan blocks all refund queries"), NativeReads + LightReads, 0);
 Plan.Status = EHyperManageDismantlePlanStatus::Ready;
 Lightweight.Status = EHyperManageRefundStatus::InvalidInstance; Reject(Build({Actor, Proxy}));
 Lightweight.Status = EHyperManageRefundStatus::Ready;
 Native.Status = EHyperManageRefundStatus::InvalidStack; Reject(Build({Actor, Proxy})); Native.Status = EHyperManageRefundStatus::Ready;
 Lightweight.NoBuildCost = true; Reject(Build({Actor, Proxy})); Lightweight.NoBuildCost = false;
 // Each individual preview is within its own stack limit, but their combined result is not.
 Native.Actors[0].Stacks.SetNum(FHyperManageDismantleRefunds::MaxStacks / 2 + 1);
 Lightweight.Instances[0].Stacks.SetNum(FHyperManageDismantleRefunds::MaxStacks / 2 + 1);
 Reject(Build({Actor, Proxy}));
 Native.Actors[0].Stacks.Empty(); Lightweight.Instances[0].Stacks.Empty();
 Plan.OrderedActors.Init(Actor, FHyperManageDismantlePlanner::MaxActors);
 NativeReads = LightReads = 0; Reject(Build({Actor, Proxy}));
 TestEqual(TEXT("Combined actor limit checked before refunds"), NativeReads + LightReads, 0);
 Plan.OrderedActors = {Child, Actor};
 Proxy->BeginRequest(); Reject(Build({Actor, Proxy}));
 TestTrue(TEXT("Review leaves buildings intact"), IsValid(Actor) && IsValid(Child) && IsValid(Proxy));
 World->DestroyWorld(false);
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHyperManageRefundCapacityTest, "HyperManage.Dismantle.RefundCapacity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHyperManageRefundCapacityTest::RunTest(const FString& Parameters)
{
 FHyperManageRefundPreview Preview;
 Preview.Status = EHyperManageRefundStatus::Ready;
 Preview.Actors.AddDefaulted(); Preview.Instances.AddDefaulted();
 auto MakeStack = [](int32 Count) {
  FInventoryStack Stack; FInventoryStack::StaticStruct()->InitializeStruct(&Stack);
  FindFProperty<FClassProperty>(FInventoryItem::StaticStruct(), TEXT("ItemClass"))->SetObjectPropertyValue_InContainer(&Stack.Item, UFGItemDescriptor::StaticClass());
  Stack.NumItems = Count; Stack.Item.CachedStackSize = 42; return Stack;
 };
 Preview.Actors[0].Stacks.Add(MakeStack(3)); Preview.Instances[0].Stacks.Add(MakeStack(5));
 int32 Calls = 0;
 bool Fits = true;
 auto Read = [&](const TArray<FInventoryStack>& Batch) {
  ++Calls;
  TestEqual(TEXT("One batch contains both refund sources"), Batch.Num(), 2);
  if (Batch.Num() == 2) {
   TestEqual(TEXT("Native amount retained"), Batch[0].NumItems, 3);
   TestEqual(TEXT("Lightweight amount retained"), Batch[1].NumItems, 5);
   TestEqual(TEXT("Same-class stacks remain separate with metadata"), Batch[1].Item.CachedStackSize, 42);
  }
  return Fits;
 };
 auto Check = [&] { return FHyperManageRefundCapacity::CheckWithReader(Preview, Read); };
 TestTrue(TEXT("Whole batch fits"), Check() == EHyperManageRefundCapacity::Fits);
 TestEqual(TEXT("Exactly one query, not independent per-stack queries"), Calls, 1);
 Fits = false;
 TestTrue(TEXT("Rejected whole batch needs overflow handling"), Check() == EHyperManageRefundCapacity::NeedsOverflow);
 TestEqual(TEXT("Input amount unchanged"), Preview.Actors[0].Stacks[0].NumItems, 3);
 Calls = 0; Preview.Status = EHyperManageRefundStatus::InvalidPlan;
 TestTrue(TEXT("Failed review is not evaluated"), Check() == EHyperManageRefundCapacity::Unavailable);
 Preview.Status = EHyperManageRefundStatus::Ready;
 Preview.Actors[0].Stacks[0].NumItems = -1;
 TestTrue(TEXT("Negative refund unavailable"), Check() == EHyperManageRefundCapacity::Unavailable);
 Preview.Actors[0].Stacks[0].NumItems = MAX_int32;
 TestTrue(TEXT("Huge quantity skipped without integer overflow"), Check() == EHyperManageRefundCapacity::TooLarge);
 Preview.Actors[0].Stacks.Init(MakeStack(1), FHyperManageRefundCapacity::MaxCheckStacks);
 TestTrue(TEXT("Combined stack limit enforced"), Check() == EHyperManageRefundCapacity::TooLarge);
 TestEqual(TEXT("Invalid and large batches never reach game query"), Calls, 0);
 Preview.Actors[0].Stacks.Empty(); Preview.Instances[0].Stacks.Empty();
 TestTrue(TEXT("Empty refund needs no capacity"), Check() == EHyperManageRefundCapacity::NoRefund);
 TestEqual(TEXT("No query for empty refunds"), Calls, 0);
 Preview.Actors[0].Stacks.Add(MakeStack(0));
 TestTrue(TEXT("Zero entries ignored"), Check() == EHyperManageRefundCapacity::NoRefund);
 Preview.Actors[0].Stacks[0].NumItems = FHyperManageRefundCapacity::MaxCheckItems;
 int32 BoundaryCalls = 0;
 auto Boundary = [&](const TArray<FInventoryStack>&) { ++BoundaryCalls; return true; };
 TestTrue(TEXT("Exact item limit checked"), FHyperManageRefundCapacity::CheckWithReader(Preview, Boundary) == EHyperManageRefundCapacity::Fits);
 ++Preview.Actors[0].Stacks[0].NumItems;
 TestTrue(TEXT("Over item limit skipped"), FHyperManageRefundCapacity::CheckWithReader(Preview, Boundary) == EHyperManageRefundCapacity::TooLarge);
 TestEqual(TEXT("Limit rejection bypasses callback"), BoundaryCalls, 1);
 TestTrue(TEXT("Missing player/world returns unavailable"), FHyperManageRefundCapacity::Check(nullptr, Preview, nullptr) == EHyperManageRefundCapacity::Unavailable);
 return true;
}
#endif
