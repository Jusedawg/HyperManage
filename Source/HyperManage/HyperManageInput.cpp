#include "TimerManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "HyperManageCopyPreview.h"
#include "HyperManageInput.h"
#include "HyperManageEquip.h"
#include "FGInputLibrary.h"
#include "Components/InputComponent.h"

void UHyperManageInput::ClearAllKeyTimers()
{
	if (!IsValid(Equip)) { KeyTimerHandleMap.Empty(); return; }
	for (auto& KeyTimer : KeyTimerHandleMap) {
		Equip->GetWorldTimerManager().ClearTimer(KeyTimer.Value);
	}
	KeyTimerHandleMap.Empty();
}

void UHyperManageInput::SetupKeyBinding(FHyperManageKeyConfig KeyConfig, EInputEvent InputEvent)
{
	FInputChord Chord = FInputChord(KeyConfig.Key, KeyConfig.Shift, KeyConfig.Ctrl, KeyConfig.Alt, false);
	FInputKeyBinding KeyBinding = FInputKeyBinding(Chord, InputEvent);
	KeyBinding.bConsumeInput = true;
	KeyBinding.bExecuteWhenPaused = false;

	if ((KeyConfig.UseRepeats) && (InputEvent == EInputEvent::IE_Pressed) && ((KeyConfig.Key == EKeys::LeftMouseButton) ||
		(KeyConfig.Key == EKeys::MiddleMouseButton) || (KeyConfig.Key == EKeys::RightMouseButton))) {

		// use alternate delegate that handles simulated mouse button repeats by adding a timer/delegate when pressed
		KeyBinding.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda([=, this](const FKey& Key)
		{
			if (!KeyTimerHandleMap.Contains(KeyConfig.Key)) {
				FTimerDelegate TimerCallback;
				TimerCallback.BindLambda([=, this]
				{
					PerformIndexedAction(KeyConfig.Key, KeyConfig.ActionIndex, EInputEvent::IE_Repeat);
				});
				FTimerHandle TimerHandle;
				Equip->GetWorldTimerManager().SetTimer(TimerHandle, TimerCallback, 0.1, true, 0.5);
				KeyTimerHandleMap.Add(KeyConfig.Key, TimerHandle);
			}
			PerformIndexedAction(Key, KeyConfig.ActionIndex, InputEvent);
		});
	} else {
		KeyBinding.KeyDelegate.GetDelegateWithKeyForManualSet().BindLambda([=, this](const FKey& Key)
		{
			PerformIndexedAction(Key, KeyConfig.ActionIndex, InputEvent);
		});
	}
	HyperManageInputComponent->KeyBindings.Add(KeyBinding);
	if ((KeyConfig.ActionIndex == CancelCopy || KeyConfig.ActionIndex == ToggleCopyMode || KeyConfig.ActionIndex == ClickPlaceCopy || KeyConfig.ActionIndex == RotateCopyRight || KeyConfig.ActionIndex == RotateCopyLeft || KeyConfig.ActionIndex == ResetCopyRotation) && InputEvent == IE_Pressed)
	{
		PreviewBindingIndices.Add(HyperManageInputComponent->KeyBindings.Num() - 1);
		RefreshPreviewBinding();
	}
}

void UHyperManageInput::SetupInputComponent()
{
	if (HyperManageInputComponent) {
		return;
	}
	auto* EnhancedComponent = NewObject<UEnhancedInputComponent>(Equip);
	HyperManageInputComponent = EnhancedComponent;
	PreviewWheelAction = NewObject<UInputAction>(this);
	PreviewWheelAction->ValueType = EInputActionValueType::Axis1D;
	PreviewWheelAction->bConsumeInput = true;
	PreviewWheelContext = NewObject<UInputMappingContext>(this);
	PreviewWheelContext->MapKey(PreviewWheelAction, EKeys::MouseWheelAxis);
	EnhancedComponent->BindAction(PreviewWheelAction, ETriggerEvent::Triggered, this, &UHyperManageInput::HandlePreviewWheel);
	HyperManageInputComponent->RegisterComponent();

	for (const auto& ThisKey : System->Config->MMKeyConfigs.ActionKeys) {
		if (ThisKey.Key == EKeys::Invalid) {
			continue;
		}
		// Wheel rotation is handled by Enhanced Input so the game's equipment axis is consumed.
		if (IsPreviewWheelKey(ThisKey)) continue;
		SetupKeyBinding(ThisKey, EInputEvent::IE_Pressed);
		if (ThisKey.UseRepeats) {
			if ((ThisKey.Key == EKeys::LeftMouseButton) || (ThisKey.Key == EKeys::MiddleMouseButton) ||
				(ThisKey.Key == EKeys::RightMouseButton)) {

				SetupKeyBinding(ThisKey, EInputEvent::IE_Released);
			} else {
				SetupKeyBinding(ThisKey, EInputEvent::IE_Repeat);
			}
		}
	}
}

void UHyperManageInput::PerformIndexedAction(FKey Key, EActionNameIdx ActionIndex, EInputEvent InputEvent)
{
	// Destructive actions run once per press, regardless of configured repeat behavior.
	if ((ActionIndex == DeleteSelection || ActionIndex == CopySelection || ActionIndex == PlaceCopy || ActionIndex == CancelCopy || ActionIndex == ToggleCopyMode || ActionIndex == ClickPlaceCopy || ActionIndex == RotateCopyRight || ActionIndex == RotateCopyLeft || ActionIndex == ResetCopyRotation) && InputEvent != IE_Pressed) return;

	// remove timer if the key has been released
	if (InputEvent == EInputEvent::IE_Released) {
		if (KeyTimerHandleMap.Contains(Key)) {
			Equip->GetWorldTimerManager().ClearTimer(KeyTimerHandleMap[Key]);
			KeyTimerHandleMap.Remove(Key);
		}
		return;
	}

	// check to see if Key is actually pressed for an existing timer and remove it if it's not
	if (InputEvent == EInputEvent::IE_Repeat) { 
		if (KeyTimerHandleMap.Contains(Key) && !System->GetLocalController()->IsInputKeyDown(Key)) {
			Equip->GetWorldTimerManager().ClearTimer(KeyTimerHandleMap[Key]);
			KeyTimerHandleMap.Remove(Key);
			return;
		}
	}

	if (InputEvent == EInputEvent::IE_Pressed && Key.IsMouseButton()) {
  UE_LOG(LogTemp, Display, TEXT("HyperManage input: key=%s action=%d"), *Key.ToString(), static_cast<int32>(ActionIndex));
 }
 System->ExecuteAction(ActionIndex);
}

bool UHyperManageInput::IsPreviewWheelKey(const FHyperManageKeyConfig& Key)
{
	return (Key.ActionIndex == RotateCopyRight || Key.ActionIndex == RotateCopyLeft) && (Key.Key == EKeys::MouseScrollUp || Key.Key == EKeys::MouseScrollDown);
}

void UHyperManageInput::RemovePreviewWheel()
{
	if (auto* Subsystem = PreviewWheelSubsystem.Get())
	{
		Subsystem->RemoveMappingContext(PreviewWheelContext);
	}
	PreviewWheelSubsystem.Reset();
}

void UHyperManageInput::RefreshPreviewWheel()
{
	const bool Active = IsValid(Equip) && System && System->CopyPreview && System->CopyPreview->GetCount() > 0 && System->Config && System->Config->MMKeyConfigs.ActionKeys.ContainsByPredicate(IsPreviewWheelKey);
	if (!Active)
	{
		RemovePreviewWheel();
		return;
	}
	if (!PreviewWheelContext || PreviewWheelSubsystem.IsValid() || !System->GetWorld()) return;
	auto* Controller = System->GetLocalController();
	auto* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	auto* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (Subsystem)
	{
		Subsystem->AddMappingContext(PreviewWheelContext, 1000);
		PreviewWheelSubsystem = Subsystem;
	}
}

void UHyperManageInput::HandlePreviewWheel(const FInputActionValue& Value)
{
	if (!IsValid(Equip) || !System || !System->GetWorld() || !System->CopyPreview || System->CopyPreview->GetCount() == 0 || !System->Config) return;
	auto* Controller = System->GetLocalController();
	if (!Controller || Controller->bShowMouseCursor || Controller->IsLookInputIgnored()) return;
	const float Delta = Value.Get<float>();
	if (!FMath::IsFinite(Delta) || FMath::IsNearlyZero(Delta)) return;
	const FKey Direction = Delta > 0 ? EKeys::MouseScrollUp : EKeys::MouseScrollDown;
	const bool Ctrl = Controller->IsInputKeyDown(EKeys::LeftControl) || Controller->IsInputKeyDown(EKeys::RightControl);
	const bool Alt = Controller->IsInputKeyDown(EKeys::LeftAlt) || Controller->IsInputKeyDown(EKeys::RightAlt);
	const bool Shift = Controller->IsInputKeyDown(EKeys::LeftShift) || Controller->IsInputKeyDown(EKeys::RightShift);
	for (const auto& Key : System->Config->MMKeyConfigs.ActionKeys)
	{
		if (IsPreviewWheelKey(Key) && Key.Key == Direction && Key.Ctrl == Ctrl && Key.Alt == Alt && Key.Shift == Shift)
		{
			System->ExecuteAction(Key.ActionIndex);
			return;
		}
	}
}

void UHyperManageInput::RefreshPreviewBinding()
{
	RefreshPreviewWheel();
	if (!HyperManageInputComponent) return;
	for (int32 Index : PreviewBindingIndices)
	{
		if (HyperManageInputComponent->KeyBindings.IsValidIndex(Index))
		{
			HyperManageInputComponent->KeyBindings[Index].bConsumeInput = System && System->CopyPreview && System->CopyPreview->GetCount() > 0;
		}
	}
}

void UHyperManageInput::Attach(AHyperManageEquip* Equipment)
{
	if (Equip == Equipment) return;
	if (IsValid(Equip)) Detach();
	Equip = Equipment;
	SetupInputComponent();
	System->GetLocalController()->PushInputComponent(HyperManageInputComponent);
	RefreshPreviewWheel();
}

void UHyperManageInput::Detach()
{
	RemovePreviewWheel();
	ClearAllKeyTimers();
	if (HyperManageInputComponent && System->GetLocalController()) System->GetLocalController()->PopInputComponent(HyperManageInputComponent);
	if (IsValid(HyperManageInputComponent)) HyperManageInputComponent->DestroyComponent();
	HyperManageInputComponent = nullptr;
	PreviewBindingIndices.Reset();
	PreviewWheelContext = nullptr;
	PreviewWheelAction = nullptr;
	Equip = nullptr;
}