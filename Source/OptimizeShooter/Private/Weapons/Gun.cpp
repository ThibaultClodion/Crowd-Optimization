// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Gun.h"

AGun::AGun()
{
	PrimaryActorTick.bCanEverTick = true;

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	RootComponent = GunMesh;

	MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	MuzzleLocation->SetupAttachment(GunMesh);
}

void AGun::BeginPlay()
{
	Super::BeginPlay();	
}

void AGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsShooting)
	{
		TimeSinceLastShot += DeltaTime;

		if (TimeSinceLastShot >= FireRate)
		{
			ShootFeedback();
			Shoot();
			TimeSinceLastShot = 0.0f;
		}
	}
}

void AGun::SwitchShootState()
{
	IsShooting = !IsShooting;
	TimeSinceLastShot = FireRate; // Allow immediate shooting when toggled
}
