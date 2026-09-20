#include "HyperManageLightweight.h"
#include "Buildables/FGBuildable.h"
#include "Components/StaticMeshComponent.h"
#include "AbstractInstanceManager.h"
#include "FGBuildEffectActor.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace {
// The current game uses build-effect actors, not the older material-effect components.
struct FScopedInstantEditEffects
{
 UWorld* World;
 TSet<AFGBuildEffectActor*> Existing;
 explicit FScopedInstantEditEffects(AFGLightweightBuildableSubsystem* Subsystem) : World(Subsystem->GetWorld())
 {
  for (TActorIterator<AFGBuildEffectActor> It(World); It; ++It) Existing.Add(*It);
 }
 ~FScopedInstantEditEffects()
 {
  TArray<TWeakObjectPtr<AFGBuildEffectActor>> Created;
  for (TActorIterator<AFGBuildEffectActor> It(World); It; ++It) if (!Existing.Contains(*It)) Created.Add(*It);
  FString Report = FString::Printf(TEXT("Existing build-effect actors=%d; edit-created actors=%d\n"), Existing.Num(), Created.Num());
  for (const auto& Weak : Created) if (auto* Effect = Weak.Get()) {
   Report += FString::Printf(TEXT("Hidden %s dismantle=%d\n"), *Effect->GetClass()->GetName(), Effect->mIsMarkedDismantle);
   // Hide only the edit-created cosmetic actor; let its normal lifecycle finish cleanup and notifications.
   Effect->SetActorHiddenInGame(true);
  }
  FFileHelper::SaveStringToFile(Report, *FPaths::Combine(FPaths::ProjectLogDir(), TEXT("HyperManage-EditEffects.txt")));
 }
};
}

bool FHyperManageLightweightRef::Matches(const FRuntimeBuildableInstanceData* Data) const
{
	return Data && Data->IsValid() && Data->BuiltWithRecipe == Recipe && Data->Transform.Equals(ExpectedTransform, 0.01);
}

bool HyperManageLightweight::IsValidTransform(const FTransform& Transform)
{
	const FVector Scale = Transform.GetScale3D();
	return !Transform.ContainsNaN() && Transform.GetRotation().IsNormalized() && Transform.GetLocation().GetAbsMax() < 1.e9 &&
		Scale.GetAbsMin() >= 0.001 && Scale.GetAbsMax() <= 1000.0;
}

AHyperManageLightweightProxy::AHyperManageLightweightProxy()
{
	bReplicates = false;
	SetActorEnableCollision(false);
	auto* Root = CreateDefaultSubobject<USceneComponent>(TEXT("SelectionRoot"));
	SetRootComponent(Root);
	SetActorHiddenInGame(true);
}

void AHyperManageLightweightProxy::Initialize(TSubclassOf<AFGBuildable> Class, int32 Index, const FRuntimeBuildableInstanceData& Data)
{
	Ref.SelectionId = FGuid::NewGuid();
	Ref.BuildableClass = Class;
	Ref.Index = Index;
	Ref.Recipe = Data.BuiltWithRecipe;
	Ref.ExpectedTransform = Data.Transform;
	Customization = Data.CustomizationData;
	SetActorTransform(Data.Transform);
	// Use the live instance geometry, including ramps and multi-mesh buildings, instead of a solid bounds cube.
	for (const auto& Handle : Data.Handles) {
		if (!Handle.IsValid() || !Handle->IsInstanced()) continue;
		const auto* Instance = Handle->GetInstanceComponent();
		if (!Instance || !Instance->GetStaticMesh()) continue;
		auto* Mesh = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Mesh->SetupAttachment(GetRootComponent());
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Instance->GetStaticMesh());
		Mesh->SetRelativeTransform(Handle->GetWorldTransform().GetRelativeTransform(Data.Transform));
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		Mesh->SetRenderInMainPass(false);
		Mesh->SetRenderInDepthPass(false);
		Mesh->SetCanEverAffectNavigation(false);
		AddInstanceComponent(Mesh);
		Mesh->RegisterComponent();
	}
}

void AHyperManageLightweightProxy::ApplyAcknowledgement(const FHyperManageLightweightRef& UpdatedRef, const FFactoryCustomizationData& Data)
{
	Pending = false;
	Ref = UpdatedRef;
	Customization = Data;
	SetActorTransform(Ref.ExpectedTransform);
	AcknowledgedAt = GetWorld()->GetTimeSeconds();
	Available = true;
}

void AHyperManageLightweightProxy::BeginRequest()
{
	Pending = true;
	RequestedAt = GetWorld()->GetTimeSeconds();
}

bool AHyperManageLightweightProxy::IsPending() const
{
	return Available && Pending && GetWorld()->GetTimeSeconds() - RequestedAt < 10.0;
}

bool AHyperManageLightweightProxy::IsAvailable() const
{
	if (!Available || (Pending && !IsPending())) return false;
	if (IsPending()) return true;
	auto* Subsystem = AFGLightweightBuildableSubsystem::Get(GetWorld());
	if (!Subsystem) return false;
	// Native replication and this acknowledgement use different channels.
	const auto* Instances = Subsystem->GetAllLightweightBuildableInstances().Find(Ref.BuildableClass);
	return (Instances && Instances->IsValidIndex(Ref.Index) && Ref.Matches(&(*Instances)[Ref.Index])) ||
		GetWorld()->GetTimeSeconds() - AcknowledgedAt < 5.0;
}

bool HyperManageLightweight::Replace(AFGLightweightBuildableSubsystem* Subsystem, FHyperManageLightweightRef& Ref, const FTransform& Transform)
{
	if (!IsValid(Subsystem) || !Subsystem->HasAuthority() || !IsValidTransform(Transform)) return false;
	const auto* Original = Subsystem->GetRuntimeDataForBuildableClassAndIndex(Ref.BuildableClass, Ref.Index);
	if (!Ref.Matches(Original)) return false;
	if (Original->Transform.Equals(Transform)) return true;
	FScopedInstantEditEffects InstantEffects(Subsystem);
	FRuntimeBuildableInstanceData Replacement = *Original;
	Replacement.Transform = Transform;
	Replacement.ConstructId = MAX_uint16;
	Replacement.Handles.Empty();
	Replacement.GridElementIds.Empty();
	Replacement.BoundingBox = FBox(ForceInit);
	// Add first so a one-part blueprint retains its proxy during replacement.
	const int32 NewIndex = Subsystem->AddFromBuildableInstanceData(Ref.BuildableClass, Replacement);
	const auto* Created = NewIndex != INDEX_NONE ? Subsystem->GetRuntimeDataForBuildableClassAndIndex(Ref.BuildableClass, NewIndex) : nullptr;
	if (!Created || !Created->IsValid()) return false;
	Subsystem->RemoveByInstanceIndex(Ref.BuildableClass, Ref.Index);
	Ref.Index = NewIndex;
	Ref.ExpectedTransform = Transform;
	return true;
}