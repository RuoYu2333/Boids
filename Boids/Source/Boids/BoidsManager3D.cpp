// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoidsManager3D.h"

#include "BoidsFreeCameraPawn.h"
#include "BoidsHUD.h"
#include "Camera/CameraActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
FTransform MakeDirectionIndicatorTransform3D(
	const FBoidAgent3D& Agent,
	float IndicatorLength,
	float IndicatorWidth)
{
	const FVector Direction = Agent.Velocity.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return FTransform(FQuat::Identity, Agent.Position, FVector::ZeroVector);
	}

	const float SafeLength = FMath::Max(1.0f, IndicatorLength);
	const float SafeWidth = FMath::Max(1.0f, IndicatorWidth);
	const float AgentRadius = 50.0f * Agent.Scale;
	const FVector Location = Agent.Position + Direction * (AgentRadius + SafeLength * 0.5f);
	const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction);
	return FTransform(
		Rotation,
		Location,
		FVector(SafeWidth / 100.0f, SafeWidth / 100.0f, SafeLength / 100.0f));
}

FTransform MakeFishTransform3D(const FBoidAgent3D& Agent)
{
	const FVector Direction = Agent.Velocity.GetSafeNormal();
	// Quaternius 这组 OBJ 的鱼头朝本地 +Z；把该轴旋转到速度方向，鱼身就会沿轨迹前进。
	const FQuat Rotation = Direction.IsNearlyZero()
		? FQuat::Identity
		: FQuat::FindBetweenNormals(FVector::UpVector, Direction);
	return FTransform(Rotation, Agent.Position, FVector(Agent.Scale));
}
}

ABoidsManager3D::ABoidsManager3D()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	auto ConfigureInstanceComponent = [SceneRoot](UInstancedStaticMeshComponent* Component)
	{
		Component->SetupAttachment(SceneRoot);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		Component->SetCanEverAffectNavigation(false);
	};

	BlueInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BlueInstances"));
	ConfigureInstanceComponent(BlueInstances);
	OrangeInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("OrangeInstances"));
	ConfigureInstanceComponent(OrangeInstances);
	GreenInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("GreenInstances"));
	ConfigureInstanceComponent(GreenInstances);
	PurpleInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PurpleInstances"));
	ConfigureInstanceComponent(PurpleInstances);
	WhiteInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WhiteInstances"));
	ConfigureInstanceComponent(WhiteInstances);
	DirectionInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DirectionInstances"));
	ConfigureInstanceComponent(DirectionInstances);
	BoundaryFrameInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BoundaryFrameInstances"));
	ConfigureInstanceComponent(BoundaryFrameInstances);
	FoodMarkerInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FoodMarkerInstances"));
	ConfigureInstanceComponent(FoodMarkerInstances);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		FoodMarkerInstances->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Fish1Mesh(
		TEXT("/Game/Fish/Quaternius/Fish1.Fish1"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Fish2Mesh(
		TEXT("/Game/Fish/Quaternius/Fish2.Fish2"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Fish3Mesh(
		TEXT("/Game/Fish/Quaternius/Fish3.Fish3"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MantaRayMesh(
		TEXT("/Game/Fish/Quaternius/Manta_ray.Manta_ray"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SharkMesh(
		TEXT("/Game/Fish/Quaternius/Shark.Shark"));
	if (Fish1Mesh.Succeeded()) BlueInstances->SetStaticMesh(Fish1Mesh.Object);
	if (Fish2Mesh.Succeeded()) OrangeInstances->SetStaticMesh(Fish2Mesh.Object);
	if (Fish3Mesh.Succeeded()) GreenInstances->SetStaticMesh(Fish3Mesh.Object);
	if (MantaRayMesh.Succeeded()) PurpleInstances->SetStaticMesh(MantaRayMesh.Object);
	if (SharkMesh.Succeeded()) WhiteInstances->SetStaticMesh(SharkMesh.Object);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		DirectionInstances->SetStaticMesh(ConeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BoundaryFrameInstances->SetStaticMesh(CubeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMaterial.Succeeded())
	{
		BlueInstances->SetMaterial(0, BaseMaterial.Object);
		OrangeInstances->SetMaterial(0, BaseMaterial.Object);
		GreenInstances->SetMaterial(0, BaseMaterial.Object);
		PurpleInstances->SetMaterial(0, BaseMaterial.Object);
		WhiteInstances->SetMaterial(0, BaseMaterial.Object);
		DirectionInstances->SetMaterial(0, BaseMaterial.Object);
		BoundaryFrameInstances->SetMaterial(0, BaseMaterial.Object);
		FoodMarkerInstances->SetMaterial(0, BaseMaterial.Object);
	}
}

void ABoidsManager3D::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInstanceDynamic* Material = BlueInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.25f, 1.0f));
	}
	if (UMaterialInstanceDynamic* Material = OrangeInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.18f, 0.02f));
	}
	if (UMaterialInstanceDynamic* Material = GreenInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 0.85f, 0.22f));
	}
	if (UMaterialInstanceDynamic* Material = PurpleInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.55f, 0.08f, 0.9f));
	}
	if (UMaterialInstanceDynamic* Material = WhiteInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.8f, 0.85f, 1.0f));
	}
	if (UMaterialInstanceDynamic* Material = DirectionInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.8f, 0.02f));
	}
	if (UMaterialInstanceDynamic* Material = BoundaryFrameInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.8f, 1.0f));
	}
	if (UMaterialInstanceDynamic* Material = FoodMarkerInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 1.0f, 0.1f));
	}

	SpawnAgents();
	SpawnFreeCamera();
}

void ABoidsManager3D::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// BeginPlay 时 PlayerController 可能尚未创建；持续重试直到自由相机真正被占有。
	SpawnFreeCamera();

	const float SafeSimulationStep = FMath::Max(0.001f, SimulationStep);
	const int32 SafeMaxSubsteps = FMath::Clamp(MaxSubsteps, 1, 16);
	TimeAccumulator += FMath::Max(0.0f, DeltaTime);

	int32 CompletedSubsteps = 0;
	while (TimeAccumulator >= SafeSimulationStep && CompletedSubsteps < SafeMaxSubsteps)
	{
		StepSimulation(SafeSimulationStep);
		TimeAccumulator -= SafeSimulationStep;
		++CompletedSubsteps;
	}

	// 严重卡顿时丢弃整步积压，避免长时间追帧。
	if (TimeAccumulator >= SafeSimulationStep)
	{
		TimeAccumulator = FMath::Fmod(TimeAccumulator, SafeSimulationStep);
	}
}

void ABoidsManager3D::ResetSimulation()
{
	TimeAccumulator = 0.0f;
	SimulationTime = 0.0f;
	CurrentBehavior = EBoids3DBehaviorMode::Cruising;
	FoodDropSequence = 0;
	for (FBoidsFood3D& Food : Foods) Food.bActive = false;
	SpawnAgents();
}

void ABoidsManager3D::StartFeedingAtWorldLocation(FVector WorldLocation)
{
	static_cast<void>(WorldLocation);
	InitializeFoodPool();
	FBoidsFood3D* NewFood = nullptr;
	for (FBoidsFood3D& Food : Foods)
	{
		if (!Food.bActive)
		{
			NewFood = &Food;
			break;
		}
	}
	if (NewFood == nullptr)
	{
		return;
	}
	const FVector Extent = BoundaryHalfExtent.GetAbs();
	const FVector TankCenter = GetTankCenterLocal();
	// 每次投食使用独立的确定性随机流，在水箱顶面随机选择水平位置。
	FRandomStream FoodStream(HashCombine(GetTypeHash(RandomSeed), GetTypeHash(++FoodDropSequence)));
	const float Radius = FMath::Max(1.0f, FoodRadius);
	const float EdgePadding = FMath::Max(0.0f, FoodSpawnEdgePadding);
	const float SpawnHalfX = FMath::Max(0.0, Extent.X - Radius - EdgePadding);
	const float SpawnHalfY = FMath::Max(0.0, Extent.Y - Radius - EdgePadding);
	NewFood->Position = FVector(
		FoodStream.FRandRange(TankCenter.X - SpawnHalfX, TankCenter.X + SpawnHalfX),
		FoodStream.FRandRange(TankCenter.Y - SpawnHalfY, TankCenter.Y + SpawnHalfY),
		TankCenter.Z + Extent.Z - Radius);
	NewFood->StableId = FoodDropSequence;
	NewFood->bActive = true;
	CurrentBehavior = EBoids3DBehaviorMode::Feeding;
	UpdateFoodMarker();
}

void ABoidsManager3D::StopFeeding()
{
	CurrentBehavior = EBoids3DBehaviorMode::Cruising;
	for (FBoidsFood3D& Food : Foods) Food.bActive = false;
	UpdateFoodMarker();
}

void ABoidsManager3D::SpawnAgents()
{
	BlueInstances->ClearInstances();
	OrangeInstances->ClearInstances();
	GreenInstances->ClearInstances();
	PurpleInstances->ClearInstances();
	WhiteInstances->ClearInstances();
	DirectionInstances->ClearInstances();
	Agents.Reset();
	SpatialGrid.Reset();

	FRandomStream RandomStream(RandomSeed);
	const float SafeMinScale = FMath::Min(MinAgentScale, MaxAgentScale);
	const float SafeMaxScale = FMath::Max(MinAgentScale, MaxAgentScale);
	const float SafeMinInitialSpeed = FMath::Min(MinInitialSpeed, MaxInitialSpeed);
	const float SafeMaxInitialSpeed = FMath::Max(MinInitialSpeed, MaxInitialSpeed);
	const int32 SafeAgentCount = FMath::Max(1, AgentCount);
	const FVector Extent = BoundaryHalfExtent.GetAbs();
	const FVector TankCenter = GetTankCenterLocal();
	Agents.Reserve(SafeAgentCount);
	BlueInstances->PreAllocateInstancesMemory((SafeAgentCount + 1) / 2);
	OrangeInstances->PreAllocateInstancesMemory(SafeAgentCount / 2);
	GreenInstances->PreAllocateInstancesMemory(SafeAgentCount / 5);
	PurpleInstances->PreAllocateInstancesMemory(SafeAgentCount / 5);
	WhiteInstances->PreAllocateInstancesMemory(SafeAgentCount / 5);
	DirectionInstances->PreAllocateInstancesMemory(SafeAgentCount);
	DirectionInstances->SetVisibility(bShowDirectionIndicators);

	for (int32 AgentIndex = 0; AgentIndex < SafeAgentCount; ++AgentIndex)
	{
		// 五种鱼采用 34/26/18/14/8 的固定非均匀配额，并使用不同体型倍率。
		const float SpeciesAlpha = (static_cast<float>(AgentIndex) + 0.5f) / static_cast<float>(SafeAgentCount);
		const int32 GroupId = SpeciesAlpha < 0.34f ? 0 : SpeciesAlpha < 0.60f ? 1
			: SpeciesAlpha < 0.78f ? 2 : SpeciesAlpha < 0.92f ? 3 : 4;
		static constexpr float SpeciesScaleMultipliers[5] = {1.12f, 0.9f, 1.28f, 0.72f, 1.0f};
		const float Scale = FMath::Clamp(
			RandomStream.FRandRange(SafeMinScale, SafeMaxScale) * SpeciesScaleMultipliers[GroupId],
			SafeMinScale * 0.6f,
			SafeMaxScale * 1.35f);
		const float Radius = 50.0f * Scale;
		const FVector LegalHalfExtent(
			FMath::Max(0.0, Extent.X - Radius),
			FMath::Max(0.0, Extent.Y - Radius),
			FMath::Max(0.0, Extent.Z - Radius));
		// 开局在整个水箱内随机打散，之后再由状态机逐渐形成各自种群。
		const FVector Position = TankCenter + FVector(
			RandomStream.FRandRange(-LegalHalfExtent.X, LegalHalfExtent.X),
			RandomStream.FRandRange(-LegalHalfExtent.Y, LegalHalfExtent.Y),
			RandomStream.FRandRange(-LegalHalfExtent.Z, LegalHalfExtent.Z));
		const float InitialSpeed = RandomStream.FRandRange(SafeMinInitialSpeed, SafeMaxInitialSpeed)
			* GetSizeSpeedMultiplier(Scale);
		const FVector Velocity = MakeRandomUnitDirection(RandomStream) * InitialSpeed;

		UInstancedStaticMeshComponent* TargetInstances = GetInstancesForGroup(GroupId);
		const int32 InstanceIndex = TargetInstances->AddInstance(
			FTransform(FQuat::FindBetweenNormals(FVector::UpVector, Velocity.GetSafeNormal()), Position, FVector(Scale)));

		FBoidAgent3D& Agent = Agents.AddDefaulted_GetRef();
		Agent.Position = Position;
		Agent.Velocity = Velocity;
		Agent.Scale = Scale;
		Agent.GroupId = GroupId;
		Agent.StableId = AgentIndex;
		Agent.InstanceIndex = InstanceIndex;

		if (bShowDirectionIndicators)
		{
			DirectionInstances->AddInstance(MakeDirectionIndicatorTransform3D(
				Agent, DirectionIndicatorLength, DirectionIndicatorWidth));
		}
	}

	const int32 SafeStartCullDistance = FMath::Max(0, InstanceStartCullDistance);
	const int32 SafeEndCullDistance = FMath::Max(SafeStartCullDistance, InstanceEndCullDistance);
	BlueInstances->SetCullDistances(SafeStartCullDistance, SafeEndCullDistance);
	OrangeInstances->SetCullDistances(SafeStartCullDistance, SafeEndCullDistance);
	GreenInstances->SetCullDistances(SafeStartCullDistance, SafeEndCullDistance);
	PurpleInstances->SetCullDistances(SafeStartCullDistance, SafeEndCullDistance);
	WhiteInstances->SetCullDistances(SafeStartCullDistance, SafeEndCullDistance);
	DirectionInstances->SetCullDistances(SafeStartCullDistance, SafeEndCullDistance);

	RebuildBoundaryFrame();
	InitializeFoodPool();
	UpdateFoodMarker();
}

void ABoidsManager3D::StepSimulation(float FixedDeltaTime)
{
	const float SafeNeighborRadius = FMath::Max(1.0f, NeighborRadius);
	const float SafeSeparationRadius = FMath::Max(1.0f, SeparationRadius);
	const float NeighborRadiusSquared = FMath::Square(SafeNeighborRadius);
	const float CellSize = FMath::Max(1.0f, SpatialHashCellSize);
	const int32 CellRange = FMath::Max(1, FMath::CeilToInt(SafeNeighborRadius / CellSize));
	const float SafeMaxSimulationSpeed = FMath::Max(0.0f, MaxSpeed);
	const float SafeMinSimulationSpeed = FMath::Clamp(MinSpeed, 0.0f, SafeMaxSimulationSpeed);
	const FVector Extent = BoundaryHalfExtent.GetAbs();
	const FVector TankCenter = GetTankCenterLocal();
	if (CurrentBehavior == EBoids3DBehaviorMode::Feeding)
	{
		const float BottomFoodZ = TankCenter.Z - Extent.Z + FMath::Max(1.0f, FoodRadius);
		for (FBoidsFood3D& Food : Foods)
		{
			if (!Food.bActive) continue;
			Food.Position.Z = FMath::Max(
				BottomFoodZ,
				Food.Position.Z - FMath::Max(0.0f, FoodFallSpeed) * FixedDeltaTime);
		}
		UpdateFoodMarker();
	}

	BuildSpatialHash(CellSize);
	TArray<FVector> Accelerations;
	Accelerations.SetNumZeroed(Agents.Num());
	TArray<bool> AgentSeekingFood;
	AgentSeekingFood.Init(false, Agents.Num());
	FVector SpeciesCenters[5] = {};
	int32 SpeciesCounts[5] = {};
	for (const FBoidAgent3D& SpeciesAgent : Agents)
	{
		if (SpeciesAgent.GroupId >= 0 && SpeciesAgent.GroupId < 5)
		{
			SpeciesCenters[SpeciesAgent.GroupId] += SpeciesAgent.Position;
			++SpeciesCounts[SpeciesAgent.GroupId];
		}
	}
	for (int32 Species = 0; Species < 5; ++Species)
	{
		if (SpeciesCounts[Species] > 0)
		{
			SpeciesCenters[Species] /= static_cast<double>(SpeciesCounts[Species]);
		}
	}
	FVector SpeciesCruiseTargets[5] = {};
	const float CruiseRange = FMath::Clamp(GroupCruiseRangeFraction, 0.1f, 1.0f);
	const float RouteTime = SimulationTime / FMath::Max(10.0f, GroupCruisePathPeriod) * 2.0f * UE_PI;
	for (int32 Species = 0; Species < 5; ++Species)
	{
		const float Phase = static_cast<float>(Species) * (2.0f * UE_PI / 5.0f);
		const float Direction = (Species & 1) == 0 ? 1.0f : -1.0f;
		const float RouteAngle = RouteTime * Direction + Phase;
		const float LaneScale = FMath::Clamp(CruiseRange - Species * GroupCruiseLaneSpacing, 0.45f, 0.95f);
		SpeciesCruiseTargets[Species] = TankCenter + FVector(
			FMath::Cos(RouteAngle) * Extent.X * LaneScale,
			FMath::Sin(RouteAngle) * Extent.Y * LaneScale,
			FMath::Sin(RouteAngle * 0.7f + Phase) * Extent.Z * 0.35f);
	}

	for (int32 AgentIndex = 0; AgentIndex < Agents.Num(); ++AgentIndex)
	{
		const FBoidAgent3D& Agent = Agents[AgentIndex];
		const float SizeSpeedMultiplier = GetSizeSpeedMultiplier(Agent.Scale);
		const float SizeAccelerationMultiplier = GetSizeAccelerationMultiplier(Agent.Scale);
		const float AgentMaxSpeed = SafeMaxSimulationSpeed * SizeSpeedMultiplier;
		const float AgentMinSpeed = SafeMinSimulationSpeed * SizeSpeedMultiplier;
		const float AgentMaxAcceleration = MaxAcceleration * SizeAccelerationMultiplier;
		const int32 SoloSegment = FMath::FloorToInt(
			SimulationTime / FMath::Max(1.0f, SoloDecisionInterval));
		FRandomStream SoloStream(HashCombine(
			HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Agent.StableId)), GetTypeHash(SoloSegment)));
		const bool bSolo = SoloStream.FRand() < FMath::Clamp(SoloFishProbability, 0.0f, 1.0f);
		const float AgentAlignmentWeight = AlignmentWeight * (bSolo ? SoloAlignmentMultiplier : 1.0f);
		const float AgentCohesionWeight = CohesionWeight * (bSolo ? SoloCohesionMultiplier : 1.0f);
		FVector SeparationSum = FVector::ZeroVector;
		FVector NeighborDirectionSum = FVector::ZeroVector;
		FVector NeighborPositionSum = FVector::ZeroVector;
		int32 CloseNeighborCount = 0;
		int32 FlockingNeighborCount = 0;
		const FIntVector CenterCell = GetCellCoordinate(Agent.Position, CellSize);

		for (int32 ZOffset = -CellRange; ZOffset <= CellRange; ++ZOffset)
		{
			for (int32 YOffset = -CellRange; YOffset <= CellRange; ++YOffset)
			{
				for (int32 XOffset = -CellRange; XOffset <= CellRange; ++XOffset)
				{
					const FIntVector Cell = CenterCell + FIntVector(XOffset, YOffset, ZOffset);
					const TArray<int32>* Bucket = SpatialGrid.Find(Cell);
					if (Bucket == nullptr)
					{
						continue;
					}

					for (const int32 NeighborIndex : *Bucket)
					{
						if (NeighborIndex == AgentIndex)
						{
							continue;
						}

						const FBoidAgent3D& Neighbor = Agents[NeighborIndex];
						const FVector Offset = Agent.Position - Neighbor.Position;
						const float DistanceSquared = Offset.SquaredLength();
						if (Neighbor.GroupId == Agent.GroupId && DistanceSquared < NeighborRadiusSquared)
						{
							const FVector NeighborDirection = Neighbor.Velocity.GetSafeNormal();
							if (!NeighborDirection.IsNearlyZero())
							{
								NeighborDirectionSum += NeighborDirection;
							}
							NeighborPositionSum += Neighbor.Position;
							++FlockingNeighborCount;
						}

						float PersonalSpaceMultiplier =
							(Agent.GroupId == 0 || Neighbor.GroupId == 0)
							? FMath::Max(1.0f, BluePersonalSpaceMultiplier) : 1.0f;
						if (Agent.GroupId != Neighbor.GroupId)
						{
							PersonalSpaceMultiplier *= FMath::Max(1.0f, CrossSpeciesPersonalSpaceMultiplier);
						}
						const float PairSeparationRadius = SafeSeparationRadius * PersonalSpaceMultiplier;
						if (DistanceSquared < FMath::Square(PairSeparationRadius))
						{
							FVector AwayDirection;
							float Proximity = 1.0f;
							if (DistanceSquared <= UE_SMALL_NUMBER)
							{
								const int32 LowerId = FMath::Min(Agent.StableId, Neighbor.StableId);
								const int32 HigherId = FMath::Max(Agent.StableId, Neighbor.StableId);
								FRandomStream PairStream(HashCombine(GetTypeHash(LowerId), GetTypeHash(HigherId)));
								AwayDirection = MakeRandomUnitDirection(PairStream);
								if (Agent.StableId > Neighbor.StableId)
								{
									AwayDirection *= -1.0;
								}
							}
							else
							{
								const float Distance = FMath::Sqrt(DistanceSquared);
								AwayDirection = Offset / Distance;
								Proximity = 1.0f - Distance / PairSeparationRadius;
							}
							SeparationSum += AwayDirection * Proximity;
							++CloseNeighborCount;
						}
					}
				}
			}
		}

		FVector CombinedAcceleration = FVector::ZeroVector;
		if (CloseNeighborCount > 0)
		{
			CombinedAcceleration += SeparationSum / static_cast<double>(CloseNeighborCount)
				* SeparationWeight * AgentMaxAcceleration;
		}
		if (FlockingNeighborCount > 0)
		{
			const FVector AverageDirection =
				(NeighborDirectionSum / static_cast<double>(FlockingNeighborCount)).GetSafeNormal();
			const float DesiredSpeed = FMath::Clamp(
				Agent.Velocity.Length(), AgentMinSpeed, AgentMaxSpeed);
			if (!AverageDirection.IsNearlyZero())
			{
				CombinedAcceleration +=
					(AverageDirection * DesiredSpeed - Agent.Velocity) * AgentAlignmentWeight;
			}

			const FVector NeighborCenter =
				NeighborPositionSum / static_cast<double>(FlockingNeighborCount);
			const FVector ToCenter = NeighborCenter - Agent.Position;
			const float DistanceToCenter = ToCenter.Length();
			if (DistanceToCenter > UE_SMALL_NUMBER)
			{
				const float CenteringStrength =
					FMath::Clamp(DistanceToCenter / SafeNeighborRadius, 0.0f, 1.0f);
				CombinedAcceleration +=
					(ToCenter / DistanceToCenter * DesiredSpeed - Agent.Velocity)
					* AgentCohesionWeight * CenteringStrength;
			}
		}

		const FVector AgentForward = Agent.Velocity.GetSafeNormal();
		const float SightCosine = FMath::Cos(FMath::DegreesToRadians(
			FMath::Clamp(FoodSightHalfAngleDegrees, 1.0f, 179.0f)));
		const FBoidsFood3D* SelectedFood = nullptr;
		float SelectedDistanceSquared = TNumericLimits<float>::Max();
		if (CurrentBehavior == EBoids3DBehaviorMode::Feeding)
		{
			for (const FBoidsFood3D& Food : Foods)
			{
				if (!Food.bActive) continue;
				const FVector CandidateOffset = Food.Position - Agent.Position;
				const float CandidateDistanceSquared = CandidateOffset.SquaredLength();
				if (CandidateDistanceSquared <= UE_SMALL_NUMBER
					|| CandidateDistanceSquared > FMath::Square(FMath::Max(0.0f, FoodSightRadius))) continue;
				const float CandidateDistance = FMath::Sqrt(CandidateDistanceSquared);
				if (FVector::DotProduct(AgentForward, CandidateOffset / CandidateDistance) < SightCosine) continue;
				if (SelectedFood == nullptr || CandidateDistanceSquared < SelectedDistanceSquared
					|| (FMath::IsNearlyEqual(CandidateDistanceSquared, SelectedDistanceSquared)
						&& Food.StableId < SelectedFood->StableId))
				{
					SelectedFood = &Food;
					SelectedDistanceSquared = CandidateDistanceSquared;
				}
			}
		}
		const bool bCanSeeFood = SelectedFood != nullptr;
		AgentSeekingFood[AgentIndex] = bCanSeeFood;
		const FVector ToFood = bCanSeeFood ? SelectedFood->Position - Agent.Position : FVector::ZeroVector;
		const float DistanceToFood = bCanSeeFood ? FMath::Sqrt(SelectedDistanceSquared) : 0.0f;

		// 种群质心过近时，同种所有成员共享远离方向，使整个集群分流而非只推开接触个体。
		FVector SpeciesAvoidance = FVector::ZeroVector;
		FVector YieldScatter = FVector::ZeroVector;
		const float SafeSpeciesRadius = FMath::Max(1.0f, SpeciesAvoidanceRadius);
		const int32 PrioritySegment = FMath::FloorToInt(
			SimulationTime / FMath::Max(1.0f, SpeciesPriorityInterval));
		for (int32 OtherSpecies = 0; OtherSpecies < 5; ++OtherSpecies)
		{
			if (OtherSpecies == Agent.GroupId || SpeciesCounts[OtherSpecies] == 0) continue;
			const int32 LowerSpecies = FMath::Min(Agent.GroupId, OtherSpecies);
			const int32 HigherSpecies = FMath::Max(Agent.GroupId, OtherSpecies);
			FRandomStream PriorityStream(HashCombine(
				HashCombine(HashCombine(GetTypeHash(RandomSeed), GetTypeHash(LowerSpecies)), GetTypeHash(HigherSpecies)),
				GetTypeHash(PrioritySegment)));
			const int32 YieldingSpecies = PriorityStream.RandRange(0, 1) == 0 ? LowerSpecies : HigherSpecies;
			if (Agent.GroupId != YieldingSpecies) continue;
			const FVector CenterOffset = SpeciesCenters[Agent.GroupId] - SpeciesCenters[OtherSpecies];
			const float CenterDistance = CenterOffset.Length();
			if (CenterDistance > UE_SMALL_NUMBER && CenterDistance < SafeSpeciesRadius)
			{
				const FVector AwayFromOtherGroup = CenterOffset / CenterDistance;
				const float Proximity = 1.0f - CenterDistance / SafeSpeciesRadius;
				SpeciesAvoidance += AwayFromOtherGroup * Proximity;
				FRandomStream ScatterStream(HashCombine(
					HashCombine(GetTypeHash(Agent.StableId), GetTypeHash(OtherSpecies)), GetTypeHash(PrioritySegment)));
				const FVector IndividualScatter =
					(AwayFromOtherGroup + MakeRandomUnitDirection(ScatterStream) * 0.8f).GetSafeNormal();
				YieldScatter += IndividualScatter * Proximity;
			}
		}
		CombinedAcceleration += SpeciesAvoidance.GetClampedToMaxSize(1.0)
			* SpeciesAvoidanceWeight * AgentMaxAcceleration * (bCanSeeFood ? 0.2f : 1.0f);
		CombinedAcceleration += YieldScatter.GetClampedToMaxSize(1.0)
			* SpeciesYieldScatterWeight * AgentMaxAcceleration * (bCanSeeFood ? 0.2f : 1.0f);

		if (!bCanSeeFood)
		{
			const float Phase = static_cast<float>(Agent.StableId) * 1.6180339f;
			const float Time = SimulationTime * FMath::Max(0.01f, CruiseFrequency);
			const FVector CruiseDirection(
				FMath::Sin(Time + Phase),
				FMath::Sin(Time * 1.37f + Phase * 0.73f),
				FMath::Sin(Time * 0.79f + Phase * 1.19f));
			CombinedAcceleration += CruiseDirection.GetSafeNormal() * CruiseWeight
				* (bSolo ? SoloCruiseMultiplier : 1.0f) * AgentMaxAcceleration;
			const FVector ToGroupTarget = SpeciesCruiseTargets[Agent.GroupId] - Agent.Position;
			if (!ToGroupTarget.IsNearlyZero())
			{
				const FVector DesiredCruiseVelocity = ToGroupTarget.GetSafeNormal() * AgentMaxSpeed;
				CombinedAcceleration += (DesiredCruiseVelocity - Agent.Velocity) * GroupCruiseWeight
					* (bSolo ? 0.2f : 1.0f);
			}
		}
		else
		{
			if (DistanceToFood > UE_SMALL_NUMBER)
			{
				const float SlowAlpha = FMath::Clamp(
					DistanceToFood / FMath::Max(1.0f, FeedingSlowRadius), 0.0f, 1.0f);
				const float DesiredSpeed = FMath::Lerp(
					FMath::Clamp(FeedingArrivalSpeed * SizeSpeedMultiplier, 0.0f, AgentMaxSpeed),
					AgentMaxSpeed,
					SlowAlpha);
				CombinedAcceleration +=
					(ToFood / DistanceToFood * DesiredSpeed - Agent.Velocity) * FeedingWeight;
			}
		}

		const float Radius = 50.0f * Agent.Scale;
		const FVector LegalHalfExtent(
			FMath::Max(0.0, Extent.X - Radius),
			FMath::Max(0.0, Extent.Y - Radius),
			FMath::Max(0.0, Extent.Z - Radius));
		const FVector RelativePosition = Agent.Position - TankCenter;
		const float SafeBoundaryPadding = FMath::Max(1.0f, BoundaryPadding);
		FVector BoundaryDirection = FVector::ZeroVector;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const float DistanceToMin = RelativePosition[Axis] + LegalHalfExtent[Axis];
			const float DistanceToMax = LegalHalfExtent[Axis] - RelativePosition[Axis];
			if (DistanceToMin < SafeBoundaryPadding)
			{
				BoundaryDirection[Axis] +=
					1.0 - FMath::Clamp(DistanceToMin / SafeBoundaryPadding, 0.0, 1.0);
			}
			if (DistanceToMax < SafeBoundaryPadding)
			{
				BoundaryDirection[Axis] -=
					1.0 - FMath::Clamp(DistanceToMax / SafeBoundaryPadding, 0.0, 1.0);
			}
		}
		// 看见食物时降低顶面软边界的竞争力；硬边界仍确保球体不会越界。
		const float BoundaryPriority = bCanSeeFood ? 0.2f : 1.0f;
		CombinedAcceleration += BoundaryDirection.GetClampedToMaxSize(1.0)
			* BoundaryWeight * AgentMaxAcceleration * BoundaryPriority;
		Accelerations[AgentIndex] = CombinedAcceleration.GetClampedToMaxSize(AgentMaxAcceleration);
		if (bShowNeighborhoodDebug && Agent.StableId == DebugAgentStableId)
		{
			const FVector WorldPosition = GetActorTransform().TransformPosition(Agent.Position);
			DrawDebugSphere(GetWorld(), WorldPosition, SafeNeighborRadius, 24, FColor::Green, false, FixedDeltaTime * 1.5f);
			DrawDebugSphere(GetWorld(), WorldPosition, SafeSeparationRadius, 20, FColor::Red, false, FixedDeltaTime * 1.5f);
			DrawDebugLine(GetWorld(), WorldPosition,
				GetActorTransform().TransformPosition(Agent.Position + Accelerations[AgentIndex]),
				FColor::Yellow, false, FixedDeltaTime * 1.5f, 0, 3.0f);
		}
	}

	for (int32 AgentIndex = 0; AgentIndex < Agents.Num(); ++AgentIndex)
	{
		FBoidAgent3D& Agent = Agents[AgentIndex];
		Agent.Acceleration = Accelerations[AgentIndex];
		const FVector PreviousDirection = Agent.Velocity.GetSafeNormal();
		Agent.Velocity += Agent.Acceleration * FixedDeltaTime;
		const float UnclampedSpeed = Agent.Velocity.Length();
		const FVector DesiredDirection = Agent.Velocity.GetSafeNormal();
		if (!PreviousDirection.IsNearlyZero() && !DesiredDirection.IsNearlyZero() && MaxTurnRateDegrees > 0.0f)
		{
			const float TurnAngle = FMath::Acos(FMath::Clamp(
				FVector::DotProduct(PreviousDirection, DesiredDirection), -1.0, 1.0));
			const float MaxTurnAngle = FMath::DegreesToRadians(MaxTurnRateDegrees) * FixedDeltaTime;
			if (TurnAngle > MaxTurnAngle && TurnAngle > UE_SMALL_NUMBER)
			{
				const FQuat FullTurn = FQuat::FindBetweenNormals(PreviousDirection, DesiredDirection);
				const FQuat LimitedTurn = FQuat::Slerp(FQuat::Identity, FullTurn, MaxTurnAngle / TurnAngle);
				Agent.Velocity = LimitedTurn.RotateVector(PreviousDirection) * UnclampedSpeed;
			}
		}

		const float SizeSpeedMultiplier = GetSizeSpeedMultiplier(Agent.Scale);
		const float AgentMaxSpeed = SafeMaxSimulationSpeed * SizeSpeedMultiplier;
		const float AgentMinSpeed = SafeMinSimulationSpeed * SizeSpeedMultiplier;
		const float StateMinSpeed = AgentSeekingFood[AgentIndex]
			? FMath::Min(AgentMinSpeed, FeedingArrivalSpeed * SizeSpeedMultiplier)
			: AgentMinSpeed;
		const float CurrentSpeed = Agent.Velocity.Length();
		if (CurrentSpeed > UE_SMALL_NUMBER)
		{
			Agent.Velocity *= FMath::Clamp(CurrentSpeed, StateMinSpeed, AgentMaxSpeed)
				/ CurrentSpeed;
		}
		else if (StateMinSpeed > 0.0f)
		{
			FRandomStream DirectionStream(HashCombine(GetTypeHash(RandomSeed), GetTypeHash(Agent.StableId)));
			Agent.Velocity = MakeRandomUnitDirection(DirectionStream) * StateMinSpeed;
		}

		Agent.Position += Agent.Velocity * FixedDeltaTime;
		const float Radius = 50.0f * Agent.Scale;
		const FVector LegalHalfExtent(
			FMath::Max(0.0, Extent.X - Radius),
			FMath::Max(0.0, Extent.Y - Radius),
			FMath::Max(0.0, Extent.Z - Radius));
		FVector RelativePosition = Agent.Position - TankCenter;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			if (RelativePosition[Axis] > LegalHalfExtent[Axis])
			{
				RelativePosition[Axis] = LegalHalfExtent[Axis];
				if (Agent.Velocity[Axis] > 0.0)
				{
					Agent.Velocity[Axis] *= -1.0;
				}
			}
			else if (RelativePosition[Axis] < -LegalHalfExtent[Axis])
			{
				RelativePosition[Axis] = -LegalHalfExtent[Axis];
				if (Agent.Velocity[Axis] < 0.0)
				{
					Agent.Velocity[Axis] *= -1.0;
				}
			}
		}
		Agent.Position = TankCenter + RelativePosition;
	}

	ResolveAgentCollisions();
	if (CurrentBehavior == EBoids3DBehaviorMode::Feeding)
	{
		for (FBoidsFood3D& Food : Foods)
		{
			if (!Food.bActive) continue;
			bool bConsumed = false;
			for (const FBoidAgent3D& Agent : Agents)
			{
				const float ContactRadius = 50.0f * Agent.Scale + FMath::Max(1.0f, FoodRadius);
				if (FVector::DistSquared(Agent.Position, Food.Position) <= FMath::Square(ContactRadius))
				{
					bConsumed = true;
					break;
				}
			}
			if (bConsumed) Food.bActive = false;
		}
		if (!HasActiveFood()) CurrentBehavior = EBoids3DBehaviorMode::Cruising;
		UpdateFoodMarker();
	}

	SimulationTime += FixedDeltaTime;
	UpdateInstances();
}

void ABoidsManager3D::BuildSpatialHash(float CellSize)
{
	SpatialGrid.Reset();
	for (int32 AgentIndex = 0; AgentIndex < Agents.Num(); ++AgentIndex)
	{
		SpatialGrid.FindOrAdd(GetCellCoordinate(Agents[AgentIndex].Position, CellSize)).Add(AgentIndex);
	}
}

void ABoidsManager3D::ResolveAgentCollisions()
{
	const float CellSize = FMath::Max(
		FMath::Max(1.0f, SpatialHashCellSize),
		100.0f * FMath::Max(MinAgentScale, MaxAgentScale));
	const FVector Extent = BoundaryHalfExtent.GetAbs();
	const FVector TankCenter = GetTankCenterLocal();

	// 两轮位置投影能处理三个以上球体形成的接触链，同时保持成本局部化。
	for (int32 Iteration = 0; Iteration < 2; ++Iteration)
	{
		BuildSpatialHash(CellSize);
		for (int32 AgentIndex = 0; AgentIndex < Agents.Num(); ++AgentIndex)
		{
			FBoidAgent3D& Agent = Agents[AgentIndex];
			const FIntVector CenterCell = GetCellCoordinate(Agent.Position, CellSize);
			for (int32 Z = -1; Z <= 1; ++Z)
			for (int32 Y = -1; Y <= 1; ++Y)
			for (int32 X = -1; X <= 1; ++X)
			{
				const TArray<int32>* Bucket = SpatialGrid.Find(CenterCell + FIntVector(X, Y, Z));
				if (Bucket == nullptr) continue;
				for (const int32 OtherIndex : *Bucket)
				{
					if (OtherIndex <= AgentIndex) continue;
					FBoidAgent3D& Other = Agents[OtherIndex];
					FVector Offset = Other.Position - Agent.Position;
					float Distance = Offset.Length();
					const float MinimumDistance = 50.0f * (Agent.Scale + Other.Scale);
					if (Distance >= MinimumDistance) continue;
					FVector Normal;
					if (Distance <= UE_SMALL_NUMBER)
					{
						FRandomStream PairStream(HashCombine(GetTypeHash(Agent.StableId), GetTypeHash(Other.StableId)));
						Normal = MakeRandomUnitDirection(PairStream);
						Distance = 0.0f;
					}
					else Normal = Offset / Distance;
					const FVector Correction = Normal * ((MinimumDistance - Distance) * 0.5f + 0.01f);
					Agent.Position -= Correction;
					Other.Position += Correction;
					const float ClosingSpeed = FVector::DotProduct(Other.Velocity - Agent.Velocity, Normal);
					if (ClosingSpeed < 0.0f)
					{
						const FVector Impulse = Normal * (-ClosingSpeed * 0.5f);
						Agent.Velocity -= Impulse;
						Other.Velocity += Impulse;
					}
				}
			}
		}
	}

	for (FBoidAgent3D& Agent : Agents)
	{
		const float Radius = 50.0f * Agent.Scale;
		const FVector LegalExtent(
			FMath::Max(0.0, Extent.X - Radius), FMath::Max(0.0, Extent.Y - Radius), FMath::Max(0.0, Extent.Z - Radius));
		FVector Relative = Agent.Position - TankCenter;
		Relative.X = FMath::Clamp(Relative.X, -LegalExtent.X, LegalExtent.X);
		Relative.Y = FMath::Clamp(Relative.Y, -LegalExtent.Y, LegalExtent.Y);
		Relative.Z = FMath::Clamp(Relative.Z, -LegalExtent.Z, LegalExtent.Z);
		Agent.Position = TankCenter + Relative;
	}
}

FIntVector ABoidsManager3D::GetCellCoordinate(const FVector& Position, float CellSize) const
{
	return FIntVector(
		FMath::FloorToInt(Position.X / CellSize),
		FMath::FloorToInt(Position.Y / CellSize),
		FMath::FloorToInt(Position.Z / CellSize));
}

void ABoidsManager3D::UpdateInstances()
{
	for (const FBoidAgent3D& Agent : Agents)
	{
		UInstancedStaticMeshComponent* TargetInstances = GetInstancesForGroup(Agent.GroupId);
		TargetInstances->UpdateInstanceTransform(
			Agent.InstanceIndex,
			MakeFishTransform3D(Agent),
			false,
			false,
			true);

		if (bShowDirectionIndicators)
		{
			DirectionInstances->UpdateInstanceTransform(
				Agent.StableId,
				MakeDirectionIndicatorTransform3D(Agent, DirectionIndicatorLength, DirectionIndicatorWidth),
				false,
				false,
				true);
		}
	}

	BlueInstances->MarkRenderStateDirty();
	OrangeInstances->MarkRenderStateDirty();
	GreenInstances->MarkRenderStateDirty();
	PurpleInstances->MarkRenderStateDirty();
	WhiteInstances->MarkRenderStateDirty();
	if (bShowDirectionIndicators)
	{
		DirectionInstances->MarkRenderStateDirty();
	}
}

UInstancedStaticMeshComponent* ABoidsManager3D::GetInstancesForGroup(int32 GroupId) const
{
	switch (GroupId)
	{
	case 0: return BlueInstances;
	case 1: return OrangeInstances;
	case 2: return GreenInstances;
	case 3: return PurpleInstances;
	default: return WhiteInstances;
	}
}

void ABoidsManager3D::RebuildBoundaryFrame()
{
	BoundaryFrameInstances->ClearInstances();
	BoundaryFrameInstances->SetVisibility(bShowBoundaryFrame);
	if (!bShowBoundaryFrame)
	{
		return;
	}

	const FVector Extent = BoundaryHalfExtent.GetAbs();
	const FVector TankCenter = GetTankCenterLocal();
	const float Thickness = FMath::Max(1.0f, BoundaryFrameThickness);
	const FVector XEdgeScale(2.0 * Extent.X / 100.0, Thickness / 100.0, Thickness / 100.0);
	const FVector YEdgeScale(Thickness / 100.0, 2.0 * Extent.Y / 100.0, Thickness / 100.0);
	const FVector ZEdgeScale(Thickness / 100.0, Thickness / 100.0, 2.0 * Extent.Z / 100.0);

	for (int32 YSign : {-1, 1})
	{
		for (int32 ZSign : {-1, 1})
		{
			BoundaryFrameInstances->AddInstance(FTransform(
				FQuat::Identity,
				TankCenter + FVector(0.0, YSign * Extent.Y, ZSign * Extent.Z),
				XEdgeScale));
		}
	}
	for (int32 XSign : {-1, 1})
	{
		for (int32 ZSign : {-1, 1})
		{
			BoundaryFrameInstances->AddInstance(FTransform(
				FQuat::Identity,
				TankCenter + FVector(XSign * Extent.X, 0.0, ZSign * Extent.Z),
				YEdgeScale));
		}
	}
	for (int32 XSign : {-1, 1})
	{
		for (int32 YSign : {-1, 1})
		{
			BoundaryFrameInstances->AddInstance(FTransform(
				FQuat::Identity,
				TankCenter + FVector(XSign * Extent.X, YSign * Extent.Y, 0.0),
				ZEdgeScale));
		}
	}
}

void ABoidsManager3D::UpdateFoodMarker()
{
	InitializeFoodPool();
	for (int32 FoodIndex = 0; FoodIndex < Foods.Num(); ++FoodIndex)
	{
		const FBoidsFood3D& Food = Foods[FoodIndex];
		FoodMarkerInstances->UpdateInstanceTransform(FoodIndex, FTransform(
			FQuat::Identity,
			Food.Position,
			Food.bActive ? FVector(FMath::Max(1.0f, FoodRadius) / 50.0f) : FVector::ZeroVector),
			false, false, true);
	}
	FoodMarkerInstances->MarkRenderStateDirty();
}

void ABoidsManager3D::InitializeFoodPool()
{
	const int32 PoolSize = FMath::Clamp(MaxSimultaneousFood, 1, 20);
	if (Foods.Num() == PoolSize && FoodMarkerInstances->GetInstanceCount() == PoolSize) return;
	Foods.SetNum(PoolSize);
	FoodMarkerInstances->ClearInstances();
	FoodMarkerInstances->PreAllocateInstancesMemory(PoolSize);
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		Foods[Index].bActive = false;
		FoodMarkerInstances->AddInstance(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector));
	}
}

bool ABoidsManager3D::HasActiveFood() const
{
	for (const FBoidsFood3D& Food : Foods)
	{
		if (Food.bActive) return true;
	}
	return false;
}

void ABoidsManager3D::SpawnFreeCamera()
{
	if (!bSpawnFreeCamera || GetWorld() == nullptr)
	{
		return;
	}

	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (Controller == nullptr)
	{
		return;
	}
	if (!Controller->GetHUD() || !Controller->GetHUD()->IsA<ABoidsHUD>())
	{
		Controller->ClientSetHUD(ABoidsHUD::StaticClass());
	}
	if (SpawnedFreeCamera.IsValid())
	{
		if (Controller->GetPawn() != SpawnedFreeCamera.Get())
		{
			Controller->Possess(SpawnedFreeCamera.Get());
		}
		if (ACameraActor* ControlledCamera = SpawnedFreeCamera->GetControlledCamera())
		{
			Controller->SetViewTarget(ControlledCamera);
		}
		return;
	}

	const FVector Extent = BoundaryHalfExtent.GetAbs();
	const FVector LocalCenter = GetTankCenterLocal();
	const FVector LocalCameraPosition(-Extent.X * 1.8, -Extent.Y * 0.35, LocalCenter.Z + Extent.Z * 0.25);
	FVector WorldCameraPosition = GetActorTransform().TransformPosition(LocalCameraPosition);
	const FVector WorldCenter = GetActorTransform().TransformPosition(LocalCenter);
	FRotator WorldCameraRotation = (WorldCenter - WorldCameraPosition).Rotation();
	ACameraActor* LevelCamera = nullptr;
	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		LevelCamera = *It;
		WorldCameraPosition = LevelCamera->GetActorLocation();
		WorldCameraRotation = LevelCamera->GetActorRotation();
		break;
	}
	SpawnedFreeCamera = GetWorld()->SpawnActor<ABoidsFreeCameraPawn>(
		ABoidsFreeCameraPawn::StaticClass(), WorldCameraPosition, WorldCameraRotation);
	if (SpawnedFreeCamera.IsValid())
	{
		SpawnedFreeCamera->SetControlledCamera(LevelCamera);
		Controller->Possess(SpawnedFreeCamera.Get());
		if (LevelCamera != nullptr)
		{
			Controller->SetViewTarget(LevelCamera);
		}
		Controller->SetInputMode(FInputModeGameOnly());
		Controller->SetShowMouseCursor(false);
		if (GEngine != nullptr)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				8.0f,
				FColor::Cyan,
				TEXT("Boids free camera: WASD move, Q/E vertical, RMB look, F drop food"));
		}
	}
}

FVector ABoidsManager3D::GetTankCenterLocal() const
{
	// 水箱底面固定在 Manager 局部 Z=0；改变高度只向上扩展，不会穿入地面。
	return FVector(0.0, 0.0, BoundaryHalfExtent.GetAbs().Z);
}

float ABoidsManager3D::GetSizeSpeedMultiplier(float Scale) const
{
	const float ReferenceScale = FMath::Max(0.05f, (MinAgentScale + MaxAgentScale) * 0.5f);
	const float RawMultiplier = FMath::Pow(
		ReferenceScale / FMath::Max(0.05f, Scale), FMath::Max(0.0f, SizeSpeedExponent));
	return FMath::Clamp(RawMultiplier,
		FMath::Min(MinSizeMovementMultiplier, MaxSizeMovementMultiplier),
		FMath::Max(MinSizeMovementMultiplier, MaxSizeMovementMultiplier));
}

float ABoidsManager3D::GetSizeAccelerationMultiplier(float Scale) const
{
	const float ReferenceScale = FMath::Max(0.05f, (MinAgentScale + MaxAgentScale) * 0.5f);
	const float RawMultiplier = FMath::Pow(
		ReferenceScale / FMath::Max(0.05f, Scale), FMath::Max(0.0f, SizeAccelerationExponent));
	return FMath::Clamp(RawMultiplier,
		FMath::Min(MinSizeMovementMultiplier, MaxSizeMovementMultiplier),
		FMath::Max(MinSizeMovementMultiplier, MaxSizeMovementMultiplier));
}

FVector ABoidsManager3D::MakeRandomUnitDirection(FRandomStream& RandomStream)
{
	// Z 与方位角均匀采样，得到无极点偏置的单位球面方向。
	const float Z = RandomStream.FRandRange(-1.0f, 1.0f);
	const float Azimuth = RandomStream.FRandRange(0.0f, 2.0f * UE_PI);
	const float RadialLength = FMath::Sqrt(FMath::Max(0.0f, 1.0f - Z * Z));
	return FVector(
		RadialLength * FMath::Cos(Azimuth),
		RadialLength * FMath::Sin(Azimuth),
		Z);
}
