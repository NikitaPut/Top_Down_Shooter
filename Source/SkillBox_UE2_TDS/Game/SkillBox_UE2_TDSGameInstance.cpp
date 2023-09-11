// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillBox_UE2_TDSGameInstance.h"

bool USkillBox_UE2_TDSGameInstance::GetWeaponInfoByName(FName WeaponName, FWeaponInfo& OutInfo) const
{
	bool bFound{false};

	if (!WeaponInfoTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("GameInstance::GetWeaponInfoByName - Weapon Table - NULL"));
		return bFound;
	}

	const FWeaponInfo* WeaponInfoRow = WeaponInfoTable->FindRow<FWeaponInfo>(WeaponName, "", false);
	if (WeaponInfoRow)
	{
		bFound = true;
		OutInfo = *WeaponInfoRow;
	}

	return bFound;
}
