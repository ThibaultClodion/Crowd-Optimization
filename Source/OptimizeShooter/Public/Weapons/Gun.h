// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gun.generated.h"

class UStaticMeshComponent;
class UForceFeedbackEffect;
class ASpawnerV3;

UCLASS()
class OPTIMIZESHOOTER_API AGun : public AActor
{
	GENERATED_BODY()
	
public:	
	AGun();
	virtual void Tick(float DeltaTime) override;

	void SwitchShootState();

	UFUNCTION(BlueprintImplementableEvent)
	void ShootFeedback();

	UFUNCTION(BlueprintImplementableEvent)
	void Shoot();

	void Fire();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UStaticMeshComponent* GunMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	USceneComponent* MuzzleLocation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun")
	float FireRate = 0.09f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun")
	float Damage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gun")
	float Range = 1000.0f;


private:
	ASpawnerV3* Spawner;

	bool IsShooting = false;
	float TimeSinceLastShot = 0.0f;
};
