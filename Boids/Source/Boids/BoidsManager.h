// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoidsManager.generated.h"

class UInstancedStaticMeshComponent;

USTRUCT()
struct FBoidAgent2D
{
	GENERATED_BODY()

	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Velocity = FVector2D::ZeroVector;
	FVector2D Acceleration = FVector2D::ZeroVector;
	float Scale = 1.0f;
	int32 GroupId = 0;
	int32 StableId = INDEX_NONE;
	int32 InstanceIndex = INDEX_NONE;
};

UCLASS()
class BOIDS_API ABoidsManager : public AActor
{
	GENERATED_BODY()

public:
	ABoidsManager();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Boids")
	void ResetSimulation();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 AgentCount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Spawn")
	int32 RandomSeed = 12345;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Bounds", meta = (ClampMin = "100.0"))
	float BoundaryHalfWidth = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Bounds", meta = (ClampMin = "100.0"))
	float BoundaryHalfHeight = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Spawn", meta = (ClampMin = "0.05"))
	float MinAgentScale = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Spawn", meta = (ClampMin = "0.05"))
	float MaxAgentScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Movement", meta = (ClampMin = "0.0"))
	float MinInitialSpeed = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Movement", meta = (ClampMin = "0.0"))
	float MaxInitialSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Separation", meta = (ClampMin = "1.0"))
	float SeparationRadius = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Separation", meta = (ClampMin = "0.0"))
	float SeparationWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Alignment", meta = (ClampMin = "1.0"))
	float NeighborRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Alignment", meta = (ClampMin = "0.0"))
	float AlignmentWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Cohesion", meta = (ClampMin = "0.0"))
	float CohesionWeight = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Movement", meta = (ClampMin = "0.0"))
	float MaxAcceleration = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Movement", meta = (ClampMin = "0.0"))
	float MinSpeed = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Bounds", meta = (ClampMin = "1.0"))
	float BoundaryPadding = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Bounds", meta = (ClampMin = "0.0"))
	float BoundaryWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Debug")
	bool bShowDirectionIndicators = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Debug", meta = (ClampMin = "1.0"))
	float DirectionIndicatorLength = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids|Debug", meta = (ClampMin = "1.0"))
	float DirectionIndicatorWidth = 12.0f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Boids|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> BlueInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> OrangeInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> DirectionInstances;

	TArray<FBoidAgent2D> Agents;

	void SpawnAgents();
	void StepSimulation(float DeltaTime);
	void UpdateInstances();
};
