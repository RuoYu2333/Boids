// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoidsHUD.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void ABoidsHUD::DrawHUD()
{
	Super::DrawHUD();
	if (Canvas == nullptr || GetWorld() == nullptr || GEngine == nullptr) return;
	if (StartTimeSeconds < 0.0) StartTimeSeconds = GetWorld()->GetTimeSeconds();

	const int32 ElapsedSeconds = FMath::Max(0, FMath::FloorToInt(GetWorld()->GetTimeSeconds() - StartTimeSeconds));
	const FString TimerText = FString::Printf(TEXT("Time  %02d:%02d"), ElapsedSeconds / 60, ElapsedSeconds % 60);
	UFont* Font = GEngine->GetMediumFont();
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(Font, TimerText, TextWidth, TextHeight);
	Canvas->SetDrawColor(FColor::White);
	Canvas->DrawText(Font, TimerText, Canvas->SizeX - TextWidth - 30.0f, 25.0f, 1.0f, 1.0f, FFontRenderInfo());
}
