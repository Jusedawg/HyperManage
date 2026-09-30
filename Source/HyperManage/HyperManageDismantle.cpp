#include "HyperManageDismantle.h"
#include "HyperManageSelection.h"
#include "HyperManageUndo.h"
#include "HyperManageUI.h"
#include "HyperManageDismantleReview.h"
#include "HyperManageRefundCapacity.h"
#include "FGCharacterPlayer.h"
#include "FGInventoryComponent.h"
#include "Equipment/FGBuildGun.h"
#include "Engine/World.h"
#include "FGBuildableBeam.h"
#include "FGBuildablePillar.h"
#include "Buildables/FGBuildableWall.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Equipment/FGBuildGunDismantle.h"
#include "UObject/StructOnScope.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

bool UHyperManageDismantle::ValidateCandidates(UWorld* World, const TArray<AActor*>& Input, AActor* Target, TArray<AActor*>& Output, FString& Error)
{
 Output.Reset(); Error.Reset();
 auto Fail = [&](const TCHAR* Message) { Output.Reset(); Error = Message; return false; };
 if (!IsValid(World) || World->GetNetMode() != NM_Standalone) return Fail(TEXT("Dismantling is currently single-player only."));
 TSet<AActor*> Seen;
 for (auto* Actor : Input) {
  if (!IsValid(Actor) || Actor->GetWorld() != World || Actor == Target) return Fail(TEXT("A building is unavailable or is the protected target. Reselect and retry."));
  if (Seen.Contains(Actor)) continue;
  Seen.Add(Actor);
  auto* Building = Cast<AFGBuildable>(Actor);
  const bool Structural = Actor->IsA<AFGBuildableBeam>() || Actor->IsA<AFGBuildablePillar>() || Actor->IsA<AFGBuildableWall>() || Actor->IsA<AFGBuildableFoundation>();
  const FString Package = Actor->GetClass()->GetOutermost()->GetName();
  if (!Building || !Structural || (!Package.StartsWith(TEXT("/Game/FactoryGame/")) && Package != TEXT("/Script/FactoryGame")))
   return Fail(TEXT("This first dismantle version supports native vanilla beams, pillars, walls and foundations only. Lightweight pieces, machines and modded buildings are not supported yet. Nothing was removed."));
  if (Building->GetIsLightweightTemporary() || Building->GetIsDismantled() || Building->GetIsPendingDismantleRemoval() || Building->IsAboutToBeDismantled())
   return Fail(TEXT("A building is temporary or already being dismantled. Reselect and retry."));
  TArray<UFGInventoryComponent*> Inventories; Building->GetComponents(Inventories);
  if (!Inventories.IsEmpty()) return Fail(TEXT("Buildings with inventory components are not supported by this first dismantle version."));
  Output.Add(Actor);
  if (Output.Num() > MaxBuildings) return Fail(TEXT("Dismantle at most 50 supported buildings at a time."));
 }
 if (Output.IsEmpty()) return Fail(TEXT("Select supported buildings first. The target is excluded."));
 return true;
}

UFunction* UHyperManageDismantle::FindNativeDispatch(UObject* State)
{
 if (!IsValid(State) || !State->IsA<UFGBuildGunStateDismantle>()) return nullptr;
 auto* Function = State->FindFunction(TEXT("Server_DismantleActors"));
 if (!Function || Function->NumParms != 2 || !Function->HasAllFunctionFlags(FUNC_Native | FUNC_NetServer)) return nullptr;
 auto* Actors = FindFProperty<FArrayProperty>(Function, TEXT("selectedActors"));
 auto* Bundles = FindFProperty<FArrayProperty>(Function, TEXT("lightweightBundles"));
 auto* ActorType = Actors ? CastField<FObjectPropertyBase>(Actors->Inner) : nullptr;
 auto* BundleType = Bundles ? CastField<FStructProperty>(Bundles->Inner) : nullptr;
 if (!ActorType || ActorType->PropertyClass != AActor::StaticClass() || !BundleType || BundleType->Struct != FDismantleLightweightBundle::StaticStruct()) return nullptr;
 return Function;
}

bool UHyperManageDismantle::MatchesSnapshot(const TArray<AActor*>& Actors, const TMap<TWeakObjectPtr<AActor>, FTransform>& Snapshot)
{
 if (Actors.IsEmpty() || Actors.Num() != Snapshot.Num()) return false;
 TSet<AActor*> Seen;
 for (auto* Actor : Actors) {
  if (!IsValid(Actor) || Seen.Contains(Actor)) return false;
  Seen.Add(Actor);
  const auto* Transform = Snapshot.Find(Actor);
  if (!Transform || !Actor->GetActorTransform().Equals(*Transform, 0.01)) return false;
 }
 return true;
}

bool UHyperManageDismantle::Preflight(TArray<AActor*>& Actors, FString& Error)
{
 if (!System || !System->Selection || System->Selection->HasPendingOperations()) { Error = TEXT("Wait for building edits to finish, then retry."); return false; }
 TArray<AActor*> Input; System->Selection->SelectedActorsNoTarget(Input);
 auto* World = System->GetWorld();
 if (!ValidateCandidates(World, Input, System->Selection->TargetActor, Actors, Error)) return false;
 auto* Controller = System->GetLocalController();
 auto* Player = Controller ? Controller->GetPlayerState<AFGPlayerState>() : nullptr;
 auto* Character = Controller ? Cast<AFGCharacterPlayer>(Controller->GetPawn()) : nullptr;
 if (!IsValid(Player) || !IsValid(Character) || Character->GetPlayerState() != Player || Character->GetWorld() != World) {
  Error = TEXT("The local player is unavailable."); return false;
 }
 for (auto* Actor : Actors) if (FVector::DistSquared(Actor->GetActorLocation(), Character->GetActorLocation()) > FMath::Square(2000.0)) {
  Error = TEXT("For this first version, every building must be within 20 meters of the player."); return false;
 }
 const auto Review = FHyperManageDismantleReviewer::Build(World, Actors, System->Selection->TargetActor, Player);
 if (!Review.Error.IsEmpty()) { Error = Review.Error; return false; }
 if (Review.AddedChildren != 0 || Review.Refunds.Actors.Num() != Actors.Num()) {
  Error = TEXT("Select all related children explicitly before dismantling. This first version never expands a destructive selection."); return false;
 }
 if (Review.NativeBlocked || Review.NativeWarnings) { Error = TEXT("The game reports a dismantle refusal or warning. Resolve it before retrying; Refund review shows details."); return false; }
 const auto Capacity = FHyperManageRefundCapacity::Check(World, Review.Refunds, Player);
 if (Capacity != EHyperManageRefundCapacity::Fits && Capacity != EHyperManageRefundCapacity::NoRefund) {
  Error = TEXT("The complete refund must fit in your inventory. Free space or select fewer buildings; overflow handling is not enabled yet."); return false;
 }
 return true;
}

void UHyperManageDismantle::Request()
{
 if (AwaitingConfirmation || !System || !System->UI) return;
 FString Error; TArray<AActor*> Actors;
 if (!Preflight(Actors, Error)) { System->UI->ShowPopup(TEXT("Cannot dismantle selection"), Error); return; }
 Pending.Reset(); for (auto* Actor : Actors) Pending.Add(Actor, Actor->GetActorTransform());
 PendingTarget = System->Selection->TargetActor;
 PendingPlayer = System->GetLocalController()->GetPlayerState<AFGPlayerState>();
 PendingNoBuildCost = PendingPlayer->GetPlayerRules().NoBuildCost;
 PendingAt = System->GetWorld()->GetRealTimeSeconds(); AwaitingConfirmation = true;
 System->UI->ShowConfirm(TEXT("Dismantle selected buildings?"), FString::Printf(TEXT("Permanently dismantle %d supported buildings?\n\nThe target is excluded. The game handles removal and refunds. This cannot be undone, and HyperManage edit history will be cleared.\n\nOnly native structural buildings within 20 m are supported. Inventory must fit all refunds. Confirmation expires after 60 seconds."), Actors.Num()), this, TEXT("Confirm"));
}

void UHyperManageDismantle::Confirm(bool Accepted)
{
 if (!AwaitingConfirmation) return;
 AwaitingConfirmation = false;
 const auto Snapshot = MoveTemp(Pending); Pending.Reset();
 if (!Accepted || !System || !System->UI) return;
 FString Error; TArray<AActor*> Actors;
 auto Fail = [&](const FString& Message) { System->UI->ShowPopup(TEXT("Dismantle cancelled"), Message); };
 if (!Preflight(Actors, Error)) { Fail(Error); return; }
 auto* Controller = System->GetLocalController();
 auto* Player = Controller->GetPlayerState<AFGPlayerState>();
 if (System->GetWorld()->GetRealTimeSeconds() - PendingAt > 60 || !PendingPlayer.IsValid() || Player != PendingPlayer.Get()
  || Player->GetPlayerRules().NoBuildCost != PendingNoBuildCost || PendingTarget.IsStale() || PendingTarget.Get() != System->Selection->TargetActor || Actors.Num() != Snapshot.Num()) {
  Fail(TEXT("The confirmation expired or its selection, target or player rules changed. Start again.")); return;
 }
 if (!MatchesSnapshot(Actors, Snapshot)) { Fail(TEXT("The confirmed buildings changed. Start again.")); return; }
 auto* Character = Cast<AFGCharacterPlayer>(Controller->GetPawn());
 auto* Gun = Character ? Character->GetBuildGun() : nullptr;
 UFGBuildGunStateDismantle* State = nullptr;
 if (IsValid(Gun)) ForEachObjectWithOuter(Gun, [&](UObject* Object) { if (auto* Candidate = Cast<UFGBuildGunStateDismantle>(Object)) State = Candidate; }, false);
 auto* Function = FindNativeDispatch(State);
 if (!Function || !State || State->GetWorld() != System->GetWorld()) { Fail(TEXT("The game's dismantle handler is unavailable. Equip the build gun once, then retry.")); return; }
 // Use the game's own transaction. Never grant a separate refund or call DestroyActor here.
 FStructOnScope Parameters(Function);
 auto* ActorArray = FindFProperty<FArrayProperty>(Function, TEXT("selectedActors"));
 ActorArray->CopyCompleteValue(ActorArray->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), &Actors);
 System->Selection->SelectClear(true);
 if (System->Undo) System->Undo->ClearUndoStack();
 State->ProcessEvent(Function, Parameters.GetStructMemory());
 System->UI->ShowPopup(TEXT("Dismantle request sent"), TEXT("The game received the confirmed selection. Check the buildings and your inventory. Individual removals may be refused by the game; this operation cannot be undone."));
}
