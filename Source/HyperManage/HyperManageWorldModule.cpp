#include "HyperManageSlotStore.h"
#include "HyperManageWorldModule.h"
#include "FGSchematic.h"
#include "UObject/ConstructorHelpers.h"

UHyperManageWorldModule::UHyperManageWorldModule()
{
	bRootModule = true;
	ModSubsystems.Add(AHyperManageSlotStore::StaticClass());
	static ConstructorHelpers::FClassFinder<UFGSchematic> Schematic(TEXT("/HyperManage/Schematics/Schematic_HyperManage"));
	if (Schematic.Succeeded()) mSchematics.Add(Schematic.Class);
}
