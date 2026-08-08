// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoidsFreeCameraPawn.h"

#include "BoidsManager3D.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "InputCoreTypes.h"

ABoidsFreeCameraPawn::ABoidsFreeCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
}

void ABoidsFreeCameraPawn::SetControlledCamera(ACameraActor* InCameraActor)
{
	ControlledCamera = InCameraActor;
	if (InCameraActor != nullptr)
	{
		// 保留关卡 CameraActor 的镜头参数和世界变换，只让它随自由 Pawn 一起移动。
		InCameraActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		Camera->Deactivate();
	}
}

void ABoidsFreeCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("BoidsMoveForward"), this, &ABoidsFreeCameraPawn::SetForwardInput);
	PlayerInputComponent->BindAxis(TEXT("BoidsMoveRight"), this, &ABoidsFreeCameraPawn::SetRightInput);
	PlayerInputComponent->BindAxis(TEXT("BoidsMoveVertical"), this, &ABoidsFreeCameraPawn::SetVerticalInput);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &ABoidsFreeCameraPawn::FastPressed);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &ABoidsFreeCameraPawn::FastReleased);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ABoidsFreeCameraPawn::LookPressed);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ABoidsFreeCameraPawn::LookReleased);
	PlayerInputComponent->BindAxisKey(EKeys::MouseX, this, &ABoidsFreeCameraPawn::LookYaw);
	PlayerInputComponent->BindAxisKey(EKeys::MouseY, this, &ABoidsFreeCameraPawn::LookPitch);
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &ABoidsFreeCameraPawn::DropFood);
	PlayerInputComponent->BindKey(EKeys::B, IE_Pressed, this, &ABoidsFreeCameraPawn::StartPerformanceBenchmark);
}

void ABoidsFreeCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const FVector Movement = GetActorForwardVector() * ForwardInput
		+ GetActorRightVector() * RightInput + FVector::UpVector * VerticalInput;
	const float Multiplier = bFastMove ? FastMoveMultiplier : 1.0f;
	SetActorLocation(GetActorLocation() + Movement.GetClampedToMaxSize(1.0) * MoveSpeed * Multiplier * DeltaTime);
}

void ABoidsFreeCameraPawn::SetForwardInput(float Value) { ForwardInput = Value; }
void ABoidsFreeCameraPawn::SetRightInput(float Value) { RightInput = Value; }
void ABoidsFreeCameraPawn::SetVerticalInput(float Value) { VerticalInput = Value; }
void ABoidsFreeCameraPawn::FastPressed() { bFastMove = true; }
void ABoidsFreeCameraPawn::FastReleased() { bFastMove = false; }
void ABoidsFreeCameraPawn::LookPressed() { bLooking = true; }
void ABoidsFreeCameraPawn::LookReleased() { bLooking = false; }

void ABoidsFreeCameraPawn::LookYaw(float Value)
{
	if (bLooking) AddActorWorldRotation(FRotator(0.0f, Value * LookSensitivity, 0.0f));
}

void ABoidsFreeCameraPawn::LookPitch(float Value)
{
	if (!bLooking) return;
	FRotator Rotation = GetActorRotation();
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch - Value * LookSensitivity, -89.0f, 89.0f);
	Rotation.Roll = 0.0f;
	SetActorRotation(Rotation);
}

void ABoidsFreeCameraPawn::DropFood()
{
	for (TActorIterator<ABoidsManager3D> It(GetWorld()); It; ++It)
	{
		It->StartFeedingAtWorldLocation(FVector::ZeroVector);
		break;
	}
}

void ABoidsFreeCameraPawn::StartPerformanceBenchmark()
{
	for (TActorIterator<ABoidsManager3D> It(GetWorld()); It; ++It)
	{
		It->StartVisualPerformanceBenchmark();
		break;
	}
}
