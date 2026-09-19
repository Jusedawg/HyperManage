#pragma once

#include "CoreMinimal.h"
#include "HyperManageSystem.h"
#include "HyperManageConfig.generated.h"

#define FILENAME_CONFIG "HyperManage-Config.cfg"
#define FILENAME_KEYS "HyperManage-Keys.cfg"

USTRUCT(BlueprintType)
struct HYPERMANAGE_API FHyperManageKeyConfig
{
	GENERATED_BODY()

public:
	TEnumAsByte<EActionNameIdx> ActionIndex;
	
	FKey Key;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	FString ActionName;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	FString KeyName;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool Ctrl;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool Alt;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool Shift;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool UseRepeats;

public:
	FHyperManageKeyConfig() { }
	FHyperManageKeyConfig(EActionNameIdx InIndex, FKey InKey, bool InCtrl, bool InAlt, bool InShift, bool InRepeats)
		: ActionIndex(InIndex)
		, Key(InKey)
		, Ctrl(InCtrl)
		, Alt(InAlt)
		, Shift(InShift)
		, UseRepeats(InRepeats)
	{
		ActionName = UEnum::GetValueAsString<EActionNameIdx>(InIndex);
		KeyName = InKey.ToString();
	}
	FORCEINLINE ~FHyperManageKeyConfig() = default;
};

USTRUCT(BlueprintType)
struct HYPERMANAGE_API FHyperManageKeyConfigs
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	TArray<struct FHyperManageKeyConfig> ActionKeys;

	UPROPERTY()
	FString DoNotEditConfigFormatVersion;

public:
	FORCEINLINE ~FHyperManageKeyConfigs() = default;
};

USTRUCT(BlueprintType)
struct HYPERMANAGE_API FHyperManageIncrement
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Size;

	TEnumAsByte<EIncrementSize> IncrementSize;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	float CentimetersToMove;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	float DegreesToRotate;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	float PercentToGrow;

public:
	FHyperManageIncrement() { }
	FHyperManageIncrement(EIncrementSize InIncSize, float InMove, float InRotate, float InGrow)
		: IncrementSize(InIncSize)
		, CentimetersToMove(InMove)
		, DegreesToRotate(InRotate)
		, PercentToGrow(InGrow)
	{
		Size = UEnum::GetValueAsString<EIncrementSize>(InIncSize);
	}
	FORCEINLINE ~FHyperManageIncrement() = default;
};

USTRUCT(BlueprintType)
struct HYPERMANAGE_API FHyperManageConfig
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	TArray<struct FHyperManageIncrement> IncrementSettings;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	float MaxTargetRangeMeters;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	float SelectionTolerance;
	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	float AlignmentGridCm = 800.f;

	TEnumAsByte<EIncrementSize> IncrementSize;

	UPROPERTY(BlueprintReadOnly, Category = "HyperManage")
	FString CurrentIncrementSize;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	int CurrentSelectedMaterial;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool IsGrouped;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool IsViewBased;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool IsScaleLockedLR;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool IsScaleLockedTB;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool IsScaleLockedFB;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	bool WarningShownForLargeMoveLag;

	UPROPERTY()
	FString DoNotEditConfigFormatVersion;

public:
	FORCEINLINE ~FHyperManageConfig() = default;
};

UCLASS(BlueprintType)
class HYPERMANAGE_API UHyperManageConfiguration : public UHyperManageComponent
{
	GENERATED_BODY()

private:
	void WriteStructToConfig(const FString& ConfigName, void* PtrToStruct, UScriptStruct* ScriptStruct);

	void ReadConfigIntoStruct(const FString& ConfigName, void* StructPtr, UScriptStruct* ScriptStruct);

public:
	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	struct FHyperManageKeyConfigs MMKeyConfigs;

	UPROPERTY(BlueprintReadWrite, Category = "HyperManage")
	struct FHyperManageConfig MMConfig;

public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	void SaveHyperManageConfig();

	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	void SaveKeyConfigs();

	UFUNCTION()
	void LoadHyperManageConfig();

	UFUNCTION()
	void LoadKeyConfigs();

	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	void NextIncrementSize();

	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	EIncrementSize CurrentIncrementSize();

	UFUNCTION(BlueprintCallable, Category = "HyperManage")
	FKey GetKeyForAction(EActionNameIdx ActionIndex);

public:
};

