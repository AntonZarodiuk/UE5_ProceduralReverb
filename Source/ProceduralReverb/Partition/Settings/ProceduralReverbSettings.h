// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProceduralReverbSettings.generated.h"

class UNNEModelData;
/**
 * 
 */
UCLASS(Config = Game, DefaultConfig)
class PROCEDURALREVERB_API UProceduralReverbSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditDefaultsOnly, Category = "Partition", meta = (ClampMin = 1, UIMin = 1, ClampMax = 100, UIMax = 100))
	int32 MaxPartitionDepth = 10;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Partition", meta = (Units = "cm", ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 100000.0f, UIMax = 100000.0f))
	float RayDistance = 5000.0f;

	UPROPERTY(Config, EditAnywhere)
	TSoftObjectPtr<UNNEModelData> PreLoadedModelData;
};
