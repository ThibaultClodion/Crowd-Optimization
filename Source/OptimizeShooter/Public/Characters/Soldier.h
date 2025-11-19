// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Weapons/Gun.h"
#include "Soldier.generated.h"

class UInputMappingContext;
class UInputAction;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class OPTIMIZESHOOTER_API ASoldier : public ACharacter
{
	GENERATED_BODY()

public:
	ASoldier();

	virtual void Tick(float DeltaTime) override;

	/** Inputs **/
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void SwitchShootingState();

protected:
	virtual void BeginPlay() override;

	void InitializeMappingContext();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Weapon)
	TSubclassOf<AGun> GunClass;

	UPROPERTY()
	AGun* Gun;

	/** Inputs **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* CharacterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ShootAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Speed, meta = (UIMin = "0.0", UIMax = "1.0"))
	float MovementSpeedFactor = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Speed, meta = (UIMin = "0.0", UIMax = "1.0"))
	float RotationSpeedFactor = 0.5f;

private:
	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere)
	UCameraComponent* ViewCamera;
};
