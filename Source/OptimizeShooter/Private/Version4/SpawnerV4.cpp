// Fill out your copyright notice in the Description page of Project Settings.

#include "Version4/SpawnerV4.h"
#include "Components/BoxComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Version3/ZombieV3.h"
#include <NiagaraFunctionLibrary.h>
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"

ASpawnerV4::ASpawnerV4()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("Spawn Area"));
	RootComponent = SpawnArea;

	ZombieHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Zombie HISM"));
	ZombieHISM->SetupAttachment(RootComponent);
	ZombieHISM->SetCollisionProfileName(TEXT("Zombie"));
	ZombieHISM->NumCustomDataFloats = 4;

	static ConstructorHelpers::FObjectFinder<USoundBase> MoanSoundAsset(TEXT("/Game/Assets/Zombie/FX/Moan/MSS_Moan2.MSS_Moan2"));
	if (MoanSoundAsset.Succeeded()) MoanSound = MoanSoundAsset.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> HitSoundAsset(TEXT("/Game/Assets/Zombie/FX/Hit/MSS_HitFlesh.MSS_HitFlesh"));
	if (HitSoundAsset.Succeeded()) HitSound = HitSoundAsset.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitEffectAsset(TEXT("/Game/Assets/Zombie/FX/Hit/NS_Blood.NS_Blood"));
	if (HitEffectAsset.Succeeded()) HitEffect = HitEffectAsset.Object;
}

void ASpawnerV4::BeginPlay()
{
	Super::BeginPlay();

	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World = GetWorld();
	PlayerTarget = UGameplayStatics::GetPlayerCharacter(World, 0);

	InitializeData();
	InitializeHISM();

	// Initialize pathfinding system
	NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	NavData = NavSystem->GetDefaultNavDataInstance();
	PathFindingSystem.Initialize(World, MaxZombieCount);
	PathFindingSystem.SetMaxPathRequestsPerFrame(MaxPathRequestsPerFrame);
	PathFindingSystem.SetPathUpdateInterval(PathUpdateInterval);
	PathFindingSystem.SetMaxPathDistance(MaxPathDistance);

	// Spawn initial zombies
	for (int32 i = 0; i < CurrentZombieCount; i++)
	{
		SpawnZombie(i);
	}
}

void ASpawnerV4::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Pathfinding and Movement
	PathFindingSystem.Update(DeltaTime, HISMTransforms, AliveIndices, PlayerTarget->GetActorLocation());
	UpdateMovementSystem(DeltaTime);
	UpdateMoanSystem(DeltaTime);

	// Health and Death
	UpdateHealthSystem();
	UpdateDeathSystem(DeltaTime);

	// Sync HISM Transforms
	SyncActorTransforms();
}

void ASpawnerV4::InitializeData()
{
	HISMTransforms.Init(FTransform::Identity, MaxZombieCount);
	Healths.Init(InitialHealth, MaxZombieCount);
	DeathTimers.Init(-1.f, MaxZombieCount);
	MoanTimers.Init(10.f, MaxZombieCount);
	IsAlive.Init(false, MaxZombieCount);
	AliveIndices.Reserve(MaxZombieCount);
}

void ASpawnerV4::InitializeHISM()
{
	ZombieHISM->ClearInstances();
	HISMTransforms.Empty();

	FTransform HiddenTransform;
	HiddenTransform.SetLocation(FVector(0, 0, -5000));
	HiddenTransform.SetScale3D(FVector(0.f));

	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		ZombieHISM->AddInstance(HiddenTransform);
		HISMTransforms.Add(HiddenTransform);
	}
}

void ASpawnerV4::SetZombieCount(int32 Count)
{
	if (Count > CurrentZombieCount)
	{
		int32 ZombiesToSpawn = FMath::Clamp(Count - CurrentZombieCount, 0, MaxZombieCount - CurrentZombieCount);
		for (int32 i = 0; i < ZombiesToSpawn; i++)
		{
			SpawnZombie(CurrentZombieCount + i);
		}
	}

	else if (Count < CurrentZombieCount)
	{
		int32 ZombiesToRemove = FMath::Clamp(CurrentZombieCount - Count, 0, CurrentZombieCount);
		for (int32 i = 0; i < ZombiesToRemove; i++)
		{
			int32 IndexToKill = AliveIndices.Last();
			KillZombie(IndexToKill);
		}
	}
}

void ASpawnerV4::HitZombieAtIndex(int32 Index, float Damage, FVector HitLocation)
{
	Healths[Index] -= Damage;

	// Play Hit Feedback
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), HitSound, HitLocation);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, HitLocation);
}

void ASpawnerV4::UpdateMovementSystem(float DeltaTime)
{
	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];

		FNavPathSharedPtr Path = PathFindingSystem.GetCachedPath(Index);
		if (!Path.IsValid()) continue;

		const TArray<FNavPathPoint>& PathPoints = Path->GetPathPoints();
		int32 CurrentWaypointIdx = PathFindingSystem.GetCurrentWaypointIndex(Index);

		if (CurrentWaypointIdx >= PathPoints.Num())
		{
			PathFindingSystem.ForcePathRecalculation(Index);
			continue;
		}

		FVector TargetWaypoint = PathPoints[CurrentWaypointIdx].Location;

		// Check if reached waypoint
		FVector LastPosition = HISMTransforms[Index].GetLocation();
		float DistanceToWaypoint = FVector::Dist(LastPosition, TargetWaypoint);
		if (DistanceToWaypoint < WaypointReachedDistance)
		{
			PathFindingSystem.AdvanceWaypoint(Index);
			CurrentWaypointIdx = PathFindingSystem.GetCurrentWaypointIndex(Index);

			if (CurrentWaypointIdx >= PathPoints.Num())
			{
				PathFindingSystem.ForcePathRecalculation(Index);
				continue;
			}

			TargetWaypoint = PathPoints[CurrentWaypointIdx].Location;
		}

		// Move towards waypoint
		FVector Direction = (TargetWaypoint - LastPosition).GetSafeNormal();
		FVector NewPosition = LastPosition + Direction * MovementSpeed * DeltaTime;

		// Project onto NavMesh
		FNavLocation ProjectedLocation;
		FVector NextPosition = NewPosition;
		if (NavSystem->ProjectPointToNavigation(NewPosition, ProjectedLocation, FVector(100.f, 100.f, 500.f), NavData))
		{
			NextPosition = ProjectedLocation.Location;
		}

		// Update rotation
		FRotator LastRotation = HISMTransforms[Index].GetRotation().Rotator();
		FRotator TargetRotation = FRotator(0.f, Direction.Rotation().Yaw - 90.f, 0.f);
		FRotator NewRotation = FMath::RInterpTo(LastRotation, TargetRotation, DeltaTime, RotationSpeed);

		// Apply new transform
		HISMTransforms[Index].SetLocation(NextPosition);
		HISMTransforms[Index].SetRotation(FQuat(NewRotation));
	}
}

void ASpawnerV4::UpdateHealthSystem()
{
	// Iterate backwards to allow safe removal from AliveIndices
	for (int32 i = AliveIndices.Num() - 1; i >= 0; i--)
	{
		const int32 Index = AliveIndices[i];

		if (Healths[Index] <= 0.f)
		{
			KillZombie(Index);
		}
	}
}

void ASpawnerV4::UpdateDeathSystem(float DeltaTime)
{
	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		if (DeathTimers[i] >= 0.f)
		{
			DeathTimers[i] -= DeltaTime;

			if (DeathTimers[i] < 0.f)
			{
				SpawnZombie(i);
			}
		}
	}
}

void ASpawnerV4::UpdateMoanSystem(float DeltaTime)
{
	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];

		MoanTimers[Index] -= DeltaTime;
		FVector Position = HISMTransforms[Index].GetLocation();

		if (MoanTimers[Index] <= 0.f)
		{
			UGameplayStatics::SpawnSoundAtLocation(World, MoanSound, Position);
			MoanTimers[Index] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
		}
	}
}

void ASpawnerV4::SyncActorTransforms()
{
	ZombieHISM->BatchUpdateInstancesTransforms(0, HISMTransforms, true, true, true);
}

void ASpawnerV4::SpawnZombie(int32 Index)
{
	// Initialize data
	HISMTransforms[Index].SetLocation(GetRandomSpawnPoint());
	HISMTransforms[Index].SetRotation(FQuat(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f)));
	HISMTransforms[Index].SetScale3D(FVector(1.f));
	Healths[Index] = InitialHealth;
	DeathTimers[Index] = -1.f;
	MoanTimers[Index] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
	IsAlive[Index] = true;

	AliveIndices.Add(Index);
	PlayWalkAnimation(Index);

	ZombieHISM->UpdateInstanceTransform(Index, HISMTransforms[Index], true, true, true);
}

void ASpawnerV4::KillZombie(int32 Index)
{
	IsAlive[Index] = false;
	DeathTimers[Index] = RespawnDelay;
	MoanTimers[Index] = MAX_flt;
	Healths[Index] = MAX_flt;

	// Clear pathfinding
	PathFindingSystem.ClearZombieData(Index);
	AliveIndices.Remove(Index);

	// Play death animation
	PlayDeathAnimation(Index);
}

FVector ASpawnerV4::GetRandomSpawnPoint() const
{
	FVector Origin = SpawnArea->GetComponentLocation();
	FVector Extent = SpawnArea->GetScaledBoxExtent();

	float RandX = FMath::FRandRange(-Extent.X, Extent.X);
	float RandY = FMath::FRandRange(-Extent.Y, Extent.Y);

	return Origin + FVector(RandX, RandY, 0.f);
}

void ASpawnerV4::PlayWalkAnimation(int32 Index)
{
	int32 RandomIndex = FMath::RandRange(0, WalkStartFrames.Num() - 1);
	float StartFrame = WalkStartFrames[RandomIndex];
	float EndFrame = WalkEndFrames[RandomIndex];

	ZombieHISM->SetCustomDataValue(Index, 0, 0, true);
	ZombieHISM->SetCustomDataValue(Index, 1, 1, true);
	ZombieHISM->SetCustomDataValue(Index, 2, StartFrame, true);
	ZombieHISM->SetCustomDataValue(Index, 3, EndFrame, true);
}

void ASpawnerV4::PlayDeathAnimation(int32 Index)
{
	int32 RandomIndex = FMath::RandRange(0, DeathStartFrames.Num() - 1);
	float StartFrame = DeathStartFrames[RandomIndex];
	float EndFrame = DeathEndFrames[RandomIndex];

	ZombieHISM->SetCustomDataValue(Index, 0, 0, true);
	ZombieHISM->SetCustomDataValue(Index, 1, 1, true);
	ZombieHISM->SetCustomDataValue(Index, 2, StartFrame, true);
	ZombieHISM->SetCustomDataValue(Index, 3, EndFrame, true);
}
