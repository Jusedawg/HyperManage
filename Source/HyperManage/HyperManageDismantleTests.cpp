#include "HyperManageDismantlePlan.h"
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
#endif
