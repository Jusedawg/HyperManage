#include "HyperManageWorldModule.h"
#include "FGSchematic.h"
#include "UObject/ConstructorHelpers.h"

UHyperManageWorldModule::UHyperManageWorldModule()
{
	bRootModule = true;
	static ConstructorHelpers::FClassFinder<UFGSchematic> Schematic(TEXT("/HyperManage/Schematics/Schematic_HyperManage"));
	if (Schematic.Succeeded()) mSchematics.Add(Schematic.Class);
}
