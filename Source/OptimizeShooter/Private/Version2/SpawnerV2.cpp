// Fill out your copyright notice in the Description page of Project Settings.

#include "Version2/SpawnerV2.h"
#include "Components/BoxComponent.h"

ASpawnerV2::ASpawnerV2()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("Spawn Area"));
	RootComponent = SpawnArea;
}

void ASpawnerV2::BeginPlay()
{
	Super::BeginPlay();

	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	World = GetWorld();
	SpawnZombies();
}

void ASpawnerV2::ChangeToSpawnCount(int32 NewCount)
{
	ToSpawnCount = NewCount;
	SpawnZombies();
}

void ASpawnerV2::SpawnZombies()
{
	while (AliveCount < ToSpawnCount)
	{
		SpawnOneZombie();
	}
}

void ASpawnerV2::SpawnOneZombie()
{
	AZombieV2* Zombie = World->SpawnActor<AZombieV2>(AZombieV2::StaticClass(), GetRandomPointInSpawnArea(), SpawnParams);
	Zombie->OnDied.AddDynamic(this, &ASpawnerV2::OnZombieDied);
	AliveCount++;
}

FTransform ASpawnerV2::GetRandomPointInSpawnArea() const
{
	FVector Origin = SpawnArea->GetComponentLocation();
	FVector Extent = SpawnArea->GetScaledBoxExtent();

	float RandX = FMath::FRandRange(-Extent.X, Extent.X);
	float RandY = FMath::FRandRange(-Extent.Y, Extent.Y);
	float ZPosition = Origin.Z;
	FVector RandomPoint = Origin + FVector(RandX, RandY, ZPosition);

	return FTransform(RandomPoint);
}

void ASpawnerV2::OnZombieDied(AZombieV2* DeadZombie)
{
	AliveCount--;

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateUObject(this, &ASpawnerV2::DestroyZombie, DeadZombie), 5.0f, false);
}

void ASpawnerV2::DestroyZombie(AZombieV2* DeadZombie)
{
	DeadZombie->Destroy();
	SpawnOneZombie();
}

