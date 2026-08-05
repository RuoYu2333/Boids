// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BoidsHUD.generated.h"

UCLASS()
class BOIDS_API ABoidsHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	double StartTimeSeconds = -1.0;
};
