// Fill out your copyright notice in the Description page of Project Settings.

#include "Version3/PathFindingSystem.h"
#include "NavigationSystem.h"

FPathFindingSystem::FPathFindingSystem()
	: World(nullptr)
	, NavSystem(nullptr)
	, NavData(nullptr)
	, MaxPathRequestsPerFrame(10)    // Process 10 paths per frame by default
	, PathUpdateInterval(1.0f)       // Update paths every second
	, MaxPathDistance(5000.0f)       // Don't calculate paths beyond 50m
{
}

void FPathFindingSystem::Initialize(UWorld* InWorld, int32 MaxZombies)
{
	World = InWorld;
	NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	NavData = NavSystem->GetDefaultNavDataInstance();
	NavAgentProperties = NavData->GetConfig().DefaultProperties;

	// Allocate arrays
	CachedPaths.SetNum(MaxZombies);
	CurrentWaypointIndices.SetNum(MaxZombies);
	PathUpdateTimers.SetNum(MaxZombies);
	PendingQueryIDs.SetNum(MaxZombies);

	// Initialize arrays
	for (int32 i = 0; i < MaxZombies; i++)
	{
		CachedPaths[i] = nullptr;
		CurrentWaypointIndices[i] = 0;
		PathUpdateTimers[i] = FMath::FRandRange(0.0f, PathUpdateInterval); // Stagger initial requests
		PendingQueryIDs[i] = 0;
	}

	RequestQueue.Reserve(MaxZombies / 2); // Reserve space for requests
}

void FPathFindingSystem::Update(float DeltaTime, const TArray<FVector>& ZombiePositions, const TArray<int32>& AliveIndices, FVector PlayerPosition)
{
	// Update path timers and queue requests for zombies that need new paths
	UpdatePathTimers(DeltaTime, ZombiePositions, AliveIndices, PlayerPosition);

	// Process the highest priority requests
	ProcessRequestQueue();
}

void FPathFindingSystem::UpdatePathTimers(float DeltaTime, const TArray<FVector>& ZombiePositions, const TArray<int32>& AliveIndices, FVector PlayerPosition)
{
	for (int32 i = 0; i < AliveIndices.Num(); i++)
	{
		const int32 Index = AliveIndices[i];

		// Update timer
		PathUpdateTimers[Index] -= DeltaTime;

		// Check if we need to request a new path
		if (PathUpdateTimers[Index] <= 0.0f && PendingQueryIDs[Index] == 0)
		{
			PathUpdateTimers[Index] = PathUpdateInterval;

			FVector ZombiePos = ZombiePositions[Index];
			float DistanceToPlayer = FVector::Dist(ZombiePos, PlayerPosition);

			// Skip distant zombies
			if (DistanceToPlayer > MaxPathDistance)
			{
				continue;
			}

			RequestPath(Index, ZombiePos, PlayerPosition);
		}
	}
}

void FPathFindingSystem::ProcessRequestQueue()
{
	if (RequestQueue.Num() == 0) return;

	SortRequestQueue();
	int32 RequestsToProcess = FMath::Min(MaxPathRequestsPerFrame, RequestQueue.Num());

	for (int32 i = 0; i < RequestsToProcess; i++)
	{
		const FPathRequest& Request = RequestQueue[i];
		int32 ZombieIndex = Request.ZombieIndex;

		// Create pathfinding query
		FPathFindingQuery Query(nullptr, *NavData, Request.StartPosition, Request.TargetPosition);

		// Request async path
		uint32 QueryID = NavSystem->FindPathAsync(
			NavAgentProperties,
			Query,
			FNavPathQueryDelegate::CreateRaw(this, &FPathFindingSystem::OnPathFound, ZombieIndex),
			EPathFindingMode::Regular
		);

		PendingQueryIDs[ZombieIndex] = QueryID;
	}

	// Remove processed requests
	RequestQueue.RemoveAt(0, RequestsToProcess, false);
}

void FPathFindingSystem::SortRequestQueue()
{
	// Sort by priority (min-heap: lower priority value = higher importance)
	RequestQueue.Sort([](const FPathRequest& A, const FPathRequest& B)
		{
			return A.Priority < B.Priority;
		});
}

float FPathFindingSystem::CalculatePriority(FVector ZombiePos, FVector PlayerPos) const
{
	// Priority = distance to player (closer zombies get calculated first)
	return FVector::DistSquared(ZombiePos, PlayerPos);
}

void FPathFindingSystem::RequestPath(int32 ZombieIndex, FVector StartPos, FVector TargetPos)
{
	// Add to queue with immediate priority
	FPathRequest Request;
	Request.ZombieIndex = ZombieIndex;
	Request.StartPosition = StartPos;
	Request.TargetPosition = TargetPos;
	Request.Priority = CalculatePriority(StartPos, TargetPos);
	Request.RequestTime = World->GetTimeSeconds();

	RequestQueue.Add(Request);
}

void FPathFindingSystem::OnPathFound(uint32 PathId, ENavigationQueryResult::Type Result, FNavPathSharedPtr Path, int32 ZombieIndex)
{
	// Clear pending query
	PendingQueryIDs[ZombieIndex] = 0;

	// Check result
	if (Result == ENavigationQueryResult::Success && Path.IsValid() && Path->IsValid())
	{
		CachedPaths[ZombieIndex] = Path;
		CurrentWaypointIndices[ZombieIndex] = 1; // Skip first waypoint (start position)
	}
	else
	{
		CachedPaths[ZombieIndex] = nullptr;
		CurrentWaypointIndices[ZombieIndex] = 0;
	}
}

FNavPathSharedPtr FPathFindingSystem::GetCachedPath(int32 ZombieIndex) const
{
	return CachedPaths[ZombieIndex];
}

bool FPathFindingSystem::HasValidPath(int32 ZombieIndex) const
{
	FNavPathSharedPtr Path = CachedPaths[ZombieIndex];
	return Path.IsValid() && Path->IsValid() && CurrentWaypointIndices[ZombieIndex] < Path->GetPathPoints().Num();
}

int32 FPathFindingSystem::GetCurrentWaypointIndex(int32 ZombieIndex) const
{
	return CurrentWaypointIndices[ZombieIndex];
}

void FPathFindingSystem::AdvanceWaypoint(int32 ZombieIndex)
{
	CurrentWaypointIndices[ZombieIndex]++;
}

void FPathFindingSystem::ForcePathRecalculation(int32 ZombieIndex)
{
	CachedPaths[ZombieIndex] = nullptr;
	CurrentWaypointIndices[ZombieIndex] = 0;
	PathUpdateTimers[ZombieIndex] = -1.0f; // Force immediate recalculation
}

void FPathFindingSystem::ClearZombieData(int32 ZombieIndex)
{
	CachedPaths[ZombieIndex] = nullptr;
	CurrentWaypointIndices[ZombieIndex] = 0;
	PathUpdateTimers[ZombieIndex] = 0.0f;
	PendingQueryIDs[ZombieIndex] = 0;
}