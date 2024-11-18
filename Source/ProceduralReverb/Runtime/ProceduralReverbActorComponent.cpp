// Fill out your copyright notice in the Description page of Project Settings.


#include "ProceduralReverbActorComponent.h"

#include "Components/AudioComponent.h"
#include "ProceduralReverb/Partition/PR_BSPNode.h"
#include "ProceduralReverb/Partition/PR_PartitionWorldSubsystem.h"
#include "Sound/SoundSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectReverb.h"


// Sets default values for this component's properties
UProceduralReverbActorComponent::UProceduralReverbActorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UProceduralReverbActorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!ReverbPreset)
	{
		ReverbPreset = NewObject<USubmixEffectReverbPreset>(this);
	}

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
	for (auto& [Node, Weight] : NodesWeights)
	{
		Node->DrawDebug(GetWorld());
		const float NormalizedWeight = Weight / Sum;
		CalculatedSettings.DecayTime += (Node->AcousticData->ReverbSettings.DecayTime) * NormalizedWeight;
		CalculatedSettings.Gain += Node->AcousticData->ReverbSettings.Gain * NormalizedWeight;
		CalculatedSettings.Density += Node->AcousticData->ReverbSettings.Density * NormalizedWeight;
		CalculatedSettings.WetLevel += Node->AcousticData->ReverbSettings.WetLevel * NormalizedWeight;
	}

	// CalculatedSettings.Gain = 1.0f;
	// CalculatedSettings.WetLevel = 1.0f;

	// Assign the reverb preset to the submix's effect chain
	for (auto Effect : ReverbSubmix->SubmixEffectChain)
	{
		auto* Reverb = Cast<USubmixEffectReverbPreset>(Effect);
		if (Reverb)
		{
			Reverb->SetSettings(CalculatedSettings);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Decay: [%f]"), CalculatedSettings.DecayTime);
	TArray<UAudioComponent*> AudioComponents;
	GetOwner()->GetComponents<UAudioComponent>(AudioComponents);
	// for (UAudioComponent* AudioComponent : AudioComponents)
	// {
	// 	AudioComponent->SetSubmixSend(ReverbSubmix, 1.0f);
	// }
}

