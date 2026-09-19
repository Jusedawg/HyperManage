#include "HyperManageModule.h"
#include "HyperManageSystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"


void FHyperManageModule::StartupModule()
{
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda([](UWorld* World, bool, bool) {
		UHyperManageSystem::ReleaseWorld(World);
	});
}

void FHyperManageModule::ShutdownModule()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
}

IMPLEMENT_MODULE(FHyperManageModule, HyperManage);
