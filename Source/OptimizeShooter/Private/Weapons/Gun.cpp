// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapons/Gun.h"
#include "Version3/SpawnerV3.h"
#include "Version3/ZombieV3.h"
#include <Kismet/GameplayStatics.h>

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

	Spawner = Cast<ASpawnerV3>(UGameplayStatics::GetActorOfClass(GetWorld(), ASpawnerV3::StaticClass()));
}

void AGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsShooting)
	{
		TimeSinceLastShot += DeltaTime;

		if (TimeSinceLastShot >= FireRate)
		{
			TimeSinceLastShot = 0.0f;
			Fire();

			// V1 & V2 shooting logic removed for V3
			// ShootFeedback();
			// Shoot();
		}
	}
}

void AGun::SwitchShootState()
{
	IsShooting = !IsShooting;
	TimeSinceLastShot = FireRate; // Allow immediate shooting when toggled
}

void AGun::Fire()
{
	ShootFeedback();

	FHitResult HitResult;

	FVector StartLocation = MuzzleLocation->GetComponentLocation();
	FVector EndLocation = StartLocation + (MuzzleLocation->GetForwardVector() * Range);
	GetWorld()->LineTraceSingleByProfile(HitResult, StartLocation, EndLocation, TEXT("Pawn"));

	if (HitResult.bBlockingHit)
	{
		AZombieV3* HitZombie = Cast<AZombieV3>(HitResult.GetActor());

		if (HitZombie)
		{
			Spawner->HitZombieAtIndex(HitZombie->ZombieIndex, Damage, HitResult.ImpactPoint);
		}
	}
}
