#include "Version2/ZombieV2.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <NiagaraFunctionLibrary.h>
#include <Kismet/GameplayStatics.h>
#include <Runtime/AIModule/Classes/Tasks/AITask_RunEQS.h>
#include <Runtime/AIModule/Classes/Navigation/PathFollowingComponent.h>


AZombieV2::AZombieV2()
{
	PrimaryActorTick.bCanEverTick = false;

	if (GetMesh())
	{
		// Set Skeletal Mesh
		static ConstructorHelpers::FObjectFinder<USkeletalMesh> ActorMesh(TEXT("/Game/Assets/Zombie/SKM_Zombie.SKM_Zombie"));
		if (ActorMesh.Succeeded())
		{
			GetMesh()->SetSkeletalMesh(ActorMesh.Object);

			GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
			GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
		}

		// Set Animation Blueprint
		GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(TEXT("/Game/Versions/Version2/ABP_ZombieV2.ABP_ZombieV2_C"));
		if (AnimBP.Succeeded())
		{
			GetMesh()->SetAnimInstanceClass(AnimBP.Class);
		}
	}

	// Load Moan Sound
	static ConstructorHelpers::FObjectFinder<USoundBase> MoanSoundAsset(TEXT("/Game/Assets/Zombie/FX/Moan/MSS_Moan.MSS_Moan"));
	if (MoanSoundAsset.Succeeded())
	{
		MoanSound = MoanSoundAsset.Object;
	}

	// Load Hit Sound
	static ConstructorHelpers::FObjectFinder<USoundBase> HitSoundAsset(TEXT("/Game/Assets/Zombie/FX/Hit/MSS_HitFlesh.MSS_HitFlesh"));
	if (HitSoundAsset.Succeeded())
	{
		HitSound = HitSoundAsset.Object;
	}

	// Load Hit Effect
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitEffectAsset(TEXT("/Game/Assets/Zombie/FX/Hit/NS_Blood.NS_Blood"));
	if (HitEffectAsset.Succeeded())
	{
		HitEffect = HitEffectAsset.Object;
	}

	// Set Character Movement properties
	GetCharacterMovement()->MaxWalkSpeed = 50.f;
}

void AZombieV2::BeginPlay()
{
	Super::BeginPlay();
	
	TargetCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	AnimationInstance = Cast<UAnimInstance>(GetMesh()->GetAnimInstance());
	AIController = Cast<AAIController>(GetController());
	MoanAudioComponent = UGameplayStatics::SpawnSoundAttached(MoanSound, RootComponent);

	MoveTowardsTarget();
}

void AZombieV2::MoveTowardsTarget()
{
	AIController->MoveToActor(TargetCharacter, AcceptanceRadius);
	AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this, &AZombieV2::MoveCompleted);
}

void AZombieV2::MoveCompleted(FAIRequestID requestID, const FPathFollowingResult& result)
{
	AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	MoveTowardsTarget();
}

void AZombieV2::TakeDamage(float damage, FVector hitLocation)
{
	Health -= damage;
	HitFeedback(hitLocation);

	if(Health <= 0.f)
	{
		Die();
	}
}

void AZombieV2::HitFeedback(FVector hitLocation)
{
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), HitSound, hitLocation);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, hitLocation);
}

void AZombieV2::Die()
{
	IsDead = true;

	AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	AIController->StopMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoanAudioComponent->Stop();

	// TODO : Tell ABP to play death animation
}
