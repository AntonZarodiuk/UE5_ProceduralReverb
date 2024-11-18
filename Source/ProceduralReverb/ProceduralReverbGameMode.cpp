// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProceduralReverbGameMode.h"
#include "ProceduralReverbCharacter.h"
#include "UObject/ConstructorHelpers.h"

AProceduralReverbGameMode::AProceduralReverbGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
