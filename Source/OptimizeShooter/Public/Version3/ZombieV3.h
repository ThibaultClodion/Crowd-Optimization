// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieV3.generated.h"

class UCapsuleComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSwitchAlive);

UCLASS()
class OPTIMIZESHOOTER_API AZombieV3 : public AActor
{
    GENERATED_BODY()

public:
    AZombieV3();

    UPROPERTY(BlueprintAssignable)
    FOnSwitchAlive OnSwitchAlive;

    UPROPERTY(VisibleAnywhere)
    USkeletalMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere)
    UCapsuleComponent* CapsuleComponent;

    int32 ZombieIndex = -1;

    void SetActive(bool bIsActive);
	void SetCollisionEnabled(bool bEnabled);
};