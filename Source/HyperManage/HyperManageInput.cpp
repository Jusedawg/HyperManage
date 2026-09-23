#include "TimerManager.h"
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
}

void UHyperManageInput::SetupInputComponent()
{
	if (HyperManageInputComponent) {
		return;
	}
	HyperManageInputComponent = NewObject<UInputComponent>(Equip);
	HyperManageInputComponent->RegisterComponent();

	for (const auto& ThisKey : System->Config->MMKeyConfigs.ActionKeys) {
		if (ThisKey.Key == EKeys::Invalid) {
			continue;
		}
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

void UHyperManageInput::Attach(AHyperManageEquip* Equipment)
{
	if (Equip == Equipment) return;
	if (IsValid(Equip)) Detach();
	Equip = Equipment;
	SetupInputComponent();
	System->GetLocalController()->PushInputComponent(HyperManageInputComponent);
}

void UHyperManageInput::Detach()
{
	ClearAllKeyTimers();
	if (HyperManageInputComponent && System->GetLocalController()) System->GetLocalController()->PopInputComponent(HyperManageInputComponent);
	if (IsValid(HyperManageInputComponent)) HyperManageInputComponent->DestroyComponent();
	HyperManageInputComponent = nullptr;
	Equip = nullptr;
}