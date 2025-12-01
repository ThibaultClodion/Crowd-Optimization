// Fill out your copyright notice in the Description page of Project Settings.

#include "Version3/SpawnerV3.h"
#include "Components/BoxComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Version3/ZombieV3.h"
#include <NiagaraFunctionLibrary.h>

ASpawnerV3::ASpawnerV3()
{
	PrimaryActorTick.bCanEverTick = true;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("Spawn Area"));
	RootComponent = SpawnArea;

	// Load Moan Sound
	static ConstructorHelpers::FObjectFinder<USoundBase> MoanSoundAsset(TEXT("/Game/Assets/Zombie/FX/Moan/MSS_Moan2.MSS_Moan2"));
	if (MoanSoundAsset.Succeeded())
	{
		MoanSound = MoanSoundAsset.Object;
	}

	// Load Hit Sound
	static ConstructorHelpers::FObjectFinder<USoundBase> HitSoundAsset(TEXT("/Game/Assets/Zombie/FX/Hit/MSS_HitFlesh.MSS_HitFlesh"));
	if (HitSoundAsset.Succeeded())
	{
		HitSound = HitSoundAsset.Object;
	}

	// Load Hit Effect
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitEffectAsset(TEXT("/Game/Assets/Zombie/FX/Hit/NS_Blood.NS_Blood"));
	if (HitEffectAsset.Succeeded())
	{
		HitEffect = HitEffectAsset.Object;
	}
}

void ASpawnerV3::BeginPlay()
{
	Super::BeginPlay();

	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World = GetWorld();
	PlayerTarget = UGameplayStatics::GetPlayerCharacter(World, 0);

	// Initialize Data Arrays
	//Positions.SetNumZeroed(ToSpawnCount);
	//Rotations.SetNumZeroed(ToSpawnCount);
	//Velocities.SetNumZeroed(ToSpawnCount);
	Healths.SetNumZeroed(ToSpawnCount);
	DeathTimers.SetNumZeroed(ToSpawnCount);
	MoanTimers.SetNumZeroed(ToSpawnCount);

	for(int i = 0; i < ToSpawnCount; i++)
	{
		Healths[i] = InitialHealth;
	}
	for(int i = 0; i < ToSpawnCount; i++)
	{
		DeathTimers[i] = -1.f;
	}
	for(int i = 0; i < ToSpawnCount; i++)
	{
		MoanTimers[i] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
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
	UpdateMoanSystem(DeltaTime);
}

void ASpawnerV3::SetZombieCount(int32 Count)
{
	ToSpawnCount = Count;
	SpawnZombies();
}

void ASpawnerV3::HitZombieAtIndex(int32 Index, float Damage, FVector HitLocation)
{
	Healths[Index] -= Damage;

	// Play Hit Feedback
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), HitSound, HitLocation);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, HitLocation);
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

void ASpawnerV3::UpdateMoanSystem(float DeltaTime)
{
	for(int i =0; i < ToSpawnCount; i++)
	{
		MoanTimers[i] -= DeltaTime;

		if (MoanTimers[i] <= 0.f)
		{
			UGameplayStatics::SpawnSoundAtLocation(GetWorld(), MoanSound, ZombieActorPool[i]->GetActorLocation());
			MoanTimers[i] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
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

		// Initialize data
		MoanTimers[Zombie->ZombieIndex] = FMath::FRandRange(MinMoanInterval, MaxMoanInterval);
		Healths[Zombie->ZombieIndex] = InitialHealth;
		DeathTimers[Zombie->ZombieIndex] = -1.f;

		AliveCount++;
	}
}

void ASpawnerV3::KillZombie(int32 Index)
{
	ZombieActorPool[Index]->OnSwitchAlive.Broadcast();
	ZombieActorPool[Index]->SetCollisionEnabled(false);

	MoanTimers[Index] = MAX_flt;
	Healths[Index] = InitialHealth;
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
	ZombieActorPool[PoolIndex]->OnSwitchAlive.Broadcast();

	AvailablePoolIndices.Enqueue(PoolIndex);
}
