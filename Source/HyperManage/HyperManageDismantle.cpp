#include "HyperManageDismantle.h"
#include "HyperManageSelection.h"
#include "HyperManageUndo.h"
#include "HyperManageUI.h"
#include "HyperManageDismantleReview.h"
#include "HyperManageRefundCapacity.h"
#include "FGCharacterPlayer.h"
#include "FGInventoryComponent.h"
#include "FGFactoryConnectionComponent.h"
#include "FGDismantleInterface.h"
#include "Buildables/FGBuildableStorage.h"
#include "Equipment/FGBuildGun.h"
#include "Engine/World.h"
#include "FGBuildableBeam.h"
#include "FGBuildablePillar.h"
#include "Buildables/FGBuildableWall.h"
#include "Buildables/FGBuildableFoundation.h"
#include "Buildables/FGBuildableWalkway.h"
#include "Buildables/FGBuildableStair.h"
#include "Buildables/FGBuildableLadder.h"
#include "Equipment/FGBuildGunDismantle.h"
#include "UObject/StructOnScope.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

bool UHyperManageDismantle::IsSupportedStorageClass(const UClass* Class)
{
 if (!IsValid(Class) || !Class->IsChildOf(AFGBuildableStorage::StaticClass())) return false;
 const FString Path = Class->GetPathName();
 return Path == TEXT("/Game/FactoryGame/Buildable/Factory/StorageContainerMk1/Build_StorageContainerMk1.Build_StorageContainerMk1_C")
  || Path == TEXT("/Game/FactoryGame/Buildable/Factory/StorageContainerMk2/Build_StorageContainerMk2.Build_StorageContainerMk2_C");
}

bool UHyperManageDismantle::RefundCoversContents(const TArray<FInventoryStack>& Contents, const TArray<FInventoryStack>& Refund)
{
 if (Contents.Num() > FHyperManageRefundCapacity::MaxCheckStacks || Refund.Num() > FHyperManageRefundCapacity::MaxCheckStacks) return false;
 auto Valid = [](const FInventoryStack& Stack) { return Stack.NumItems >= 0 && (Stack.NumItems == 0 || IsValid(Stack.Item.GetItemClass().Get())); };
 TArray<int32> Remaining;
 for (const auto& Stack : Refund) { if (!Valid(Stack)) return false; Remaining.Add(Stack.NumItems); }
 for (const auto& Stack : Contents) {
  if (!Valid(Stack)) return false;
  int32 Needed = Stack.NumItems;
  for (int32 Index = 0; Index < Refund.Num() && Needed > 0; ++Index) {
   const auto& Item = Refund[Index].Item;
   if (Remaining[Index] == 0 || Item.GetItemClass() != Stack.Item.GetItemClass() || Item.LegacyItemStateActor != Stack.Item.LegacyItemStateActor
    || Item.HasState() != Stack.Item.HasState() || (Item.HasState() && !Item.GetItemState().Identical(Stack.Item.GetItemState()))) continue;
   const int32 Taken = FMath::Min(Needed, Remaining[Index]); Needed -= Taken; Remaining[Index] -= Taken;
  }
  if (Needed > 0) return false;
 }
 return true;
}

bool UHyperManageDismantle::ValidateCandidates(UWorld* World, const TArray<AActor*>& Input, AActor* Target, TArray<AActor*>& Output, FString& Error)
{
 auto* Subsystem = IsValid(World) ? AFGLightweightBuildableSubsystem::Get(World) : nullptr;
 return ValidateWithResolver(World, Input, Target, Output, Error, [Subsystem](const FHyperManageLightweightRef& Ref) {
  return IsValid(Subsystem) ? Subsystem->GetRuntimeDataForBuildableClassAndIndex(Ref.BuildableClass, Ref.Index) : nullptr;
 });
}

bool UHyperManageDismantle::ValidateWithResolver(UWorld* World, const TArray<AActor*>& Input, AActor* Target, TArray<AActor*>& Output, FString& Error, FResolveInstance Resolve)
{
 Output.Reset(); Error.Reset();
 auto Fail = [&](const TCHAR* Message) { Output.Reset(); Error = Message; return false; };
 if (!IsValid(World) || World->GetNetMode() != NM_Standalone) return Fail(TEXT("Dismantling is currently single-player only."));
 TSet<AActor*> Seen;
 for (auto* Actor : Input) {
  if (!IsValid(Actor) || Actor->GetWorld() != World || Actor == Target) return Fail(TEXT("A building is unavailable or is the protected target. Reselect and retry."));
  if (Seen.Contains(Actor)) continue;
  Seen.Add(Actor);
  auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor);
  auto* Building = Proxy ? Proxy->Ref.BuildableClass.GetDefaultObject() : Cast<AFGBuildable>(Actor);
  const bool Structural = Building && (Building->IsA<AFGBuildableBeam>() || Building->IsA<AFGBuildablePillar>()
   || Building->IsA<AFGBuildableWall>() || Building->IsA<AFGBuildableFoundation>()
   || Building->IsA<AFGBuildableWalkway>() || Building->IsA<AFGBuildableStair>() || Building->IsA<AFGBuildableLadder>());
  const bool Storage = !Proxy && Building && IsSupportedStorageClass(Building->GetClass());
  const FString Package = Building ? Building->GetClass()->GetOutermost()->GetName() : FString();
  if ((!Structural && !Storage) || (!Package.StartsWith(TEXT("/Game/FactoryGame/")) && Package != TEXT("/Script/FactoryGame"))) {
   Output.Reset();
   const FString Name = Building && !Building->mDisplayName.IsEmpty() ? Building->mDisplayName.ToString() : Actor->GetName();
   Error = FString::Printf(TEXT("%s is not supported yet. Select vanilla foundations, ramps, walls, beams, pillars, walkways, stairs or ladders. Disconnected Storage Containers and Industrial Storage Containers are also supported. Machines, special storage and modded buildings will come later. Nothing was removed."), *Name);
   return false;
  }
  if (Proxy) {
   if (!Proxy->Available || Proxy->IsPending() || !Proxy->Ref.SelectionId.IsValid() || Proxy->Ref.Index < 0 || !Proxy->Ref.Matches(Resolve(Proxy->Ref)))
    return Fail(TEXT("A selected piece changed or is unavailable. Reselect it and retry. Nothing was removed."));
   if (const auto* TargetProxy = Cast<AHyperManageLightweightProxy>(Target); IsValid(TargetProxy)
    && TargetProxy->Ref.BuildableClass == Proxy->Ref.BuildableClass && TargetProxy->Ref.Index == Proxy->Ref.Index)
    return Fail(TEXT("A selected piece is the protected target. Nothing was removed."));
   for (auto* Existing : Output) if (const auto* Other = Cast<AHyperManageLightweightProxy>(Existing);
    Other && Other->Ref.BuildableClass == Proxy->Ref.BuildableClass && Other->Ref.Index == Proxy->Ref.Index)
    return Fail(TEXT("The selection contains duplicate handles for one building. Clear the selection and reselect it."));
  }
  else if (Building->GetIsLightweightTemporary() || Building->GetIsDismantled() || Building->GetIsPendingDismantleRemoval() || Building->IsAboutToBeDismantled())
   return Fail(TEXT("A building is temporary or already being dismantled. Reselect and retry."));
  TArray<UFGInventoryComponent*> Inventories; Building->GetComponents(Inventories);
  if (Storage) {
   auto* Inventory = CastChecked<AFGBuildableStorage>(Building)->GetStorageInventory();
   if (!IsValid(Inventory) || Inventory->GetOwner() != Building || Inventory->GetWorld() != World || !Inventories.Contains(Inventory))
    return Fail(TEXT("The container inventory is unavailable. Nothing was removed."));
   for (auto* Other : Inventories) if (!IsValid(Other) || (Other != Inventory && !Other->IsEmpty()))
    return Fail(TEXT("The container has additional inventory contents that are not supported yet. Nothing was removed."));
   TArray<UFGFactoryConnectionComponent*> Connections; Building->GetComponents(Connections);
   for (auto* Connection : Connections) if (Connection->IsConnected())
    return Fail(TEXT("Disconnect the container's belts before dismantling it. This prevents incoming items from changing the refund during confirmation."));
  }
  else if (!Inventories.IsEmpty()) return Fail(TEXT("This building's inventory is not supported yet. Nothing was removed."));
  Output.Add(Actor);
  if (Output.Num() > MaxBuildings) return Fail(TEXT("Dismantle at most 50 supported buildings at a time."));
 }
 if (Output.IsEmpty()) return Fail(TEXT("Select supported buildings first. The target is excluded."));
 return true;
}

void UHyperManageDismantle::MakeDispatch(const TArray<AActor*>& Selection, TArray<AActor*>& Actors, TArray<FDismantleLightweightBundle>& Bundles)
{
 Actors.Reset(); Bundles.Reset();
 for (auto* Actor : Selection) {
  if (const auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
   auto* Bundle = Bundles.FindByPredicate([&](const FDismantleLightweightBundle& Entry) { return Entry.BuildableClass == Proxy->Ref.BuildableClass; });
   if (!Bundle) Bundle = &Bundles.Add_GetRef(FDismantleLightweightBundle(Proxy->Ref.BuildableClass));
   Bundle->RemovalIndices.AddUnique(Proxy->Ref.Index);
  }
  else Actors.AddUnique(Actor);
 }
}

bool UHyperManageDismantle::MatchesInstance(const FConfirmedInstance& Snapshot, const FHyperManageLightweightRef& Ref, const FRuntimeBuildableInstanceData* Data)
{
 return Snapshot.Ref.SelectionId == Ref.SelectionId && Snapshot.Ref.BuildableClass == Ref.BuildableClass && Snapshot.Ref.Index == Ref.Index
  && Snapshot.Ref.Recipe == Ref.Recipe && Snapshot.Ref.ExpectedTransform.Equals(Ref.ExpectedTransform, 0.01)
  && Ref.Matches(Data) && Snapshot.Handles == Data->Handles;
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
 if (Review.AddedChildren != 0 || Review.Refunds.Actors.Num() + Review.Refunds.Instances.Num() != Actors.Num()) {
  Error = TEXT("Select all related children explicitly before dismantling. This first version never expands a destructive selection."); return false;
 }
 if (Review.NativeBlocked || Review.NativeWarnings) { Error = TEXT("The game reports a dismantle refusal or warning. Resolve it before retrying; Refund review shows details."); return false; }
 for (const auto& Entry : Review.Refunds.Actors) if (auto* Storage = Cast<AFGBuildableStorage>(Entry.Actor.Get())) {
  auto* Inventory = Storage->GetStorageInventory();
  if (!IsValid(Inventory)) { Error = TEXT("A container inventory became unavailable. Retry."); return false; }
  TArray<FInventoryStack> Contents, ContentRefund;
  Inventory->GetInventoryStacks(Contents);
  // No-build-cost asks the native contract for contents only, avoiding construction materials masking a missing stored stack.
  IFGDismantleInterface::Execute_GetDismantleRefund(Storage, ContentRefund, true);
  if (!RefundCoversContents(Contents, ContentRefund) || !RefundCoversContents(ContentRefund, Entry.Stacks)) {
   Error = TEXT("The container's stored items could not be verified in the game refund. Nothing was removed; empty the container and retry."); return false;
  }
 }
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
 Pending.Reset(); PendingInstances.Reset();
 auto* Subsystem = AFGLightweightBuildableSubsystem::Get(System->GetWorld());
 for (auto* Actor : Actors) {
  Pending.Add(Actor, Actor->GetActorTransform());
  if (const auto* Proxy = Cast<AHyperManageLightweightProxy>(Actor)) {
   const auto* Data = IsValid(Subsystem) ? Subsystem->GetRuntimeDataForBuildableClassAndIndex(Proxy->Ref.BuildableClass, Proxy->Ref.Index) : nullptr;
   if (!Proxy->Ref.Matches(Data)) { Pending.Reset(); PendingInstances.Reset(); System->UI->ShowPopup(TEXT("Cannot dismantle selection"), TEXT("A piece changed. Reselect and retry.")); return; }
   PendingInstances.Add(Actor, FConfirmedInstance{Proxy->Ref, Data->Handles});
  }
 }
 PendingTarget = System->Selection->TargetActor;
 PendingPlayer = System->GetLocalController()->GetPlayerState<AFGPlayerState>();
 PendingNoBuildCost = PendingPlayer->GetPlayerRules().NoBuildCost;
 PendingAt = System->GetWorld()->GetRealTimeSeconds(); AwaitingConfirmation = true;
 System->UI->ShowConfirm(TEXT("Dismantle selected buildings?"), FString::Printf(TEXT("Permanently dismantle %d supported buildings?\n\nThe target is excluded. The game handles removal and refunds. This cannot be undone, and HyperManage edit history will be cleared.\n\nSupports foundations, ramps, walls, beams, pillars, walkways, stairs and ladders within 20 m. Disconnected Storage Containers and Industrial Storage Containers are also supported, including their contents. Inventory must fit all refunds. Confirmation expires after 60 seconds."), Actors.Num()), this, TEXT("Confirm"));
}

void UHyperManageDismantle::Confirm(bool Accepted)
{
 if (!AwaitingConfirmation) return;
 AwaitingConfirmation = false;
 const auto Snapshot = MoveTemp(Pending); Pending.Reset();
 const auto Instances = MoveTemp(PendingInstances); PendingInstances.Reset();
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
 auto* Subsystem = AFGLightweightBuildableSubsystem::Get(System->GetWorld());
 for (const auto& Entry : Instances) {
  const auto* Proxy = Cast<AHyperManageLightweightProxy>(Entry.Key.Get());
  const auto* Data = Proxy && IsValid(Subsystem) ? Subsystem->GetRuntimeDataForBuildableClassAndIndex(Proxy->Ref.BuildableClass, Proxy->Ref.Index) : nullptr;
  if (!Proxy || !MatchesInstance(Entry.Value, Proxy->Ref, Data)) { Fail(TEXT("A confirmed piece changed or was replaced. Reselect and retry.")); return; }
 }
 auto* Character = Cast<AFGCharacterPlayer>(Controller->GetPawn());
 auto* Gun = Character ? Character->GetBuildGun() : nullptr;
 UFGBuildGunStateDismantle* State = nullptr;
 if (IsValid(Gun)) ForEachObjectWithOuter(Gun, [&](UObject* Object) { if (auto* Candidate = Cast<UFGBuildGunStateDismantle>(Object)) State = Candidate; }, false);
 auto* Function = FindNativeDispatch(State);
 if (!Function || !State || State->GetWorld() != System->GetWorld()) { Fail(TEXT("The game's dismantle handler is unavailable. Equip the build gun once, then retry.")); return; }
 // Use the game's own batch handler. Never grant a separate refund or call DestroyActor here.
 FStructOnScope Parameters(Function);
 auto* ActorArray = FindFProperty<FArrayProperty>(Function, TEXT("selectedActors"));
 TArray<AActor*> NativeActors; TArray<FDismantleLightweightBundle> Bundles;
 MakeDispatch(Actors, NativeActors, Bundles);
 ActorArray->CopyCompleteValue(ActorArray->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), &NativeActors);
 auto* BundleArray = FindFProperty<FArrayProperty>(Function, TEXT("lightweightBundles"));
 BundleArray->CopyCompleteValue(BundleArray->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), &Bundles);
 System->Selection->SelectClear(true);
 if (System->Undo) System->Undo->ClearUndoStack();
 State->ProcessEvent(Function, Parameters.GetStructMemory());
 System->UI->ShowPopup(TEXT("Dismantle request sent"), TEXT("The game received the confirmed selection. Check the buildings and your inventory. Individual removals may be refused by the game; this operation cannot be undone."));
}
