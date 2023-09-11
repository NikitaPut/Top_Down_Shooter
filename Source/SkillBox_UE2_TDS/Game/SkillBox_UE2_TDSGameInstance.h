// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "SkillBox_UE2_TDS/FunctionLibrary/Types.h"
#include "SkillBox_UE2_TDSGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SKILLBOX_UE2_TDS_API USkillBox_UE2_TDSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Setting")
		UDataTable* WeaponInfoTable = nullptr;
	UFUNCTION(BlueprintCallable)
		bool GetWeaponInfoByName(FName WeaponName, FWeaponInfo& OutInfo) const;
};
