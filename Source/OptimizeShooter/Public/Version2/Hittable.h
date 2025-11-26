// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Hittable.generated.h"

UINTERFACE(MinimalAPI, Meta = (CannotImplementInterfaceInBlueprint))
class UHittable : public UInterface
{
	GENERATED_BODY()
};

class OPTIMIZESHOOTER_API IHittable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual void OnHit(float Damage, FVector hitLocation) = 0;
};
