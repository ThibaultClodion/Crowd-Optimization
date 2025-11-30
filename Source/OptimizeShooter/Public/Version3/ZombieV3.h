// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombieV3.generated.h"

class UCapsuleComponent;


UCLASS()
class OPTIMIZESHOOTER_API AZombieV3 : public AActor
{
    GENERATED_BODY()

public:
    AZombieV3();

    UPROPERTY(VisibleAnywhere)
    USkeletalMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere)
    UCapsuleComponent* CapsuleComponent;

    int32 ZombieIndex = -1;

    void SetTransform(FVector Position, FRotator Rotation);
    void SetActive(bool bIsActive);
};