// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/CapsuleComponent.h"
#include "Version3/ZombieV3.h"

AZombieV3::AZombieV3()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create Capsule Component
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(34.f, 88.f);
	CapsuleComponent->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	CapsuleComponent->SetCollisionProfileName(TEXT("Zombie"));
	RootComponent = CapsuleComponent;

	// Create Mesh Component
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ActorMesh(TEXT("/Game/Assets/Zombie/SKM_Zombie.SKM_Zombie"));
	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));

	if (ActorMesh.Succeeded())
	{
		MeshComponent->SetSkeletalMesh(ActorMesh.Object);
		MeshComponent->SetupAttachment(RootComponent);
		MeshComponent->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
		MeshComponent->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

		// Set Animation Blueprint
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Versions/Version2/ABP_ZombieV2.ABP_ZombieV2_C"));
		if (AnimBP.Succeeded())
		{
			MeshComponent->SetAnimInstanceClass(AnimBP.Class);
		}
	}
}

void AZombieV3::SetTransform(FVector Position, FRotator Rotation)
{
	SetActorLocationAndRotation(Position, Rotation);
}

void AZombieV3::SetActive(bool bIsActive)
{
	SetActorHiddenInGame(!bIsActive);
	SetActorEnableCollision(bIsActive);
}
