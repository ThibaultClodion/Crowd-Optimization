// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigationPath.h"
#include "Version3/PathFindingSystem.h"
#include "SpawnerV3.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class AZombieV3;
class UNavigationSystemV1;

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
	TArray<FVector> Positions;
	TArray<FRotator> Rotations;
	TArray<float> Healths;
	TArray<float> DeathTimers;
	TArray<float> MoanTimers;
	TBitArray<> IsAlive;
	TArray<int32> AliveIndices;

	UPROPERTY()
	TArray<AZombieV3*> ZombieActorPool;

	// Parameters
	int32 MaxZombieCount = 100;
	int32 CurrentZombieCount = 0;
	float MovementSpeed = 50.f;
	float RotationSpeed = 5.f;
	float InitialHealth = 20.f;
	float RespawnDelay = 5.f;
	float MinMoanInterval = 5.f;
	float MaxMoanInterval = 20.f;

	// Pathfinding system
	UNavigationSystemV1* NavSystem;
	ANavigationData* NavData;
	FPathFindingSystem PathFindingSystem;
	int32 MaxPathRequestsPerFrame = 10;
	float PathUpdateInterval = 0.5f;
	float MaxPathDistance = 5000.0f;
	float WaypointReachedDistance = 50.f;

	// FX
	USoundBase* MoanSound;
	USoundBase* HitSound;
	UNiagaraSystem* HitEffect;

	// References
	UPROPERTY()
	ACharacter* PlayerTarget;
	UWorld* World;
	FActorSpawnParameters SpawnParams;

	// Systems
	void UpdateMovementSystem(float DeltaTime);
	void UpdateHealthSystem();
	void UpdateDeathSystem(float DeltaTime);
	void UpdateMoanSystem(float DeltaTime);
	void SyncActorTransforms();

	// Helpers
	void InitializeData();
	void InitializePool();
	void SpawnZombie(int32 Index);
	void KillZombie(int32 Index);
	FVector GetRandomSpawnPoint() const;
};
