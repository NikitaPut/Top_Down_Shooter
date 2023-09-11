// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillBox_UE2_TDSGameMode.h"
#include "SkillBox_UE2_TDSPlayerController.h"
#include "SkillBox_UE2_TDS/Character/SkillBox_UE2_TDSCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASkillBox_UE2_TDSGameMode::ASkillBox_UE2_TDSGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = ASkillBox_UE2_TDSPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprint/Character/BP_Character"));
	if (PlayerPawnBPClass.Class != nullptr)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}