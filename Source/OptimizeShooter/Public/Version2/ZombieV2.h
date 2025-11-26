// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Version2/Hittable.h"
#include "ZombieV2.generated.h"

class UAnimInstance;
class USoundBase;
class UAudioComponent;
class UNiagaraSystem;
class AAIController;
struct FAIRequestID;
struct FPathFollowingResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZombieDied, AZombieV2*, DeadZombie);

UCLASS()
class OPTIMIZESHOOTER_API AZombieV2 : public ACharacter, public IHittable
{
	GENERATED_BODY()

public:
	AZombieV2();

	UPROPERTY(BlueprintAssignable, Category = "Zombie")
	FOnZombieDied OnDied;

protected:
	virtual void BeginPlay() override;

private:

	// *** Movement *** //
	void MoveTowardsTarget();
	void MoveCompleted(FAIRequestID requestID, const FPathFollowingResult& result);

	//*** Hits & Death *** //
	UFUNCTION(BlueprintCallable)
	void OnHit(float damage, FVector hitLocation) override;
	void HitFeedback(FVector hitLocation);
	void Die();

	// *** References *** //
	UAnimInstance* AnimationInstance;
	ACharacter* TargetCharacter = nullptr;
	AAIController* AIController = nullptr;
	UAudioComponent* MoanAudioComponent = nullptr;

	// *** Config *** //
	float AcceptanceRadius = 5.f;
	float Health = 20.f;
	bool IsDead = false;

	// *** FX *** //
	USoundBase* MoanSound;
	USoundBase* HitSound;
	UNiagaraSystem* HitEffect;
};
