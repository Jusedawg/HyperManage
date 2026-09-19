#include "HyperManageConfig.h"
#include "Configuration/ConfigManager.h"
#include "JsonObjectConverter.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogHyperManageConfig, Log, All);

void UHyperManageConfiguration::Init()
{
	Super::Init();
	LoadHyperManageConfig();
	LoadKeyConfigs();
	// Do not overwrite existing configuration on load, especially if parsing fails.
}

void UHyperManageConfiguration::WriteStructToConfig(const FString& ConfigName, void* StructPtr, UScriptStruct* ScriptStruct)
{
	const FString ConfigPath = UConfigManager::GetConfigurationFolderPath() / ConfigName;
	TSharedRef<FJsonObject> ConfigValues = MakeShared<FJsonObject>();
	if (!FJsonObjectConverter::UStructToJsonObject(ScriptStruct, StructPtr, ConfigValues, 0, 0)) return;
	FString ConfigString;
	const TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&ConfigString);
	if (!FJsonSerializer::Serialize(ConfigValues, JsonWriter)) return;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ConfigPath), true);
	const FString TemporaryPath = ConfigPath + TEXT(".tmp");
	if (!FFileHelper::SaveStringToFile(ConfigString, *TemporaryPath) || !IFileManager::Get().Move(*ConfigPath, *TemporaryPath, true, true)) {
		UE_LOG(LogHyperManageConfig, Warning, TEXT("Could not save configuration: %s"), *ConfigPath);
	}
}

void UHyperManageConfiguration::ReadConfigIntoStruct(const FString& ConfigName, void* StructPtr, UScriptStruct* ScriptStruct)
{
	FString ConfigPath = UConfigManager::GetConfigurationFolderPath() / ConfigName;
	if (!FPaths::FileExists(ConfigPath)) return;
	FString ConfigString;
	if (!FFileHelper::LoadFileToString(ConfigString, *ConfigPath)) return;
	TSharedPtr<FJsonObject> ConfigValues;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ConfigString), ConfigValues) || !ConfigValues.IsValid()) {
		UE_LOG(LogHyperManageConfig, Warning, TEXT("Invalid JSON in %s; keeping defaults and preserving the file."), *ConfigPath);
		return;
	}
	// Convert into a temporary copy so a failed field conversion cannot partially change live settings.
	void* TemporaryStruct = FMemory::Malloc(ScriptStruct->GetStructureSize(), ScriptStruct->GetMinAlignment());
	ScriptStruct->InitializeStruct(TemporaryStruct);
	ScriptStruct->CopyScriptStruct(TemporaryStruct, StructPtr);
	if (FJsonObjectConverter::JsonObjectToUStruct(ConfigValues.ToSharedRef(), ScriptStruct, TemporaryStruct, 0, 0)) {
		ScriptStruct->CopyScriptStruct(StructPtr, TemporaryStruct);
	} else {
		UE_LOG(LogHyperManageConfig, Warning, TEXT("Invalid settings in %s; keeping defaults and preserving the file."), *ConfigPath);
	}
	ScriptStruct->DestroyStruct(TemporaryStruct);
	FMemory::Free(TemporaryStruct);
}

void UHyperManageConfiguration::SaveHyperManageConfig()
{
	WriteStructToConfig(FILENAME_CONFIG, &MMConfig, MMConfig.StaticStruct());
}

void UHyperManageConfiguration::SaveKeyConfigs()
{
	WriteStructToConfig(FILENAME_KEYS, &MMKeyConfigs, MMKeyConfigs.StaticStruct());
}

void UHyperManageConfiguration::LoadHyperManageConfig()
{
	// initialize default values and read in any existing configuration
	MMConfig.IncrementSettings.Empty();
	MMConfig.MaxTargetRangeMeters = 100.f;
	MMConfig.SelectionTolerance = 0.5f;
	MMConfig.AlignmentGridCm = 800.f;
	MMConfig.CurrentSelectedMaterial = 0;
	MMConfig.IsGrouped = true;
	MMConfig.IsViewBased = true;
	MMConfig.IsScaleLockedLR = false;
	MMConfig.IsScaleLockedTB = false;
	MMConfig.IsScaleLockedFB = false;
	MMConfig.WarningShownForLargeMoveLag = true;
	MMConfig.CurrentIncrementSize = "Medium";
	MMConfig.IncrementSize = EIncrementSize::Medium;
	MMConfig.DoNotEditConfigFormatVersion = "1.0";
	ReadConfigIntoStruct(FILENAME_CONFIG, &MMConfig, MMConfig.StaticStruct());

	if (!FMath::IsFinite(MMConfig.MaxTargetRangeMeters) || MMConfig.MaxTargetRangeMeters <= 0.f) MMConfig.MaxTargetRangeMeters = 100.f;
	if (!FMath::IsFinite(MMConfig.SelectionTolerance) || MMConfig.SelectionTolerance < 0.f) MMConfig.SelectionTolerance = 0.5f;
	if (!FMath::IsFinite(MMConfig.AlignmentGridCm) || MMConfig.AlignmentGridCm < 1.f || MMConfig.AlignmentGridCm > 100000.f) MMConfig.AlignmentGridCm = 800.f;
	MMConfig.CurrentSelectedMaterial = FMath::Clamp(MMConfig.CurrentSelectedMaterial, 0, 1);

	// Keep a fixed enum order even if the JSON contains reordered, duplicate or unknown entries.
	TArray<FHyperManageIncrement> NormalizedIncrements;
	auto CheckIncrementSetting = [&](FHyperManageIncrement DefaultIncrement)
	{
		for (const auto& Loaded : MMConfig.IncrementSettings) {
			if (!Loaded.Size.Equals(DefaultIncrement.Size, ESearchCase::IgnoreCase)) continue;
			if (FMath::IsFinite(Loaded.CentimetersToMove) && Loaded.CentimetersToMove > 0.f) DefaultIncrement.CentimetersToMove = Loaded.CentimetersToMove;
			if (FMath::IsFinite(Loaded.DegreesToRotate) && Loaded.DegreesToRotate >= 0.1f && Loaded.DegreesToRotate <= 180.f) DefaultIncrement.DegreesToRotate = Loaded.DegreesToRotate;
			if (FMath::IsFinite(Loaded.PercentToGrow) && Loaded.PercentToGrow > 0.f && Loaded.PercentToGrow < 100.f) DefaultIncrement.PercentToGrow = Loaded.PercentToGrow;
			break;
		}
		NormalizedIncrements.Add(DefaultIncrement);
	};
	CheckIncrementSetting(FHyperManageIncrement(EIncrementSize::Tiny, 1.f, 1.f, 1.f));
	CheckIncrementSetting(FHyperManageIncrement(EIncrementSize::Medium, 10.f, 5.f, 5.f));
	CheckIncrementSetting(FHyperManageIncrement(EIncrementSize::Large, 25.f, 10.f, 10.f));
	CheckIncrementSetting(FHyperManageIncrement(EIncrementSize::Huge, 100.f, 45.f, 20.f));
	MMConfig.IncrementSettings = MoveTemp(NormalizedIncrements);
	// convert loaded string CurrentIncrementSize to enum IncrementSize
	for (const auto& ThisInc : MMConfig.IncrementSettings) {
		if (ThisInc.Size.Compare(MMConfig.CurrentIncrementSize, ESearchCase::IgnoreCase) == 0) {
			MMConfig.IncrementSize = ThisInc.IncrementSize;
			break;
		}
	}
}

void UHyperManageConfiguration::LoadKeyConfigs()
{
	// initialize default values and read in any existing configuration
	MMKeyConfigs.ActionKeys.Empty();
	MMKeyConfigs.DoNotEditConfigFormatVersion = "1.0";
	ReadConfigIntoStruct(FILENAME_KEYS, &MMKeyConfigs, MMKeyConfigs.StaticStruct());

	// use default values if wasn't loaded from the existing configuration
	enum { NoAlt, Alt };
	enum { NoCtrl, Ctrl };
	enum { NoShift, Shift };
	enum { NoRepeat, Repeats };

	auto CheckKeyConfigSettings = [&](FHyperManageKeyConfig KeyConfig)
	{
		for (auto& ThisKey : MMKeyConfigs.ActionKeys) {
			if (ThisKey.ActionName.Compare(KeyConfig.ActionName, ESearchCase::IgnoreCase) == 0) {
				ThisKey.ActionIndex = KeyConfig.ActionIndex;
				ThisKey.Key = FKey(FName(*ThisKey.KeyName));
				return;
			}
		}
		MMKeyConfigs.ActionKeys.Add(KeyConfig);
	};

	CheckKeyConfigSettings(FHyperManageKeyConfig(SelectTarget, EKeys::LeftMouseButton, Ctrl, NoAlt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(DeselectTarget, EKeys::RightMouseButton, Ctrl, NoAlt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(Undo, EKeys::Z, Ctrl, NoAlt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(ChangeIncSize, EKeys::I, Ctrl, Alt, NoShift, NoRepeat));
	CheckKeyConfigSettings(FHyperManageKeyConfig(KnowNotes, EKeys::K, Ctrl, Alt, NoShift, NoRepeat));
	CheckKeyConfigSettings(FHyperManageKeyConfig(ShowTools, EKeys::RightMouseButton, NoCtrl, NoAlt, NoShift, NoRepeat));
	CheckKeyConfigSettings(FHyperManageKeyConfig(SetAnchor, EKeys::LeftMouseButton, NoCtrl, NoAlt, Shift, NoRepeat));
	CheckKeyConfigSettings(FHyperManageKeyConfig(SetTarget, EKeys::RightMouseButton, NoCtrl, NoAlt, Shift, NoRepeat));

	CheckKeyConfigSettings(FHyperManageKeyConfig(SpinLeft, EKeys::J, Ctrl, NoAlt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(SpinRight, EKeys::L, Ctrl, NoAlt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(MoveUp, EKeys::I, Ctrl, NoAlt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(MoveDown, EKeys::K, Ctrl, NoAlt, NoShift, Repeats));

	CheckKeyConfigSettings(FHyperManageKeyConfig(MoveLeft, EKeys::J, NoCtrl, Alt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(MoveRight, EKeys::L, NoCtrl, Alt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(MoveAway, EKeys::I, NoCtrl, Alt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(MoveToward, EKeys::K, NoCtrl, Alt, NoShift, Repeats));

	CheckKeyConfigSettings(FHyperManageKeyConfig(RollLeft, EKeys::J, NoCtrl, NoAlt, Shift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(RollRight, EKeys::L, NoCtrl, NoAlt, Shift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(PitchAway, EKeys::I, NoCtrl, NoAlt, Shift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(PitchToward, EKeys::K, NoCtrl, NoAlt, Shift, Repeats));

	CheckKeyConfigSettings(FHyperManageKeyConfig(Shrink, EKeys::J, Ctrl, Alt, NoShift, Repeats));
	CheckKeyConfigSettings(FHyperManageKeyConfig(Grow, EKeys::L, Ctrl, Alt, NoShift, Repeats));

	// add all actions that don't have default keys assigned to them
	EActionNameIdx Actions[] = { AlignLeft, AlignLRCenter, AlignRight, SpaceLR,	StackLR, LockScaleLR,
		AlignTop, AlignTBCenter, AlignBottom, SpaceTB, StackTB, LockScaleTB,
		AlignFront,AlignFBCenter, AlignBack, SpaceFB, StackFB, LockScaleFB,
		SameRotation, SameScale, SamePaint,
		SelectBoxSides, SelectBoxPivot, MoveSelection, CopySelection, NewSelection, DeleteSelection, SaveSelection, LoadSelection,
		ClearUndo, EActionNameIdx::IsGrouped, EActionNameIdx::IsViewBased, NextHologram, Settings };
	TArray<EActionNameIdx> InvalidKeyActions;
	InvalidKeyActions.Append(Actions, UE_ARRAY_COUNT(Actions));
	for (auto ActionIdx : InvalidKeyActions) {
		CheckKeyConfigSettings(FHyperManageKeyConfig(ActionIdx, EKeys::Invalid, NoCtrl, NoAlt, NoShift, NoRepeat));
	}
}

void UHyperManageConfiguration::NextIncrementSize()
{
	MMConfig.IncrementSize = static_cast<EIncrementSize>((MMConfig.IncrementSize + 1) % 4);
	MMConfig.CurrentIncrementSize = MMConfig.IncrementSettings[MMConfig.IncrementSize].Size;
}

EIncrementSize UHyperManageConfiguration::CurrentIncrementSize()
{
	return MMConfig.IncrementSize;
}

FKey UHyperManageConfiguration::GetKeyForAction(EActionNameIdx ActionIndex)
{
	for (auto KeyConfig : MMKeyConfigs.ActionKeys) {
		if (KeyConfig.ActionIndex == ActionIndex) {
			return KeyConfig.Key;
		}
	}
	return EKeys::Invalid;
}
