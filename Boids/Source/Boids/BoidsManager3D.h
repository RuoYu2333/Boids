// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BoidsManager3D.generated.h"

class UInstancedStaticMeshComponent;
class ABoidsFreeCameraPawn;

UENUM(BlueprintType)
enum class EBoids3DBehaviorMode : uint8
{
	Cruising,
	Feeding
};

USTRUCT()
struct FBoidAgent3D
{
	GENERATED_BODY()

	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;
	bool bSeekingFood = false;
	float Scale = 1.0f;
	int32 GroupId = 0;
	int32 StableId = INDEX_NONE;
	int32 InstanceIndex = INDEX_NONE;
};

struct FBoidsFood3D
{
	FVector Position = FVector::ZeroVector;
	int32 StableId = INDEX_NONE;
	bool bActive = false;
};

UCLASS()
class BOIDS_API ABoidsManager3D : public AActor
{
	GENERATED_BODY()

public:
	ABoidsManager3D();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Boids3D")
	void ResetSimulation();

	UFUNCTION(BlueprintCallable, Category = "Boids3D|Feeding")
	void StartFeedingAtWorldLocation(FVector WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Boids3D|Feeding")
	void StopFeeding();

	UFUNCTION(BlueprintCallable, Category = "Boids3D|Performance")
	void StartVisualPerformanceBenchmark();

	void GetPerformanceOverlayLines(TArray<FString>& OutLines) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boids3D|Behavior")
	EBoids3DBehaviorMode CurrentBehavior = EBoids3DBehaviorMode::Cruising;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Spawn", meta = (ClampMin = "1", UIMin = "1"))
	int32 AgentCount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Spawn")
	int32 RandomSeed = 12345;

	// Manager 的局部原点定义为水箱底面中心；中心高度随 HalfExtent.Z 自动变化。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Bounds")
	FVector BoundaryHalfExtent = FVector(2000.0, 1200.0, 800.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Spawn", meta = (ClampMin = "0.05"))
	float MinAgentScale = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Spawn", meta = (ClampMin = "0.05"))
	float MaxAgentScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0"))
	float MinInitialSpeed = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0"))
	float MaxInitialSpeed = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0"))
	float MinSpeed = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0"))
	float MaxSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0"))
	float MaxAcceleration = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0", ClampMax = "1080.0"))
	float MaxTurnRateDegrees = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0"))
	float SizeSpeedExponent = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.0"))
	float SizeAccelerationExponent = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.1"))
	float MinSizeMovementMultiplier = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Movement", meta = (ClampMin = "0.1"))
	float MaxSizeMovementMultiplier = 1.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "1.0"))
	float NeighborRadius = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "1.0"))
	float SeparationRadius = 170.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "0.0"))
	float SeparationWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "0.0"))
	float AlignmentWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "0.0"))
	float CohesionWeight = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Schooling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SoloFishProbability = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Schooling", meta = (ClampMin = "1.0"))
	float SoloDecisionInterval = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Schooling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SoloAlignmentMultiplier = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Schooling", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SoloCohesionMultiplier = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Schooling", meta = (ClampMin = "1.0"))
	float SoloCruiseMultiplier = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "1.0"))
	float SpeciesAvoidanceRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "0.0"))
	float SpeciesAvoidanceWeight = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "0.0"))
	float SpeciesYieldScatterWeight = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "1.0"))
	float SpeciesPriorityInterval = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "1.0"))
	float BluePersonalSpaceMultiplier = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Flocking", meta = (ClampMin = "1.0"))
	float CrossSpeciesPersonalSpaceMultiplier = 1.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Cruising", meta = (ClampMin = "0.0"))
	float CruiseWeight = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Cruising", meta = (ClampMin = "0.01"))
	float CruiseFrequency = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Cruising", meta = (ClampMin = "0.0"))
	float GroupCruiseWeight = 2.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Cruising", meta = (ClampMin = "1.0"))
	float GroupCruiseTargetInterval = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Cruising", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float GroupCruiseRangeFraction = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Cruising", meta = (ClampMin = "10.0"))
	float GroupCruisePathPeriod = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Cruising", meta = (ClampMin = "0.0", ClampMax = "0.15"))
	float GroupCruiseLaneSpacing = 0.055f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "0.0"))
	float FeedingWeight = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "1.0"))
	float FeedingSlowRadius = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "0.0"))
	float FeedingArrivalSpeed = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "0.0"))
	float FoodSightRadius = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "1.0", ClampMax = "179.0"))
	float FoodSightHalfAngleDegrees = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "1", ClampMax = "20"))
	int32 MaxSimultaneousFood = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "0.0"))
	float FoodFallSpeed = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "0.0"))
	float FoodSpawnEdgePadding = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Feeding", meta = (ClampMin = "1.0"))
	float FoodRadius = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Camera")
	bool bSpawnFreeCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Bounds", meta = (ClampMin = "1.0"))
	float BoundaryPadding = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Bounds", meta = (ClampMin = "0.0"))
	float BoundaryWeight = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|SpatialHash", meta = (ClampMin = "1.0"))
	float SpatialHashCellSize = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Simulation", meta = (ClampMin = "0.001"))
	float SimulationStep = 1.0f / 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Simulation", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxSubsteps = 4;

	// 受力计算按 StableId 固定分桶；1 表示每个固定步都重算，2/4 对应高低配分级。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Simulation", meta = (ClampMin = "1", ClampMax = "8"))
	int32 ForceUpdateInterval = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Performance", meta = (ClampMin = "1"))
	int32 VisualBenchmarkWarmupSteps = 120;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Performance", meta = (ClampMin = "60"))
	int32 VisualBenchmarkMeasureSteps = 600;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Rendering", meta = (ClampMin = "0"))
	int32 InstanceStartCullDistance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Rendering", meta = (ClampMin = "0"))
	int32 InstanceEndCullDistance = 12000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Debug")
	bool bShowDirectionIndicators = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Debug", meta = (ClampMin = "1.0"))
	float DirectionIndicatorLength = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Debug", meta = (ClampMin = "1.0"))
	float DirectionIndicatorWidth = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Debug")
	bool bShowBoundaryFrame = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Debug", meta = (ClampMin = "1.0"))
	float BoundaryFrameThickness = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Rendering", meta = (ClampMin = "1.0"))
	float FishDisplayRadius = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Debug")
	bool bShowNeighborhoodDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boids3D|Debug", meta = (ClampMin = "0"))
	int32 DebugAgentStableId = 0;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> BlueInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> OrangeInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> GreenInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> PurpleInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> WhiteInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> DirectionInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> BoundaryFrameInstances;

	UPROPERTY(VisibleAnywhere, Category = "Boids3D|Rendering")
	TObjectPtr<UInstancedStaticMeshComponent> FoodMarkerInstances;

	TArray<FBoidAgent3D> Agents;
	TMap<FIntVector, TArray<int32>> SpatialGrid;
	TArray<FBoidsFood3D> Foods;
	TArray<float> FishMeshScaleMultipliers;
	float TimeAccumulator = 0.0f;
	float SimulationTime = 0.0f;
	uint64 ForceEvaluationStep = 0;
	double LiveAverageStepMilliseconds = 0.0;
	double VisualBenchmarkResults[3] = {-1.0, -1.0, -1.0};
	double VisualBenchmarkAccumulatedMilliseconds = 0.0;
	int32 VisualBenchmarkModeIndex = INDEX_NONE;
	int32 VisualBenchmarkPhaseSteps = 0;
	int32 VisualBenchmarkOriginalInterval = 2;
	bool bVisualBenchmarkActive = false;
	bool bVisualBenchmarkWarmingUp = false;
	int32 FoodDropSequence = 0;
	TWeakObjectPtr<ABoidsFreeCameraPawn> SpawnedFreeCamera;

	void SpawnAgents();
	void RunCommandLineBenchmark(int32 BenchmarkSteps, int32 WarmupSteps);
	void BeginVisualBenchmarkMode(int32 ModeIndex);
	void RecordVisualBenchmarkStep(double StepMilliseconds);
	void StepSimulation(float FixedDeltaTime);
	void BuildSpatialHash(float CellSize);
	FIntVector GetCellCoordinate(const FVector& Position, float CellSize) const;
	void UpdateInstances();
	void RebuildBoundaryFrame();
	void UpdateFoodMarker();
	void InitializeFoodPool();
	bool HasActiveFood() const;
	void SpawnFreeCamera();
	void ResolveAgentCollisions();
	UInstancedStaticMeshComponent* GetInstancesForGroup(int32 GroupId) const;
	void RefreshFishMeshScaleMultipliers();
	float GetFishMeshScaleMultiplier(int32 GroupId) const;
	FVector GetTankCenterLocal() const;
	float GetSizeSpeedMultiplier(float Scale) const;
	float GetSizeAccelerationMultiplier(float Scale) const;
	static FVector MakeRandomUnitDirection(FRandomStream& RandomStream);
};
