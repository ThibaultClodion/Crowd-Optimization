// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnerV3.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class AZombieV3;

UCLASS()
class OPTIMIZESHOOTER_API ASpawnerV3 : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpawnerV3();
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetZombieCount(int32 Count);

	void HitZombieAtIndex(int32 Index, float Damage, FVector HitLocation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Components")
	UBoxComponent* SpawnArea;

private:
	// Zombie data arrays
	//TArray<FVector> Positions;
	//TArray<FRotator> Rotations;
	//TArray<FVector> Velocities;
	TArray<float> Healths;
	TArray<float> DeathTimers;
	float InitialHealth = 20.f;
	float RespawnDelay = 5.f;

	// Zombie pool
	UPROPERTY()
	TArray<AZombieV3*> ZombieActorPool;
	TQueue<int32> AvailablePoolIndices;

	// Spawning parameters
	int32 ToSpawnCount = 10;
	int32 AliveCount = 0;

	// FX
	//USoundBase* MoanSound;
	USoundBase* HitSound;
	UNiagaraSystem* HitEffect;

	// References
	UPROPERTY()
	ACharacter* PlayerTarget;
	UWorld* World;
	FActorSpawnParameters SpawnParams;

	// Systems
	//void UpdateMovementSystem(float DeltaTime);
	void UpdateHealthSystem();
	void UpdateDeathSystem(float DeltaTime);

	// Spawner functions
	void SpawnZombies();
	void SpawnOneZombie();
	void KillZombie(int32 Index);
	FTransform GetRandomPointInSpawnArea() const;

	// Pooling functions
	void InitializePool();
	AZombieV3* GetActorFromPool();
	void ReturnActorToPool(int32 PoolIndex);
};
