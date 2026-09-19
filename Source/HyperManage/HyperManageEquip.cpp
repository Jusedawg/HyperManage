#include "HyperManageEquip.h"
#include "HyperManageRCO.h"
#include "HyperManageSelection.h"
#include "HyperManageSystem.h"
#include "HyperManageUI.h"
#include "HyperManageInput.h"
#include "Components/SceneComponent.h"

AHyperManageEquip::AHyperManageEquip()
{
	// AFGEquipment::Equip attaches and repositions the root; native equipment must provide one.
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("EquipmentRoot")));
	bOnlyRelevantToOwner = false;
	bNetUseOwnerRelevancy = false;
	bAlwaysRelevant = true;
	bReplicates = true;

	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//mAttachmentClass = AHyperManageAttachment::StaticClass();
	mEquipmentSlot = EEquipmentSlot::ES_ARMS;
	mAttachSocket = TEXT("hand_rSocket");
}

void AHyperManageEquip::BeginPlay()
{
	Super::BeginPlay();

	SetupHyperManageSystem();
	System->AddActiveEquipment(this);
}

void AHyperManageEquip::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(System)) {
		if (IsLocal && ManagerEquipped) {
			System->Input->Detach();
			System->UI->HideMMWidget();
			ManagerEquipped = false;
		}
		System->RemoveActiveEquipment(this);
	}
	Super::EndPlay(EndPlayReason);
}
void AHyperManageEquip::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHyperManageEquip::SetupHyperManageSystem()
{
	if (!System) {
		System = UHyperManageSystem::GetForWorld(GetWorld());
	}
}

// Equip/UnEquip functions ------------------------------------------------------------------------

void AHyperManageEquip::Equip(AFGCharacterPlayer* Character)
{
	Super::Equip(Character);
	IsLocal = Character->IsLocallyControlled();
	SetupHyperManageSystem();
	System->AddActiveEquipment(this);
	if (IsLocal) {
		ManagerEquipped = true;
		System->UI->ShowMMWidget();
		System->Input->Attach(this);
	}
}

void AHyperManageEquip::MulticastUnEquip_Implementation()
{
	SetupHyperManageSystem();
	System->RemoveActiveEquipment(this);
	if (IsLocal && ManagerEquipped) {
		System->Input->Detach();
		System->UI->HideMMWidget();
		System->Selection->SelectClear();
		ManagerEquipped = false;
	}
}

void AHyperManageEquip::UnEquip()
{
	Super::UnEquip();
	// UnEquip and ShouldSaveState are sent only to server.  multicast to allow clients to locally process their own unequip.
	if (HasAuthority()) {
		MulticastUnEquip();
	}
}

// Multicast action functions ---------------------------------------------------------------------

void AHyperManageEquip::MulticastShowPopup_Implementation(const FGuid& Id, const FString& Title, const FString& Body)
{
	if (System->IsIdMatch(Id)) {
		System->UI->ShowPopup(Title, Body);
	}
}

void AHyperManageEquip::MulticastTransformActors_Implementation(const TArray<AActor*>& Actors, FHyperManageTransformData TransformData)
{
	System->Transform->ProcessTransform(Actors, TransformData);
}

void AHyperManageEquip::MulticastUndoTransforms_Implementation(const FUndoInfo& UndoInfo)
{
	for (const auto& UndoComponent : UndoInfo.TransformComponents) {
		if (IsValid(UndoComponent.Component)) {
			System->Transform->TransformComponent(UndoComponent.Component, UndoComponent.Transform);
		}
	}
	for (const auto& UndoActor : UndoInfo.TransformActors) {
		if (IsValid(UndoActor.Actor)) {
			System->Transform->TransformActor(UndoActor.Actor, UndoActor.Transform);
		}
	}
}

void AHyperManageEquip::MulticastRefreshMaterials_Implementation(const TArray<AActor*>& Actors)
{
	System->Selection->RefreshMaterials(Actors);
}
