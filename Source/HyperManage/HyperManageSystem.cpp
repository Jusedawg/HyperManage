#include "HyperManageSystem.h"
#include "HyperManageUndo.h"
#include "HyperManageConfig.h"
#include "HyperManageSelection.h"
#include "HyperManageTransform.h"
#include "HyperManageUI.h"
#include "HyperManageAction.h"
#include "HyperManageInput.h"
#include "HyperManageRCO.h"
#include "Engine/GameEngine.h"
#include "Engine/GameInstance.h"

UHyperManageSystem* UHyperManageSystem::HyperManageSystemSingleton = nullptr;

namespace
{
	TMap<TWeakObjectPtr<UWorld>, TWeakObjectPtr<UHyperManageSystem>> WorldSystems;
}

UHyperManageSystem* UHyperManageSystem::Get()
{
	if (!GEngine) return nullptr;
	for (const FWorldContext& Context : GEngine->GetWorldContexts()) {
		if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE) {
			UWorld* World = Context.World();
			if (World && World->GetFirstLocalPlayerFromController()) return GetForWorld(World);
		}
	}
	return nullptr;
}

UHyperManageSystem* UHyperManageSystem::GetForWorld(UWorld* World)
{
	if (!IsValid(World) || !World->GetGameInstance()) return nullptr;
	if (auto* Existing = WorldSystems.Find(World)) {
		if (Existing->IsValid()) return Existing->Get();
	}
	auto* System = NewObject<UHyperManageSystem>(World->GetGameInstance());
	WorldSystems.Add(World, System);
	HyperManageSystemSingleton = System;
	System->SystemId = FGuid::NewGuid();
	System->Initialize(World->GetGameInstance(), World);
	return System;
}

void UHyperManageSystem::ReleaseWorld(UWorld* World)
{
	TWeakObjectPtr<UHyperManageSystem> Existing;
	if (!WorldSystems.RemoveAndCopyValue(World, Existing) || !Existing.IsValid()) return;
	if (auto* GameInstance = World->GetGameInstance()) GameInstance->UnregisterReferencedObject(Existing.Get());
	if (HyperManageSystemSingleton == Existing.Get()) HyperManageSystemSingleton = nullptr;
}
void UHyperManageSystem::Initialize(UGameInstance* GameInstance, UWorld* World)
{
	// force persistence of UHyperManageSystem object
	GameInstance->RegisterReferencedObject(this);
	CurrentWorld = World;

	// Init all components
	Transform = InitComponent<UHyperManageTransform>(this);
	Selection = InitComponent<UHyperManageSelection>(this);
	Undo = InitComponent<UHyperManageUndo>(this);
	Config = InitComponent<UHyperManageConfiguration>(this);
	UI = InitComponent<UHyperManageUI>(this);
	Action = InitComponent<UHyperManageAction>(this);
	Input = InitComponent<UHyperManageInput>(this);
}

UHyperManageRCO* UHyperManageSystem::GetMMRCO()
{
	if (!MMRCO) {
		MMRCO = Cast<UHyperManageRCO>(GetLocalController()->GetRemoteCallObjectOfClass(UHyperManageRCO::StaticClass()));
	}
	return MMRCO;
}

AHyperManageEquip* UHyperManageSystem::GetEquip()
{
	for (AHyperManageEquip* Equip : ActiveEquipment) {
		if (IsValid(Equip) && Equip->IsLocal) {
			return Equip;
		}
	}
	return ActiveEquipment.IsEmpty() ? nullptr : ActiveEquipment[0];
}

void UHyperManageSystem::AddActiveEquipment(AHyperManageEquip* Equip)
{
	if (!ActiveEquipment.Contains(Equip)) {
		ActiveEquipment.Emplace(Equip);
	}
}

void UHyperManageSystem::RemoveActiveEquipment(AHyperManageEquip* Equip)
{
	if (ActiveEquipment.Contains(Equip)) {
		ActiveEquipment.Remove(Equip);
	}
}

UWorld* UHyperManageSystem::GetWorld() const
{
	return CurrentWorld;
}

AFGPlayerController* UHyperManageSystem::GetLocalController()
{
	if (!IsValid(LocalController)) {
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator) {
			if (Iterator->Get()->IsLocalPlayerController()) {
				LocalController = Cast<AFGPlayerController>(Iterator->Get());
				break;
			}
		}
	}
	return LocalController;
}

bool UHyperManageSystem::IsIdMatch(const FGuid& CheckId)
{
	return ((CheckId.A == SystemId.A) && (CheckId.B == SystemId.B) && (CheckId.C == SystemId.C) && (CheckId.D == SystemId.D));
}

FVector UHyperManageSystem::GetCameraViewVector()
{
	FVector CameraVector = GetLocalController()->PlayerCameraManager->GetCameraRotation().Vector();
	return FVector(CameraVector.X, CameraVector.Y, 0.f).GetSafeNormal(); // flatten and normalize
}

void UHyperManageSystem::BasicTransform(EActionNameIdx ActionIndex)
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	FVector Scale = FVector::OneVector;
	FHyperManageIncrement IncSetting = Config->MMConfig.IncrementSettings[Config->CurrentIncrementSize()];
	switch (ActionIndex) {
		case EActionNameIdx::SpinLeft:
			Rotation.Yaw = -IncSetting.DegreesToRotate; // -Rotate to Z
			break;
		case EActionNameIdx::SpinRight:
			Rotation.Yaw = IncSetting.DegreesToRotate; // Rotate to Z
			break;
		case EActionNameIdx::MoveUp:
			Location.Z = IncSetting.CentimetersToMove; // Move to Z
			break;
		case EActionNameIdx::MoveDown:
			Location.Z = -IncSetting.CentimetersToMove; // -Move to Z
			break;

		case EActionNameIdx::MoveLeft:
			Location.Y = -IncSetting.CentimetersToMove; // -Move to Y
			break;
		case EActionNameIdx::MoveRight:
			Location.Y = IncSetting.CentimetersToMove; // Move to Y
			break;
		case EActionNameIdx::MoveAway:
			Location.X = IncSetting.CentimetersToMove; // Move to X
			break;
		case EActionNameIdx::MoveToward:
			Location.X = -IncSetting.CentimetersToMove; // -Move to X
			break;

		case EActionNameIdx::RollLeft:
			Rotation.Roll = IncSetting.DegreesToRotate; // Rotate to X
			break;
		case EActionNameIdx::RollRight:
			Rotation.Roll = -IncSetting.DegreesToRotate; // -Rotate to X
			break;
		case EActionNameIdx::PitchAway:
			Rotation.Pitch = IncSetting.DegreesToRotate; // Rotate to Y
			break;
		case EActionNameIdx::PitchToward:
			Rotation.Pitch = -IncSetting.DegreesToRotate; // -Rotate to Y
			break;

		case EActionNameIdx::Shrink:
			Scale = FVector(1.f / ((IncSetting.PercentToGrow / 100.f) + 1.f));
			break;
		case EActionNameIdx::Grow:
			Scale = FVector(((IncSetting.PercentToGrow / 100.f) + 1.f));
			break;
	}
	Action->PrepareTransform(Location, Rotation, Scale);
}

void UHyperManageSystem::ExecuteAction(EActionNameIdx ActionIndex)
{
	if (Selection->HasPendingOperations()) return;
	if ((ActionIndex >= EActionNameIdx::SpinLeft) && (ActionIndex <= EActionNameIdx::Grow)) {
		BasicTransform(ActionIndex);
		return;
	}

	switch (ActionIndex) {
		case EActionNameIdx::NoAction:
			break;
		case EActionNameIdx::SelectTarget:
		case EActionNameIdx::DeselectTarget:
			Action->SelectActor(Selection->LineTraceFromPlayer(), ActionIndex == EActionNameIdx::SelectTarget);
			break;
		case EActionNameIdx::Undo:
			Action->PerformUndo();
			break;
		case EActionNameIdx::Redo:
			Action->PerformRedo();
			break;
		case EActionNameIdx::ChangeIncSize:
			Config->NextIncrementSize();
			Config->SaveHyperManageConfig();
			break;
		case EActionNameIdx::KnowNotes:
			UI->NextMMWidget();
			break;
		case EActionNameIdx::ShowTools:
			UI->ShowToolsUI();
			break;
		case EActionNameIdx::SetAnchor:
			if (Selection->SetMarkerWithHistory(Selection->LineTraceFromPlayer(), true)) {
				Action->MakeActorMovable(Selection->AnchorActor);
			}
			break;
		case EActionNameIdx::SetTarget:
			if (Selection->SetMarkerWithHistory(Selection->LineTraceFromPlayer(), false)) {
				Action->MakeActorMovable(Selection->TargetActor);
			}
			break;

		case EActionNameIdx::AlignLeft:
			UI->ComingSoon();
			break;
		case EActionNameIdx::AlignLRCenter:
			UI->ComingSoon();
			break;
		case EActionNameIdx::AlignRight:
			UI->ComingSoon();
			break;
		case EActionNameIdx::SpaceLR:
			UI->ComingSoon();
			break;
		case EActionNameIdx::StackLR:
			UI->ComingSoon();
			break;
		case EActionNameIdx::LockScaleLR:
			Config->MMConfig.IsScaleLockedLR = !Config->MMConfig.IsScaleLockedLR;
			Config->SaveHyperManageConfig();
			break;

		case EActionNameIdx::AlignTop:
			UI->ComingSoon();
			break;
		case EActionNameIdx::AlignTBCenter:
			UI->ComingSoon();
			break;
		case EActionNameIdx::AlignBottom:
			UI->ComingSoon();
			break;
		case EActionNameIdx::SpaceTB:
			UI->ComingSoon();
			break;
		case EActionNameIdx::StackTB:
			UI->ComingSoon();
			break;
		case EActionNameIdx::LockScaleTB:
			Config->MMConfig.IsScaleLockedTB = !Config->MMConfig.IsScaleLockedTB;
			Config->SaveHyperManageConfig();
			break;

		case EActionNameIdx::AlignFront:
			UI->ComingSoon();
			break;
		case EActionNameIdx::AlignFBCenter:
			UI->ComingSoon();
			break;
		case EActionNameIdx::AlignBack:
			UI->ComingSoon();
			break;
		case EActionNameIdx::SpaceFB:
			UI->ComingSoon();
			break;
		case EActionNameIdx::StackFB:
			UI->ComingSoon();
			break;
		case EActionNameIdx::LockScaleFB:
			Config->MMConfig.IsScaleLockedFB = !Config->MMConfig.IsScaleLockedFB;
			Config->SaveHyperManageConfig();
			break;

		case EActionNameIdx::SameRotation:
			Action->MoveSelectionToTarget(true);
			break;
		case EActionNameIdx::SameScale:
			Action->SetSameScale();
			break;
		case EActionNameIdx::SamePaint:
			Action->SetSamePaint();
			break;

		case EActionNameIdx::Connect:
			if (Cast<AHyperManageLightweightProxy>(Selection->AnchorActor) || Cast<AHyperManageLightweightProxy>(Selection->TargetActor)) break;
			GetMMRCO()->ServerHandleConnect(SystemId, true, Selection->AnchorActor, Selection->TargetActor);
			break;
		case EActionNameIdx::Disconnect:
			if (Cast<AHyperManageLightweightProxy>(Selection->AnchorActor) || Cast<AHyperManageLightweightProxy>(Selection->TargetActor)) break;
			GetMMRCO()->ServerHandleConnect(SystemId, false, Selection->AnchorActor, Selection->TargetActor);
			break;

		case EActionNameIdx::SelectBoxSides:
		case EActionNameIdx::SelectBoxPivot:
			Selection->AddAnchorTargetBoxToSelection(ActionIndex == EActionNameIdx::SelectBoxSides);
			break;
		case EActionNameIdx::MoveSelection:
			Action->PrepareMove();
			break;
		case EActionNameIdx::CopySelection:
			UI->ComingSoon();
			break;
		case EActionNameIdx::NewSelection:
			UI->ShowConfirm(TITLE_START_NEW_SELECTION, BODY_START_NEW_SELECTION, Selection, "SelectClear");
			break;
		case EActionNameIdx::DeleteSelection:
			UI->ComingSoon();
			break;
		case EActionNameIdx::SaveSelection:
			Selection->SaveSelection();
			break;
		case EActionNameIdx::LoadSelection:
			Selection->LoadSelection();
			break;

		case EActionNameIdx::ClearUndo:
			Undo->ClearUndoStack();
			break;
		case EActionNameIdx::IsGrouped:
			Config->MMConfig.IsGrouped = !Config->MMConfig.IsGrouped;
			Config->SaveHyperManageConfig();
			break;
		case EActionNameIdx::IsViewBased:
			Config->MMConfig.IsViewBased = !Config->MMConfig.IsViewBased;
			Config->SaveHyperManageConfig();
			break;
		case EActionNameIdx::NextHologram:
			Selection->SelectNextMaterial();
			Config->SaveHyperManageConfig();
			break;
		case EActionNameIdx::SnapWorldXY:
		case EActionNameIdx::SnapWorldRotation:
		case EActionNameIdx::LevelWorldRotation:
			Action->AlignToWorld(ActionIndex);
			break;
		case EActionNameIdx::Settings:
			UI->ComingSoon();
			break;
	}
}
