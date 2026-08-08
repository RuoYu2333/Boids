// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoidsHUD.h"

#include "BoidsManager3D.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

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

	for (TActorIterator<ABoidsManager3D> It(GetWorld()); It; ++It)
	{
		TArray<FString> PerformanceLines;
		It->GetPerformanceOverlayLines(PerformanceLines);
		float Y = 25.0f;
		for (int32 LineIndex = 0; LineIndex < PerformanceLines.Num(); ++LineIndex)
		{
			Canvas->SetDrawColor(LineIndex == 0 ? FColor(100, 220, 255) : FColor::White);
			Canvas->DrawText(Font, PerformanceLines[LineIndex], 30.0f, Y, 0.85f, 0.85f, FFontRenderInfo());
			Y += 24.0f;
		}
		break;
	}
}
