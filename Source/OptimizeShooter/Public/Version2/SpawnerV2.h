// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieV2.h"
#include "SpawnerV2.generated.h"

class UBoxComponent;
class UWorld;

UCLASS()
class OPTIMIZESHOOTER_API ASpawnerV2 : public AActor
{
	GENERATED_BODY()
	
public:	
	ASpawnerV2();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void ChangeToSpawnCount(int32 NewCount);

	UPROPERTY(EditAnywhere, Category = "Components")
	UBoxComponent* SpawnArea;

private:
	void SpawnZombies();
	void SpawnOneZombie();
	FTransform GetRandomPointInSpawnArea() const;

	UFUNCTION()
	void OnZombieDied(AZombieV2* DeadZombie);
	void DestroyZombie(AZombieV2* DeadZombie);

	UWorld* World;
	FActorSpawnParameters SpawnParams;
	int32 ToSpawnCount = 0;
	int32 AliveCount = 0;
};
