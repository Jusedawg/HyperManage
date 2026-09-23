#pragma once
#include "Subsystem/ModSubsystem.h"
#include "FGSaveInterface.h"
#include "HyperManageSlotStore.generated.h"

USTRUCT()
struct FHyperManageStoredSlot
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) TArray<TObjectPtr<AActor>> Actors;
 UPROPERTY(SaveGame) TObjectPtr<AActor> Anchor;
 UPROPERTY(SaveGame) TObjectPtr<AActor> Target;
 UPROPERTY(SaveGame) FString Name;
 UPROPERTY(SaveGame) bool Occupied = false;
 UPROPERTY(SaveGame) bool BlueprintSlot = false;
 UPROPERTY(SaveGame) TObjectPtr<class AFGBlueprintProxy> Blueprint;
};

UCLASS()
class HYPERMANAGE_API AHyperManageSlotStore : public AModSubsystem, public IFGSaveInterface
{
 GENERATED_BODY()
public:
 AHyperManageSlotStore();
 UPROPERTY(SaveGame) TArray<FHyperManageStoredSlot> Slots;
 static AHyperManageSlotStore* Get(UWorld* World);
 static bool CanPersist(const TArray<TObjectPtr<AActor>>& Actors);
 bool Store(int32 Index, const FHyperManageStoredSlot& Slot);
 void SanitizeSlots();
 virtual void PreSaveGame_Implementation(int32 SaveVersion, int32 GameVersion) override { SanitizeSlots(); }
 virtual void PostLoadGame_Implementation(int32 SaveVersion, int32 GameVersion) override { SanitizeSlots(); }
 virtual bool ShouldSave_Implementation() const override { return true; }
 virtual bool NeedTransform_Implementation() override { return false; }
};
