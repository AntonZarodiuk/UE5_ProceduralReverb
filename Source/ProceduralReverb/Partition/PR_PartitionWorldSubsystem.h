// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PR_PartitionWorldSubsystem.generated.h"

struct FPR_Polygon;
struct FPR_BSPNode;

/**
 * 
 */
UCLASS()
class PROCEDURALREVERB_API UPR_PartitionWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

protected:
	// UWorldSubsystem interface
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	//~ UWorldSubsystem interface

public:
	void Generate();

	FBox GetInitialBoundingBox() const;
	void GenerateBSPTree(const FBox& InitialBox);

	void FindNearbyNodes(const FVector& Position, float SearchRadius, TSet<TSharedPtr<FPR_BSPNode>>& OutNearbyNodes) const;
	void FindNearbyNodes(const FVector& Position, TSet<TSharedPtr<FPR_BSPNode>>& OutNearbyNodes, int32 MaxSearchDepth = 0, int32 CurrentSearchDepth = 0) const;

private:
	TSharedPtr<FPR_BSPNode> RootNode;
};
