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
	NavAgentProperties = NavData->GetConfig().DefaultProperties;

	// Spawn initial zombies
	for (int32 i = 0; i < CurrentZombieCount; i++)
	{
		SpawnZombie(i);
	}
}

void ASpawnerV3::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	Velocities.SetNum(MaxZombieCount);
	Healths.SetNum(MaxZombieCount);
	DeathTimers.SetNum(MaxZombieCount);
	MoanTimers.SetNum(MaxZombieCount);
	IsAlive.Init(false, MaxZombieCount);
	AliveIndices.Reserve(MaxZombieCount);

	// Pathfinding data
	CachedPaths.SetNum(MaxZombieCount);
	PathUpdateTimers.SetNum(MaxZombieCount);

	// Initialize arrays
	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		Positions[i] = FVector::ZeroVector;
		Rotations[i] = FRotator::ZeroRotator;
		Velocities[i] = FVector::ZeroVector;
		Healths[i] = InitialHealth;
		DeathTimers[i] = -1.f;
		MoanTimers[i] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
		CachedPaths[i] = nullptr;
		PathUpdateTimers[i] = 0.f;
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
	if (!PlayerTarget) return;

	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];

		// Update path request timer
		PathUpdateTimers[Index] -= DeltaTime;

		// Request new path if timer expired
		if (PathUpdateTimers[Index] <= 0.f)
		{
			PathUpdateTimers[Index] = PathUpdateInterval;

			// Find path synchronously for now (we'll improve this later)
			UNavigationPath* Path = NavSystem->FindPathToActorSynchronously(World, Positions[Index], PlayerTarget);
			CachedPaths[Index] = Path;
		}

		// Use cached path for movement
		UNavigationPath* Path = CachedPaths[Index];
		if (Path && Path->PathPoints.Num() > 1)
		{
			FVector TargetWaypoint = Path->PathPoints[1];

			// Calculate direction (use waypoint height from NavMesh)
			FVector Direction = (TargetWaypoint - Positions[Index]).GetSafeNormal();
			Velocities[Index] = Direction * MovementSpeed;

			// Calculate new position
			FVector NewPosition = Positions[Index] + Velocities[Index] * DeltaTime;

			// Project position onto NavMesh to ensure it stays on walkable surface
			FNavLocation ProjectedLocation;
			if (NavSystem->ProjectPointToNavigation(NewPosition, ProjectedLocation, FVector(100.f, 100.f, 500.f), NavData))
			{
				Positions[Index] = ProjectedLocation.Location;
				Positions[Index].Z += 90.f; // Adjust for zombie height
			}
			else
			{
				// Fallback: keep horizontal movement but preserve current height
				Positions[Index].X = NewPosition.X;
				Positions[Index].Y = NewPosition.Y;
			}

			// Lerp rotation towards movement direction
			FRotator TargetRotation = Direction.Rotation();
			TargetRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
			Rotations[Index] = FMath::RInterpTo(Rotations[Index], TargetRotation, DeltaTime, RotationSpeed);
		}
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

void ASpawnerV3::RequestPathForZombie(int32 ZombieIndex)
{
	if (!PlayerTarget) return;

	// Create path finding query
	FPathFindingQuery Query(this, *NavData, Positions[ZombieIndex], PlayerTarget->GetActorLocation());

	// Request async path
	uint32 QueryID = NavSystem->FindPathAsync(
		NavAgentProperties,
		Query,
		FNavPathQueryDelegate::CreateUObject(this, &ASpawnerV3::OnPathFound),
		EPathFindingMode::Regular
	);
}

void ASpawnerV3::OnPathFound(uint32 PathId, ENavigationQueryResult::Type Result, FNavPathSharedPtr Path)
{
	// This callback is called when the async path is completed
	// For simplicity, we'll store the path for all zombies that need updating
	// In a real implementation, you'd want to track which zombie requested which path

	if (Result == ENavigationQueryResult::Success && Path.IsValid())
	{
		// For now, just log that we found a path
		// In the actual movement system, you'd use this path
		UE_LOG(LogTemp, Log, TEXT("Path found with %d points"), Path->GetPathPoints().Num());
	}
}

void ASpawnerV3::SpawnZombie(int32 Index)
{
	// Initialize data
	Positions[Index] = GetRandomSpawnPoint();
	Rotations[Index] = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
	Velocities[Index] = FVector::ZeroVector;
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
