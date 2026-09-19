#include "HyperManageInstanceModule.h"
#include "HyperManageRCO.h"

UHyperManageInstanceModule::UHyperManageInstanceModule()
{
	bRootModule = true;
	RemoteCallObjects.Add(UHyperManageRCO::StaticClass());
}
