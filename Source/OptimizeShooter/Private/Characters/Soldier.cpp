// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/Soldier.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

ASoldier::ASoldier()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(CameraBoom);
}

void ASoldier::BeginPlay()
{
	Super::BeginPlay();

	InitializeMappingContext();

	if (GunClass)
	{
		Gun = GetWorld()->SpawnActor<AGun>(GunClass);
		if (Gun)
		{
			Gun->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("LeftHandSocket"));
		}
	}
}

void ASoldier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASoldier::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASoldier::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASoldier::Look);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ASoldier::SwitchShootingState);
	}
}

void ASoldier::InitializeMappingContext()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);

	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(CharacterMappingContext, 0);
		}
	}
}

void ASoldier::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>() * MovementSpeedFactor;

	if (Controller)
	{
		const FVector ForwardDirection = GetActorForwardVector();
		AddMovementInput(ForwardDirection, MovementVector.Y);

		const FVector RightDirection = GetActorRightVector();
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASoldier::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>() * RotationSpeedFactor;

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASoldier::SwitchShootingState()
{
	if (Gun)
	{
		Gun->SwitchShootState();
	}
}

