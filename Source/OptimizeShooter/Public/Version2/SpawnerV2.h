// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieV2.h"
#include "SpawnerV2.generated.h"

class UBoxComponent;

UCLASS()
class OPTIMIZESHOOTER_API ASpawnerV2 : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpawnerV2();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Components")
	UBoxComponent* SpawnArea;

private:
	void SpawnZombies();
	void SpawnOneZombie();
	FTransform GetRandomPointInSpawnArea() const;

	UFUNCTION()
	void OnZombieDied(AZombieV2* DeadZombie);
	void RespawnZombie(AZombieV2* DeadZombie);
	FTimerHandle RespawnTimerHandle;
	float RespawnDelay = 5.f;

	FActorSpawnParameters SpawnParams;
	int32 ToSpawnCount = 60;
	int32 AliveCount = 0;
};
