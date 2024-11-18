// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralReverbActorComponent.h"

#include "Components/AudioComponent.h"
#include "ProceduralReverb/LogPrPartition.h"
#include "ProceduralReverb/Partition/PR_BSPNode.h"
#include "ProceduralReverb/Partition/PR_PartitionWorldSubsystem.h"
#include "Sound/SoundSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"

#if UE_ENABLE_DEBUG_DRAWING
static TAutoConsoleVariable<float> CVarDebugReverbParametersPrintInterval(
	TEXT("PR.Debug.Reverb.ParametersPrintInterval"),
	0.5f,
	TEXT("How often debug should be printed to log"),
	ECVF_Default
);
#endif // UE_ENABLE_DEBUG_DRAWING


// Sets default values for this component's properties
UProceduralReverbActorComponent::UProceduralReverbActorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UProceduralReverbActorComponent::BeginPlay()
{
	if (!ReverbSubmix)
	{
		ReverbSubmix = NewObject<USoundSubmix>(this);
	}
}


// Called every frame
void UProceduralReverbActorComponent::TickComponent(float DeltaTime, ELevelTick TickType,
													FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	check(GetOwner());

	const FVector& Position = GetOwner()->GetActorLocation();
	auto* ReverbSubsystem = GetWorld()->GetSubsystem<UPR_PartitionWorldSubsystem>();
	if (!ReverbSubsystem)
	{
		return;
	}

	TArray<TSharedPtr<FPR_BSPNode>> NearbyNodes;
	ReverbSubsystem->FindNearbyNodes(Position, NodesSearchRadius, NearbyNodes);

	if (NodesSearchRadius <= 0.0)
	{
		// TODO: Handle
		return;
	}

	if (NearbyNodes.IsEmpty())
	{
		return;
	}

	float Sum = 0.0f;
	TMap<TSharedPtr<FPR_BSPNode>, float> NodesWeights;
	// TODO: Check visibility to ignore nodes that are not visible
	for (TSharedPtr<FPR_BSPNode>& Node : NearbyNodes)
	{
		if (!Node->AcousticData)
		{
			continue;
		}

		const float Distance = Node->DistanceTo(Position);
		const float Weight = (1.0f - Distance / NodesSearchRadius);

		Sum += Weight;
		NodesWeights.Add(Node, Weight);
	}

	if (Sum <= 0.0f)
	{
		return;
	}

	FSubmixEffectReverbSettings CalculatedSettings;
	CalculatedSettings.DecayTime = 0;
	CalculatedSettings.Gain = 0;
	CalculatedSettings.Density = 0;
	CalculatedSettings.WetLevel = 0;

	float MaxWeight = 0.0f;
	for (auto& [Node, Weight] : NodesWeights)
	{
		Node->DrawDebug(GetWorld());
		const float NormalizedWeight = Weight / Sum;
		CalculatedSettings.DecayTime += (Node->AcousticData->ReverbSettings.DecayTime) * NormalizedWeight;

		if (Weight > MaxWeight)
		{
			MaxWeight = Weight;
			CalculatedSettings.Gain = Node->AcousticData->ReverbSettings.Gain;
			CalculatedSettings.Density = Node->AcousticData->ReverbSettings.Density;
			CalculatedSettings.WetLevel = Node->AcousticData->ReverbSettings.WetLevel;
		}
	}

	// Assign the reverb preset to the submix's effect chain
	for (TObjectPtr<USoundEffectSubmixPreset> Effect : ReverbSubmix->SubmixEffectChain)
	{
		auto* Reverb = Cast<USubmixEffectReverbPreset>(Effect);
		if (Reverb)
		{
			Reverb->SetSettings(CalculatedSettings);
		}
	}

#if UE_ENABLE_DEBUG_DRAWING
	static float TimePassedSincePrint = 0.0f;
	if (TimePassedSincePrint < CVarDebugReverbParametersPrintInterval.GetValueOnGameThread())
	{
		TimePassedSincePrint += DeltaTime;
		return;
	}

	TimePassedSincePrint = 0.0f;

	UE_LOG(LogPrPartition, Verbose, TEXT("Decay: [%.2f] Gain [%.2f] Density [%.2f] Wet Level [%.2f]"),
		CalculatedSettings.DecayTime,
		CalculatedSettings.Gain,
		CalculatedSettings.Density,
		CalculatedSettings.WetLevel);
#endif // UE_ENABLE_DEBUG_DRAWING
}

