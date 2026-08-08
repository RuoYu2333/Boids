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
	UFont* TimerFont = GEngine->GetMediumFont();
	UFont* PerformanceFont = GEngine->GetLargeFont();
	auto DrawShadowedText = [this](UFont* Font, const FString& Text, float X, float Y, float Scale, const FColor& Color)
	{
		// 阴影只向文字正下方延伸，多层透明度形成柔和的底部落影，不产生横向投影。
		Canvas->SetDrawColor(FColor(0, 0, 0, 180));
		Canvas->DrawText(Font, Text, X, Y + 2.0f, Scale, Scale, FFontRenderInfo());
		Canvas->SetDrawColor(FColor(0, 0, 0, 110));
		Canvas->DrawText(Font, Text, X, Y + 4.0f, Scale, Scale, FFontRenderInfo());
		Canvas->SetDrawColor(FColor(0, 0, 0, 55));
		Canvas->DrawText(Font, Text, X, Y + 6.0f, Scale, Scale, FFontRenderInfo());
		Canvas->SetDrawColor(Color);
		Canvas->DrawText(Font, Text, X, Y, Scale, Scale, FFontRenderInfo());
	};

	constexpr float TimerScale = 1.15f;
	float TextWidth = 0.0f;
	float TextHeight = 0.0f;
	Canvas->StrLen(TimerFont, TimerText, TextWidth, TextHeight);
	DrawShadowedText(TimerFont, TimerText,
		Canvas->SizeX - TextWidth * TimerScale - 35.0f, 30.0f,
		TimerScale, FColor(80, 255, 120));

	for (TActorIterator<ABoidsManager3D> It(GetWorld()); It; ++It)
	{
		TArray<FString> PerformanceLines;
		It->GetPerformanceOverlayLines(PerformanceLines);
		float Y = 30.0f;
		for (int32 LineIndex = 0; LineIndex < PerformanceLines.Num(); ++LineIndex)
		{
			const FString& Line = PerformanceLines[LineIndex];
			FColor LineColor = FColor(90, 255, 120);
			if (LineIndex == 0 || Line.StartsWith(TEXT("Baseline"))
				|| (Line.StartsWith(TEXT("Testing")) && Line.Contains(TEXT("interval: 1"))))
			{
				LineColor = FColor(255, 70, 70);
			}
			DrawShadowedText(PerformanceFont, Line, 35.0f, Y, 0.95f, LineColor);
			Y += 42.0f;
		}
		break;
	}
}
