// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoidsManager.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
FTransform MakeDirectionIndicatorTransform2D(
	const FBoidAgent2D& Agent,
	float IndicatorLength,
	float IndicatorWidth)
{
	const FVector Direction(Agent.Velocity.GetSafeNormal(), 0.0f);
	if (Direction.IsNearlyZero())
	{
		return FTransform(FQuat::Identity, FVector(Agent.Position, 0.0f), FVector::ZeroVector);
	}

	const float SafeLength = FMath::Max(1.0f, IndicatorLength);
	const float SafeWidth = FMath::Max(1.0f, IndicatorWidth);
	const float AgentRadius = 50.0f * Agent.Scale;
	const FVector Location = FVector(Agent.Position, 0.0f)
		+ Direction * (AgentRadius + SafeLength * 0.5f);
	const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Direction);
	return FTransform(
		Rotation,
		Location,
		FVector(SafeWidth / 100.0f, SafeWidth / 100.0f, SafeLength / 100.0f));
}
}

ABoidsManager::ABoidsManager()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BlueInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BlueInstances"));
	BlueInstances->SetupAttachment(SceneRoot);
	BlueInstances->SetMobility(EComponentMobility::Movable);

	OrangeInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("OrangeInstances"));
	OrangeInstances->SetupAttachment(SceneRoot);
	OrangeInstances->SetMobility(EComponentMobility::Movable);

	DirectionInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DirectionInstances"));
	DirectionInstances->SetupAttachment(SceneRoot);
	DirectionInstances->SetMobility(EComponentMobility::Movable);
	DirectionInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DirectionInstances->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		BlueInstances->SetStaticMesh(SphereMesh.Object);
		OrangeInstances->SetStaticMesh(SphereMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(
		TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		DirectionInstances->SetStaticMesh(ConeMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BaseMaterial.Succeeded())
	{
		// 构造阶段只保存基础材质引用，避免 CDO 持有不可序列化的动态材质实例。
		BlueInstances->SetMaterial(0, BaseMaterial.Object);
		OrangeInstances->SetMaterial(0, BaseMaterial.Object);
		DirectionInstances->SetMaterial(0, BaseMaterial.Object);
	}
}

void ABoidsManager::BeginPlay()
{
	Super::BeginPlay();

	// 动态材质属于实际运行中的组件，应在 BeginPlay 创建而不是在构造函数中创建。
	if (UMaterialInstanceDynamic* BlueMaterial = BlueInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		BlueMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.25f, 1.0f));
	}
	if (UMaterialInstanceDynamic* OrangeMaterial = OrangeInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		OrangeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.18f, 0.02f));
	}
	if (UMaterialInstanceDynamic* DirectionMaterial = DirectionInstances->CreateAndSetMaterialInstanceDynamic(0))
	{
		DirectionMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.8f, 0.02f));
	}

	SpawnAgents();
}

void ABoidsManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	StepSimulation(DeltaTime);
}

void ABoidsManager::ResetSimulation()
{
	SpawnAgents();
}

void ABoidsManager::SpawnAgents()
{
	BlueInstances->ClearInstances();
	OrangeInstances->ClearInstances();
	DirectionInstances->ClearInstances();
	Agents.Reset();

	// 固定种子使每次 Play 和 Reset 都得到相同初态，便于复现和测试。
	FRandomStream RandomStream(RandomSeed);
	const float SafeMinScale = FMath::Min(MinAgentScale, MaxAgentScale);
	const float SafeMaxScale = FMath::Max(MinAgentScale, MaxAgentScale);
	const float SafeMinSpeed = FMath::Min(MinInitialSpeed, MaxInitialSpeed);
	const float SafeMaxSpeed = FMath::Max(MinInitialSpeed, MaxInitialSpeed);
	const int32 SafeAgentCount = FMath::Max(1, AgentCount);

	Agents.Reserve(SafeAgentCount);
	BlueInstances->PreAllocateInstancesMemory((SafeAgentCount + 1) / 2);
	OrangeInstances->PreAllocateInstancesMemory(SafeAgentCount / 2);
	DirectionInstances->PreAllocateInstancesMemory(SafeAgentCount);
	DirectionInstances->SetVisibility(bShowDirectionIndicators);

	for (int32 AgentIndex = 0; AgentIndex < SafeAgentCount; ++AgentIndex)
	{
		const float Scale = RandomStream.FRandRange(SafeMinScale, SafeMaxScale);
		const FVector2D Position(
			RandomStream.FRandRange(-BoundaryHalfWidth, BoundaryHalfWidth),
			RandomStream.FRandRange(-BoundaryHalfHeight, BoundaryHalfHeight));
		const float DirectionAngle = RandomStream.FRandRange(0.0f, 2.0f * UE_PI);
		const float InitialSpeed = RandomStream.FRandRange(SafeMinSpeed, SafeMaxSpeed);
		const FVector2D Velocity(
			FMath::Cos(DirectionAngle) * InitialSpeed,
			FMath::Sin(DirectionAngle) * InitialSpeed);

		const FTransform InstanceTransform(
			FQuat::Identity,
			FVector(Position.X, Position.Y, 0.0f),
			FVector(Scale));

		// 偶数和奇数索引稳定分到两个颜色组，后续三力只作用于同组邻居。
		const int32 GroupId = AgentIndex % 2;
		UInstancedStaticMeshComponent* TargetInstances = GroupId == 0 ? BlueInstances : OrangeInstances;
		const int32 InstanceIndex = TargetInstances->AddInstance(InstanceTransform);

		FBoidAgent2D& Agent = Agents.AddDefaulted_GetRef();
		Agent.Position = Position;
		Agent.Velocity = Velocity;
		Agent.Scale = Scale;
		Agent.GroupId = GroupId;
		Agent.StableId = AgentIndex;
		Agent.InstanceIndex = InstanceIndex;
		DirectionInstances->AddInstance(MakeDirectionIndicatorTransform2D(
			Agent, DirectionIndicatorLength, DirectionIndicatorWidth));
	}
}

void ABoidsManager::StepSimulation(float DeltaTime)
{
	TArray<FVector2D> Accelerations;
	Accelerations.SetNumZeroed(Agents.Num());

	const float SafeSeparationRadius = FMath::Max(1.0f, SeparationRadius);
	const float SeparationRadiusSquared = FMath::Square(SafeSeparationRadius);
	const float SafeNeighborRadius = FMath::Max(1.0f, NeighborRadius);
	const float NeighborRadiusSquared = FMath::Square(SafeNeighborRadius);
	const float SafeMaxSimulationSpeed = FMath::Max(0.0f, MaxSpeed);
	const float SafeMinSimulationSpeed = FMath::Clamp(MinSpeed, 0.0f, SafeMaxSimulationSpeed);

	// 第一遍只读取帧初状态，确保代理的数组顺序不会影响同一帧的受力结果。
	for (int32 AgentIndex = 0; AgentIndex < Agents.Num(); ++AgentIndex)
	{
		const FBoidAgent2D& Agent = Agents[AgentIndex];
		FVector2D SeparationSum = FVector2D::ZeroVector;
		FVector2D NeighborDirectionSum = FVector2D::ZeroVector;
		FVector2D NeighborPositionSum = FVector2D::ZeroVector;
		int32 CloseNeighborCount = 0;
		int32 FlockingNeighborCount = 0;

		for (int32 NeighborIndex = 0; NeighborIndex < Agents.Num(); ++NeighborIndex)
		{
			if (AgentIndex == NeighborIndex)
			{
				continue;
			}

			const FBoidAgent2D& Neighbor = Agents[NeighborIndex];
			if (Agent.GroupId != Neighbor.GroupId)
			{
				continue;
			}

			const FVector2D Offset = Agent.Position - Neighbor.Position;
			const float DistanceSquared = Offset.SquaredLength();

			// 只平均方向，避免相反速度相互抵消后把整个群体减速到零。
			if (DistanceSquared < NeighborRadiusSquared)
			{
				const FVector2D NeighborDirection = Neighbor.Velocity.GetSafeNormal();
				if (!NeighborDirection.IsNearlyZero())
				{
					NeighborDirectionSum += NeighborDirection;
				}
				NeighborPositionSum += Neighbor.Position;
				++FlockingNeighborCount;
			}

			if (DistanceSquared < SeparationRadiusSquared)
			{
				FVector2D AwayDirection;
				float Proximity = 1.0f;
				if (DistanceSquared <= UE_SMALL_NUMBER)
				{
					// 完全重叠时用稳定 ID 生成成对相反的方向，避免除零和不可复现随机数。
					const int32 LowerId = FMath::Min(Agent.StableId, Neighbor.StableId);
					const int32 HigherId = FMath::Max(Agent.StableId, Neighbor.StableId);
					const uint32 PairHash = HashCombine(GetTypeHash(LowerId), GetTypeHash(HigherId));
					const float Angle = static_cast<float>(PairHash % 3600u) * (2.0f * UE_PI / 3600.0f);
					AwayDirection = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
					if (Agent.StableId > Neighbor.StableId)
					{
						AwayDirection *= -1.0f;
					}
				}
				else
				{
					const float Distance = FMath::Sqrt(DistanceSquared);
					AwayDirection = Offset / Distance;
					Proximity = 1.0f - Distance / SafeSeparationRadius;
				}

				SeparationSum += AwayDirection * Proximity;
				++CloseNeighborCount;
			}
		}

		FVector2D CombinedAcceleration = FVector2D::ZeroVector;
		if (CloseNeighborCount > 0)
		{
			const FVector2D SeparationDirection = SeparationSum / static_cast<float>(CloseNeighborCount);
			CombinedAcceleration += SeparationDirection * SeparationWeight * MaxAcceleration;
		}

		if (FlockingNeighborCount > 0)
		{
			const FVector2D AverageDirection =
				(NeighborDirectionSum / static_cast<float>(FlockingNeighborCount)).GetSafeNormal();
			if (!AverageDirection.IsNearlyZero())
			{
				const float DesiredSpeed = FMath::Clamp(
					Agent.Velocity.Length(), SafeMinSimulationSpeed, SafeMaxSimulationSpeed);
				const FVector2D DesiredVelocity = AverageDirection * DesiredSpeed;
				CombinedAcceleration += (DesiredVelocity - Agent.Velocity) * AlignmentWeight;
			}

			// Cohesion 指向同组邻居中心，接近中心后逐渐减弱，避免持续过冲。
			const FVector2D NeighborCenter =
				NeighborPositionSum / static_cast<float>(FlockingNeighborCount);
			const FVector2D ToCenter = NeighborCenter - Agent.Position;
			const float DistanceToCenter = ToCenter.Length();
			if (DistanceToCenter > UE_SMALL_NUMBER)
			{
				const float DesiredSpeed = FMath::Clamp(
					Agent.Velocity.Length(), SafeMinSimulationSpeed, SafeMaxSimulationSpeed);
				const FVector2D DesiredVelocity = ToCenter / DistanceToCenter * DesiredSpeed;
				const float CenteringStrength =
					FMath::Clamp(DistanceToCenter / SafeNeighborRadius, 0.0f, 1.0f);
				CombinedAcceleration +=
					(DesiredVelocity - Agent.Velocity) * CohesionWeight * CenteringStrength;
			}
		}

		// 在真正撞墙前逐渐转向内部，硬反弹只作为最后兜底。
		const float Radius = 50.0f * Agent.Scale;
		const float MaxX = FMath::Max(0.0f, BoundaryHalfWidth - Radius);
		const float MaxY = FMath::Max(0.0f, BoundaryHalfHeight - Radius);
		const float SafeBoundaryPadding = FMath::Max(1.0f, BoundaryPadding);
		FVector2D BoundaryDirection = FVector2D::ZeroVector;

		const float DistanceToLeft = Agent.Position.X + MaxX;
		const float DistanceToRight = MaxX - Agent.Position.X;
		const float DistanceToBottom = Agent.Position.Y + MaxY;
		const float DistanceToTop = MaxY - Agent.Position.Y;

		if (DistanceToLeft < SafeBoundaryPadding)
		{
			BoundaryDirection.X += 1.0f - FMath::Clamp(DistanceToLeft / SafeBoundaryPadding, 0.0f, 1.0f);
		}
		if (DistanceToRight < SafeBoundaryPadding)
		{
			BoundaryDirection.X -= 1.0f - FMath::Clamp(DistanceToRight / SafeBoundaryPadding, 0.0f, 1.0f);
		}
		if (DistanceToBottom < SafeBoundaryPadding)
		{
			BoundaryDirection.Y += 1.0f - FMath::Clamp(DistanceToBottom / SafeBoundaryPadding, 0.0f, 1.0f);
		}
		if (DistanceToTop < SafeBoundaryPadding)
		{
			BoundaryDirection.Y -= 1.0f - FMath::Clamp(DistanceToTop / SafeBoundaryPadding, 0.0f, 1.0f);
		}

		CombinedAcceleration += BoundaryDirection.GetClampedToMaxSize(1.0f)
			* BoundaryWeight * MaxAcceleration;

		Accelerations[AgentIndex] = CombinedAcceleration.GetClampedToMaxSize(MaxAcceleration);
	}

	// 第二遍统一积分，并执行计入球半径的边界反弹。
	for (int32 AgentIndex = 0; AgentIndex < Agents.Num(); ++AgentIndex)
	{
		FBoidAgent2D& Agent = Agents[AgentIndex];
		Agent.Acceleration = Accelerations[AgentIndex];
		Agent.Velocity += Agent.Acceleration * DeltaTime;

		const float CurrentSpeed = Agent.Velocity.Length();
		if (CurrentSpeed > UE_SMALL_NUMBER)
		{
			Agent.Velocity *= FMath::Clamp(
				CurrentSpeed, SafeMinSimulationSpeed, SafeMaxSimulationSpeed) / CurrentSpeed;
		}
		else if (SafeMinSimulationSpeed > 0.0f)
		{
			// 极少数速度完全抵消的情况，用稳定 ID 恢复一个可复现方向。
			const uint32 DirectionHash = GetTypeHash(Agent.StableId);
			const float Angle = static_cast<float>(DirectionHash % 3600u) * (2.0f * UE_PI / 3600.0f);
			Agent.Velocity = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * SafeMinSimulationSpeed;
		}
		Agent.Position += Agent.Velocity * DeltaTime;

		const float Radius = 50.0f * Agent.Scale;
		const float MaxX = FMath::Max(0.0f, BoundaryHalfWidth - Radius);
		const float MaxY = FMath::Max(0.0f, BoundaryHalfHeight - Radius);

		if (Agent.Position.X > MaxX)
		{
			Agent.Position.X = MaxX;
			if (Agent.Velocity.X > 0.0f)
			{
				Agent.Velocity.X *= -1.0f;
			}
		}
		else if (Agent.Position.X < -MaxX)
		{
			Agent.Position.X = -MaxX;
			if (Agent.Velocity.X < 0.0f)
			{
				Agent.Velocity.X *= -1.0f;
			}
		}

		if (Agent.Position.Y > MaxY)
		{
			Agent.Position.Y = MaxY;
			if (Agent.Velocity.Y > 0.0f)
			{
				Agent.Velocity.Y *= -1.0f;
			}
		}
		else if (Agent.Position.Y < -MaxY)
		{
			Agent.Position.Y = -MaxY;
			if (Agent.Velocity.Y < 0.0f)
			{
				Agent.Velocity.Y *= -1.0f;
			}
		}
	}

	UpdateInstances();
}

void ABoidsManager::UpdateInstances()
{
	for (const FBoidAgent2D& Agent : Agents)
	{
		const FTransform InstanceTransform(
			FQuat::Identity,
			FVector(Agent.Position.X, Agent.Position.Y, 0.0f),
			FVector(Agent.Scale));

		UInstancedStaticMeshComponent* TargetInstances = Agent.GroupId == 0 ? BlueInstances : OrangeInstances;
		TargetInstances->UpdateInstanceTransform(
			Agent.InstanceIndex,
			InstanceTransform,
			false,
			false,
			true);

		DirectionInstances->UpdateInstanceTransform(
			Agent.StableId,
			MakeDirectionIndicatorTransform2D(Agent, DirectionIndicatorLength, DirectionIndicatorWidth),
			false,
			false,
			true);
	}

	// 循环内只修改实例数据，最后各通知渲染线程一次，避免每条鱼都触发刷新。
	BlueInstances->MarkRenderStateDirty();
	OrangeInstances->MarkRenderStateDirty();
	DirectionInstances->MarkRenderStateDirty();
}
