// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BoidsFreeCameraPawn.generated.h"

class UCameraComponent;
class ACameraActor;

UCLASS()
class BOIDS_API ABoidsFreeCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	ABoidsFreeCameraPawn();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	void SetControlledCamera(ACameraActor* InCameraActor);
	ACameraActor* GetControlledCamera() const { return ControlledCamera.Get(); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids3D|Camera")
	float MoveSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids3D|Camera")
	float FastMoveMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boids3D|Camera")
	float LookSensitivity = 0.12f;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<ACameraActor> ControlledCamera;

	float ForwardInput = 0.0f;
	float RightInput = 0.0f;
	float VerticalInput = 0.0f;
	bool bFastMove = false;
	bool bLooking = false;

	void SetForwardInput(float Value);
	void SetRightInput(float Value);
	void SetVerticalInput(float Value);
	void FastPressed();
	void FastReleased();
	void LookPressed();
	void LookReleased();
	void LookYaw(float Value);
	void LookPitch(float Value);
	void DropFood();
	void StartPerformanceBenchmark();
};
