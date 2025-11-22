// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieV2.generated.h"

class UAnimInstance;
class USoundBase;
class UAudioComponent;
class UNiagaraSystem;
class AAIController;
struct FAIRequestID;
struct FPathFollowingResult;

UCLASS()
class OPTIMIZESHOOTER_API AZombieV2 : public ACharacter
{
	GENERATED_BODY()

public:
	AZombieV2();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
	void DieAnimation();

	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	UAnimInstance* AnimationInstance;

	UPROPERTY(EditAnywhere, Category = "SFX")
	USoundBase* MoanSound;

	UPROPERTY(EditAnywhere, Category = "SFX")
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* HitEffect;

private:

	// *** Movement *** //
	void MoveTowardsTarget();
	void MoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);

	//*** Hits & Death *** //
	UFUNCTION(BlueprintCallable)
	void TakeDamage(float Damage, FVector HitLocation);
	void HitFeedback(FVector HitLocation);
	void Die();

	// *** Config *** //
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AcceptanceRadius = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	float Health = 20.f;

	bool IsDead = false;

	// *** References *** //
	ACharacter* TargetCharacter = nullptr;
	AAIController* AIController = nullptr;
	UAudioComponent* MoanAudioComponent = nullptr;
};
