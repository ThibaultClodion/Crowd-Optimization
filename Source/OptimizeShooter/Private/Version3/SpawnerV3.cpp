// Fill out your copyright notice in the Description page of Project Settings.

#include "Version3/SpawnerV3.h"
#include "Components/BoxComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Version3/ZombieV3.h"
#include <NiagaraFunctionLibrary.h>
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

	// Initialize arrays
	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		Positions[i] = FVector::ZeroVector;
		Rotations[i] = FRotator::ZeroRotator;
		Velocities[i] = FVector::ZeroVector;
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
	// TODO : May need a refacto (might not working)
	CurrentZombieCount = FMath::Clamp(Count, 0, MaxZombieCount);
	RebuildAliveIndices();
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
	// TODO : Might not work (pathfinding ...)

	if (!PlayerTarget) return;

	const FVector PlayerPos = PlayerTarget->GetActorLocation();

	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];

		FVector Direction = (PlayerPos - Positions[Index]).GetSafeNormal();
		Direction.Z = 0.f;

		Velocities[Index] = Direction * MovementSpeed;
		Positions[Index] += Velocities[Index] * DeltaTime;

		if (!Direction.IsNearlyZero())
		{
			Rotations[Index] = Direction.Rotation();
		}
	}
}

void ASpawnerV3::UpdateHealthSystem()
{
	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];

		if (Healths[Index] <= 0.f)
		{
			KillZombie(Index);
			i--;
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

void ASpawnerV3::RebuildAliveIndices()
{
	AliveIndices.Empty();

	for (int32 i = 0; i < MaxZombieCount; i++)
	{
		if (IsAlive[i])
		{
			AliveIndices.Add(i);
		}
	}
}
