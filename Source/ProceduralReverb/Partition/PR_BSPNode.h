// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Chaos/ChaosEngineInterface.h"
#include "CoreMinimal.h"
#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"

namespace UE::NNE
{
class FTensorShape;
class IModelInstanceCPU;
}

struct FPR_Polygon;


struct FPR_AcousticData
{
	TArray<float, TInlineAllocator<6>> Distances;
	TArray<EPhysicalSurface, TInlineAllocator<6>> Materials;

	FSubmixEffectReverbSettings ReverbSettings;
};


enum EDistances
{
	Front = 0,
	Back,
	Right,
	Left,
	Up,
	Down
};


struct FPR_BSPNode
{
	FPR_BSPNode(const FBox& BoundingBox);

	void PartitionSpace(int32 Depth);
	void CollectAcousticData(const UWorld* World);

	TSharedPtr<FPR_BSPNode> FindNode(const FVector& Position);
	void FindNearbyNodes(const FVector& Position, float SearchRadius, TArray<TSharedPtr<FPR_BSPNode>>& OutNearbyNodes);
	float DistanceTo(const FVector& Point) const;

	void RunModel(
		const TSharedPtr<UE::NNE::IModelInstanceCPU>& ModelInstance,
		const TArray<UE::NNE::FTensorShape>& InputTensorShapes,
		const TArray<UE::NNE::FTensorShape>& OutputTensorShapes) const;

	void ConvertAcousticData(TArray<float>& OutData) const;
	void SaveModelOutputData(const TArray<float>& OutputData) const;

	void DrawDebug(const UWorld* World) const;

	FBox BoundingBox;
	TSharedPtr<FPR_BSPNode> LeftChild;
	TSharedPtr<FPR_BSPNode> RightChild;
	bool bIsLeaf = false;

	TSharedPtr<FPR_AcousticData> AcousticData;

	FColor Color = FColor::MakeRandomColor();

	int32 NodeId = INDEX_NONE;
};
