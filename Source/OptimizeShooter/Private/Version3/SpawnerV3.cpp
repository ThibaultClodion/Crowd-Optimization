// Fill out your copyright notice in the Description page of Project Settings.

#include "Version3/SpawnerV3.h"
#include "Components/BoxComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Version3/ZombieV3.h"
#include <NiagaraFunctionLibrary.h>
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"

ASpawnerV3::ASpawnerV3()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("Spawn Area"));
	RootComponent = SpawnArea;

	static ConstructorHelpers::FObjectFinder<USoundBase> MoanSoundAsset(TEXT("/Game/Assets/Zombie/FX/Moan/MSS_Moan2.MSS_Moan2"));
	if (MoanSoundAsset.Succeeded()) MoanSound = MoanSoundAsset.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> HitSoundAsset(TEXT("/Game/Assets/Zombie/FX/Hit/MSS_HitFlesh.MSS_HitFlesh"));
	if (HitSoundAsset.Succeeded()) HitSound = HitSoundAsset.Object;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitEffectAsset(TEXT("/Game/Assets/Zombie/FX/Hit/NS_Blood.NS_Blood"));
	if (HitEffectAsset.Succeeded()) HitEffect = HitEffectAsset.Object;
}

void ASpawnerV3::BeginPlay()
{
	Super::BeginPlay();

	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World = GetWorld();
	PlayerTarget = UGameplayStatics::GetPlayerCharacter(World, 0);

	InitializeData();
	InitializePool();

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

void ASpawnerV3::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PathFindingSystem.Update(DeltaTime, Positions, AliveIndices, PlayerTarget->GetActorLocation());
	UpdateMovementSystem(DeltaTime);
	UpdateHealthSystem();
	UpdateDeathSystem(DeltaTime);
	UpdateMoanSystem(DeltaTime);

	SyncActorTransforms();
}

void ASpawnerV3::InitializeData()
{
	Positions.SetNum(MaxZombieCount);
	Rotations.SetNum(MaxZombieCount);
	Healths.SetNum(MaxZombieCount);
	DeathTimers.SetNum(MaxZombieCount);
	MoanTimers.SetNum(MaxZombieCount);
	IsAlive.Init(false, MaxZombieCount);
	AliveIndices.Reserve(MaxZombieCount);

	// Initialize arrays
	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		Positions[i] = FVector::ZeroVector;
		Rotations[i] = FRotator::ZeroRotator;
		Healths[i] = InitialHealth;
		DeathTimers[i] = -1.f;
		MoanTimers[i] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
	}
}

void ASpawnerV3::InitializePool()
{
	ZombieActorPool.SetNum(MaxZombieCount);

	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		AZombieV3* Zombie = World->SpawnActor<AZombieV3>(AZombieV3::StaticClass(), FTransform::Identity, SpawnParams);
		Zombie->ZombieIndex = i;
		Zombie->SetActive(false);
		ZombieActorPool[i] = Zombie;
	}
}

void ASpawnerV3::SetZombieCount(int32 Count)
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

void ASpawnerV3::HitZombieAtIndex(int32 Index, float Damage, FVector HitLocation)
{
	Healths[Index] -= Damage;

	// Play Hit Feedback
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), HitSound, HitLocation);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, HitLocation);
}

void ASpawnerV3::UpdateMovementSystem(float DeltaTime)
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
		float DistanceToWaypoint = FVector::Dist(Positions[Index], TargetWaypoint);
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
		FVector Direction = (TargetWaypoint - Positions[Index]).GetSafeNormal();
		FVector NewPosition = Positions[Index] + Direction * MovementSpeed * DeltaTime;

		// Project onto NavMesh
		FNavLocation ProjectedLocation;
		if (NavSystem->ProjectPointToNavigation(NewPosition, ProjectedLocation, FVector(100.f, 100.f, 500.f), NavData))
		{
			Positions[Index] = ProjectedLocation.Location;
			Positions[Index].Z += 90.f;
		}
		else
		{
			Positions[Index].X = NewPosition.X;
			Positions[Index].Y = NewPosition.Y;
		}

		// Update rotation
		FRotator TargetRotation = FRotator(0.f, Direction.Rotation().Yaw, 0.f);
		Rotations[Index] = FMath::RInterpTo(Rotations[Index], TargetRotation, DeltaTime, RotationSpeed);
	}
}

void ASpawnerV3::UpdateHealthSystem()
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

void ASpawnerV3::UpdateDeathSystem(float DeltaTime)
{
	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		if (DeathTimers[i] >= 0.f)
		{
			DeathTimers[i] -= DeltaTime;

			if (DeathTimers[i] < 0.f)
			{
				ZombieActorPool[i]->OnSwitchAlive.Broadcast();
				SpawnZombie(i);
			}
		}
	}
}

void ASpawnerV3::UpdateMoanSystem(float DeltaTime)
{
	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];

		MoanTimers[Index] -= DeltaTime;

		if (MoanTimers[Index] <= 0.f)
		{
			UGameplayStatics::SpawnSoundAtLocation(World, MoanSound, Positions[Index]);
			MoanTimers[Index] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
		}
	}
}

void ASpawnerV3::SyncActorTransforms()
{
	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];
		ZombieActorPool[Index]->SetActorLocationAndRotation(Positions[Index], Rotations[Index]);
	}
}

void ASpawnerV3::SpawnZombie(int32 Index)
{
	// Initialize data
	Positions[Index] = GetRandomSpawnPoint();
	Rotations[Index] = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
	Healths[Index] = InitialHealth;
	DeathTimers[Index] = -1.f;
	MoanTimers[Index] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
	IsAlive[Index] = true;

	AliveIndices.Add(Index);
	ZombieActorPool[Index]->SetActive(true);
}

void ASpawnerV3::KillZombie(int32 Index)
{
	IsAlive[Index] = false;
	DeathTimers[Index] = RespawnDelay;
	MoanTimers[Index] = MAX_flt;
	Healths[Index] = MAX_flt;

	// Clear pathfinding
	PathFindingSystem.ClearZombieData(Index);

	ZombieActorPool[Index]->OnSwitchAlive.Broadcast();
	ZombieActorPool[Index]->SetCollisionEnabled(false);

	AliveIndices.Remove(Index);
}

FVector ASpawnerV3::GetRandomSpawnPoint() const
{
	FVector Origin = SpawnArea->GetComponentLocation();
	FVector Extent = SpawnArea->GetScaledBoxExtent();

	float RandX = FMath::FRandRange(-Extent.X, Extent.X);
	float RandY = FMath::FRandRange(-Extent.Y, Extent.Y);

	return Origin + FVector(RandX, RandY, 0.f);
}
