// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/BoxComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Version3/SpawnerV3.h"

ASpawnerV3::ASpawnerV3()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("Spawn Area"));
	RootComponent = SpawnArea;
}

void ASpawnerV3::BeginPlay()
{
	Super::BeginPlay();

	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World = GetWorld();
	PlayerTarget = UGameplayStatics::GetPlayerCharacter(World, 0);

	// Initialize Data Arrays
	Positions.SetNumZeroed(ToSpawnCount);
	Rotations.SetNumZeroed(ToSpawnCount);
	Velocities.SetNumZeroed(ToSpawnCount);
	Healths.SetNumZeroed(ToSpawnCount);
	DeathTimers.SetNumZeroed(ToSpawnCount);

	for(int i = 0; i < ToSpawnCount; i++)
	{
		Healths[i] = InitialHealth;
		DeathTimers[i] = -1.f; // Negative means alive
	}

	// Initialize Zombie Pool
	InitializePool();

	// Initial Spawn
	SpawnZombies();
}

void ASpawnerV3::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHealthSystem();
	UpdateDeathSystem(DeltaTime);
}

void ASpawnerV3::SetZombieCount(int32 Count)
{
	ToSpawnCount = Count;
	SpawnZombies();
}

void ASpawnerV3::HitZombieAtIndex(int32 Index, float Damage)
{
	Healths[Index] -= Damage;

	// TODO : Implement Feedback
}

void ASpawnerV3::UpdateHealthSystem()
{
	for(int i =0; i < ToSpawnCount; i++)
	{
		if(Healths[i] <= 0.f)
		{
			KillZombie(i);
		}
	}
}

void ASpawnerV3::UpdateDeathSystem(float DeltaTime)
{
	for(int i =0; i < ToSpawnCount; i++)
	{
		if(DeathTimers[i] >= 0.f)
		{
			DeathTimers[i] -= DeltaTime;

			if(DeathTimers[i] < 0.f)
			{
				ReturnActorToPool(i);
				SpawnOneZombie();
			}
		}
	}
}

void ASpawnerV3::SpawnZombies()
{
	while (AliveCount < ToSpawnCount)
	{
		SpawnOneZombie();
	}
}

void ASpawnerV3::SpawnOneZombie()
{
	AZombieV3* Zombie = GetActorFromPool();
	if (Zombie)
	{
		FTransform SpawnTransform = GetRandomPointInSpawnArea();
		Zombie->SetActorTransform(SpawnTransform);
		Zombie->SetActive(true);
		AliveCount++;
	}
}

void ASpawnerV3::KillZombie(int32 Index)
{
	Healths[Index] = MAX_flt;
	DeathTimers[Index] = RespawnDelay;
	AliveCount--;
}

FTransform ASpawnerV3::GetRandomPointInSpawnArea() const
{
	FVector Origin = SpawnArea->GetComponentLocation();
	FVector Extent = SpawnArea->GetScaledBoxExtent();

	float RandX = FMath::FRandRange(-Extent.X, Extent.X);
	float RandY = FMath::FRandRange(-Extent.Y, Extent.Y);
	float ZPosition = Origin.Z;
	FVector RandomPoint = Origin + FVector(RandX, RandY, ZPosition);

	return FTransform(RandomPoint);
}

void ASpawnerV3::InitializePool()
{
	ZombieActorPool.SetNum(ToSpawnCount);

	for (int32 i = 0; i < ToSpawnCount; i++)
	{
		AZombieV3* NewZombie = World->SpawnActor<AZombieV3>(AZombieV3::StaticClass(), FTransform::Identity, SpawnParams);
		NewZombie->SetActive(false);
		NewZombie->ZombieIndex = i;
		ZombieActorPool[i] = NewZombie;
		AvailablePoolIndices.Enqueue(i);
	}
}

AZombieV3* ASpawnerV3::GetActorFromPool()
{
	int32 PoolIndex;

	if (AvailablePoolIndices.Dequeue(PoolIndex))
	{
		return ZombieActorPool[PoolIndex];
	}

	return nullptr;
}

void ASpawnerV3::ReturnActorToPool(int32 PoolIndex)
{
	ZombieActorPool[PoolIndex]->SetActive(false);
	AvailablePoolIndices.Enqueue(PoolIndex);
}
