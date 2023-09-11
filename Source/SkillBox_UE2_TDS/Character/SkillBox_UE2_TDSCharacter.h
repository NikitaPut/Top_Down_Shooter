// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SkillBox_UE2_TDS/WeaponDefault.h"
#include "SkillBox_UE2_TDS/FunctionLibrary/Types.h"
#include "SkillBox_UE2_TDSCharacter.generated.h"

UCLASS(Blueprintable)
class ASkillBox_UE2_TDSCharacter : public ACharacter
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
public:
	ASkillBox_UE2_TDSCharacter();

	// Called every frame.
	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* InInputComponent) override;
	
	/** Returns TopDownCameraComponent sub-object **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	/** Returns CameraBoom sub-object **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

public:
	// Cursor
	UPROPERTY()
		UDecalComponent* CurrentCursor = nullptr;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cursor")
		UMaterialInterface* CursorMaterial = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cursor")
		FVector CursorSize = FVector(20.0f, 40.0f, 40.0f);

	// Movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		EMovementState MovementState = EMovementState::Run_State;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		FCharacterSpeed MovementSpeedInfo;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		bool bSprintEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		bool bAimEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
		bool bWalkEnabled = false;
		
	UFUNCTION(BlueprintCallable)
		void ChangeMovementState();

	// Weapon
	UPROPERTY()
		AWeaponDefault* CurrentWeapon = nullptr;
	UFUNCTION(BlueprintCallable)
		AWeaponDefault* GetCurrentWeapon();
	
	UFUNCTION(BlueprintCallable)
		void InitWeapon(FName WeaponId);
		
	UFUNCTION(BlueprintCallable)
		void TryReloadWeapon();
	UFUNCTION()
		void WeaponReloadStart(UAnimMontage* Animation);
	UFUNCTION()
		void WeaponReloadEnd();
	UFUNCTION(BlueprintNativeEvent)
		void WeaponReloadStart_BP(UAnimMontage* Animation);
	UFUNCTION(BlueprintNativeEvent)
		void WeaponReloadEnd_BP();
	
	// for demo
	/// @deprecated 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
		FName InitWeaponName;

	// Inputs
	float AxisX{ 0.0f };
	float AxisY{ 0.0f };
	
	UFUNCTION(Category="Inputs")
		void InputAxisX(const float Value);

	UFUNCTION(Category="Inputs")
		void InputAxisY(const float Value);

	UFUNCTION()
		void InputAttackPressed();
	UFUNCTION()
		void InputAttackReleased();

	UFUNCTION(Category="Movement")
		void MovementTick(float DeltaTime);

	UFUNCTION()
		void RotationTick();
	
	UFUNCTION(BlueprintCallable)
		void AttackCharEvent(bool bIsFiring);

	UFUNCTION(BlueprintCallable)
		void CharacterUpdate();

	UFUNCTION(BlueprintCallable)
		UDecalComponent* GetCursorToWorld() const;
};
