// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"


class OPTIMIZESHOOTER_API FPathFindingSystem
{
public:
	FPathFindingSystem();

	// Initialize the manager
	void Initialize(UWorld* InWorld, int32 MaxZombies);

	// Update the system (call once per tick)
	void Update(float DeltaTime, const TArray<FVector>& ZombiePositions, const TArray<int32>& AliveIndices, FVector PlayerPosition);

	// Request a path for a zombie (queued with priority)
	void RequestPath(int32 ZombieIndex, FVector StartPos, FVector TargetPos);

	// Get cached path for a zombie
	FNavPathSharedPtr GetCachedPath(int32 ZombieIndex) const;

	// Check if zombie has a valid path
	bool HasValidPath(int32 ZombieIndex) const;

	// Get current waypoint index
	int32 GetCurrentWaypointIndex(int32 ZombieIndex) const;

	// Advance to next waypoint
	void AdvanceWaypoint(int32 ZombieIndex);

	// Invalidate path (force recalculation)
	void ForcePathRecalculation(int32 ZombieIndex);

	// Clear path data for a zombie
	void ClearZombieData(int32 ZombieIndex);

	// Configuration
	void SetMaxPathRequestsPerFrame(int32 MaxRequests) { MaxPathRequestsPerFrame = MaxRequests; }
	void SetPathUpdateInterval(float Interval) { PathUpdateInterval = Interval; }
	void SetMaxPathDistance(float Distance) { MaxPathDistance = Distance; }

private:
	// Path request with priority
	struct FPathRequest
	{
		int32 ZombieIndex;
		FVector StartPosition;
		FVector TargetPosition;
		float Priority; // Lower = higher priority (distance to player)
		float RequestTime;

		bool operator<(const FPathRequest& Other) const
		{
			return Priority < Other.Priority; // Min-heap
		}
	};

	// DOD: All zombie pathfinding data in contiguous arrays
	TArray<FNavPathSharedPtr> CachedPaths;
	TArray<int32> CurrentWaypointIndices;
	TArray<float> PathUpdateTimers;
	TArray<uint32> PendingQueryIDs;

	// Request queue (sorted by priority)
	TArray<FPathRequest> RequestQueue;

	// Navigation references
	UWorld* World;
	UNavigationSystemV1* NavSystem;
	ANavigationData* NavData;
	FNavAgentProperties NavAgentProperties;

	// Configuration
	int32 MaxPathRequestsPerFrame;  // Limit async requests per frame
	float PathUpdateInterval;        // Time between path recalculations
	float MaxPathDistance;           // Don't calculate paths beyond this distance
	float MinimumPlayerMovementToCompute; // Minimum player movement to trigger path recalculation

	// Internal methods
	void ProcessRequestQueue();
	void UpdatePathTimers(float DeltaTime, const TArray<FVector>& ZombiePositions, const TArray<int32>& AliveIndices, FVector PlayerPosition);
	void SortRequestQueue();
	float CalculatePriority(FVector ZombiePos, FVector PlayerPos) const;
	void OnPathFound(uint32 PathId, ENavigationQueryResult::Type Result, FNavPathSharedPtr Path, int32 ZombieIndex);
};