#include "Version2/ZombieV2.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "AIController.h"
#include <NiagaraFunctionLibrary.h>
#include <Kismet/GameplayStatics.h>
#include <Runtime/AIModule/Classes/Tasks/AITask_RunEQS.h>
#include <Runtime/AIModule/Classes/Navigation/PathFollowingComponent.h>


AZombieV2::AZombieV2()
{
	PrimaryActorTick.bCanEverTick = false;
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

void AZombieV2::MoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	MoveTowardsTarget();
}

void AZombieV2::TakeDamage(float Damage, FVector HitLocation)
{
	Health -= Damage;
	HitFeedback(HitLocation);

	if(Health <= 0.f)
	{
		Die();
	}
}

void AZombieV2::HitFeedback(FVector HitLocation)
{
	UGameplayStatics::SpawnSoundAtLocation(GetWorld(), HitSound, HitLocation);
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitEffect, HitLocation);
}

void AZombieV2::Die()
{
	IsDead = true;

	AIController->GetPathFollowingComponent()->OnRequestFinished.RemoveAll(this);
	AIController->StopMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoanAudioComponent->Stop();

	DieAnimation();
}
