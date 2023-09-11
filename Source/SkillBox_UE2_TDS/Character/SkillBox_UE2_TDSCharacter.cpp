// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkillBox_UE2_TDSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "SkillBox_UE2_TDS/Game/SkillBox_UE2_TDSGameInstance.h"

ASkillBox_UE2_TDSCharacter::ASkillBox_UE2_TDSCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void PositionCursorDecal(const APlayerController* PlayerController, UDecalComponent* Cursor)
{
	FHitResult TraceHitResult;
	PlayerController->GetHitResultUnderCursor(ECC_Visibility, true, TraceHitResult);
	const FVector CursorFV = TraceHitResult.ImpactNormal;
	const FRotator CursorR = CursorFV.Rotation();
	Cursor->SetWorldLocation(TraceHitResult.Location);
	Cursor->SetWorldRotation(CursorR);
}

void ASkillBox_UE2_TDSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (IsValid(PC) && IsValid(CurrentCursor))
	{
		PositionCursorDecal(PC, CurrentCursor);
	}

	MovementTick(DeltaSeconds);
	RotationTick();
}

void ASkillBox_UE2_TDSCharacter::BeginPlay()
{
	Super::BeginPlay();

	InitWeapon(InitWeaponName);
	
	if (IsValid(CursorMaterial))
	{
		CurrentCursor = UGameplayStatics::SpawnDecalAtLocation(GetWorld(), CursorMaterial, CursorSize, FVector(0));
	}
}

void ASkillBox_UE2_TDSCharacter::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);

	InInputComponent->BindAxis(FName("MoveForward"), this, &ASkillBox_UE2_TDSCharacter::InputAxisX);
	InInputComponent->BindAxis(FName("MoveRight"), this, &ASkillBox_UE2_TDSCharacter::InputAxisY);

	InInputComponent->BindAction(TEXT("FireEvent"), EInputEvent::IE_Pressed, this,
	                             &ASkillBox_UE2_TDSCharacter::InputAttackPressed);
	InInputComponent->BindAction(TEXT("FireEvent"), EInputEvent::IE_Released, this,
	                             &ASkillBox_UE2_TDSCharacter::InputAttackReleased);
	
	InInputComponent->BindAction(TEXT("ReloadEvent"), EInputEvent::IE_Released, this,
								 &ASkillBox_UE2_TDSCharacter::TryReloadWeapon);
}

void ASkillBox_UE2_TDSCharacter::InputAxisX(const float Value)
{
	AxisX = Value;
}

void ASkillBox_UE2_TDSCharacter::InputAxisY(const float Value)
{
	AxisY = Value;
}

void ASkillBox_UE2_TDSCharacter::InputAttackPressed()
{
	AttackCharEvent(true);
}

void ASkillBox_UE2_TDSCharacter::InputAttackReleased()
{
	AttackCharEvent(false);
}

void ASkillBox_UE2_TDSCharacter::AttackCharEvent(bool bIsFiring)
{
	AWeaponDefault* MyWeapon{GetCurrentWeapon()};

	if (MyWeapon)
	{
		//ToDo Check melee or range
		MyWeapon->SetWeaponStateFire(bIsFiring);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ASkillBox_UE2_TDSCharacter::AttackCharEvent - CurrentWeapon - NULL"));
	}
}

void ASkillBox_UE2_TDSCharacter::MovementTick(float DeltaTime)
{
	AddMovementInput(FVector(1.0f, 0.0f, 0.0f), AxisX);
	AddMovementInput(FVector(0.0f, 1.0f, 0.0f), AxisY);
}

void ASkillBox_UE2_TDSCharacter::RotationTick()
{
	const TWeakObjectPtr<APlayerController> MyController{Cast<APlayerController>(GetController())};
	if (!MyController.IsValid())
	{
		return;
	}

	FHitResult CursorHitResult;
	MyController->GetHitResultUnderCursor(ECC_GameTraceChannel1, true, CursorHitResult);

	FRotator RotationToCursor{
		UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), CursorHitResult.Location)
	};
	// Constraint rotation to a plane
	RotationToCursor.Pitch = 0.0f;
	RotationToCursor.Roll = 0.0f;

	SetActorRotation(FQuat{RotationToCursor});

	if (CurrentWeapon)
	{
		FVector Displacement{0};
		switch (MovementState)
		{
		case EMovementState::Aim_State:
		case EMovementState::AimWalk_State:
			Displacement.Z = 160.0f;
			CurrentWeapon->ShouldReduceDispersion = true;
			break;
		case EMovementState::Walk_State:
		case EMovementState::Run_State:
			Displacement.Z = 120.0f;
			CurrentWeapon->ShouldReduceDispersion = false;
			break;
		case EMovementState::Sprint_State:
			break;
		default:
			break;
		}

		CurrentWeapon->ShootEndLocation = CursorHitResult.Location + Displacement;
	}
}

void ASkillBox_UE2_TDSCharacter::CharacterUpdate()
{
	float ResSpeed = 600.0f;
	switch (MovementState)
	{
	case EMovementState::AimWalk_State:
		ResSpeed = MovementSpeedInfo.AimWalkSpeed;
		break;
	case EMovementState::Aim_State:
		ResSpeed = MovementSpeedInfo.AimSpeed;
		break;
	case EMovementState::Walk_State:
		ResSpeed = MovementSpeedInfo.WalkSpeed;
		break;
	case EMovementState::Run_State:
		ResSpeed = MovementSpeedInfo.RunSpeed;
		break;
	case EMovementState::Sprint_State:
		ResSpeed = MovementSpeedInfo.SprintSpeed;
		break;
	default:
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = ResSpeed;
}

void ASkillBox_UE2_TDSCharacter::ChangeMovementState()
{
	// Default movement state is Run
	if (!bSprintEnabled && !bWalkEnabled && !bAimEnabled)
	{
		MovementState = EMovementState::Run_State;
	}
	else if (!bSprintEnabled && !bWalkEnabled && bAimEnabled)
	{
		MovementState = EMovementState::Aim_State;
	}
	else if (!bSprintEnabled && bWalkEnabled && !bAimEnabled)
	{
		MovementState = EMovementState::Walk_State;
	}
	else if (!bSprintEnabled && bWalkEnabled && bAimEnabled)
	{
		MovementState = EMovementState::AimWalk_State;
	}
	else if (bSprintEnabled)
	{
		bWalkEnabled = false;
		bAimEnabled = false;
		MovementState = EMovementState::Sprint_State;
	}
	
	CharacterUpdate();

	AWeaponDefault* myWeapon = GetCurrentWeapon();
	if (myWeapon)
	{
		myWeapon->UpdateStateWeapon(MovementState);
	}
}

UDecalComponent* ASkillBox_UE2_TDSCharacter::GetCursorToWorld() const
{
	return CurrentCursor;
}

void ASkillBox_UE2_TDSCharacter::TryReloadWeapon()
{
	if (!CurrentWeapon)
	{
		return;
	}

	if (CurrentWeapon->GetWeaponRounds() < CurrentWeapon->WeaponSetting.MaxRound)
	{
		CurrentWeapon->InitReload();
	}
}

void ASkillBox_UE2_TDSCharacter::WeaponReloadStart(UAnimMontage* Animation)
{
	WeaponReloadStart_BP(Animation);
}

void ASkillBox_UE2_TDSCharacter::WeaponReloadEnd()
{
	WeaponReloadEnd_BP();
}

void ASkillBox_UE2_TDSCharacter::WeaponReloadStart_BP_Implementation(UAnimMontage* Animation)
{
	// done in Blueprints
}

void ASkillBox_UE2_TDSCharacter::WeaponReloadEnd_BP_Implementation()
{
	// done in Blueprints
}


AWeaponDefault* ASkillBox_UE2_TDSCharacter::GetCurrentWeapon()
{
	return CurrentWeapon;
}

void ASkillBox_UE2_TDSCharacter::InitWeapon(FName WeaponId)
{
	const USkillBox_UE2_TDSGameInstance* MyGameInstance = Cast<USkillBox_UE2_TDSGameInstance>(GetGameInstance());
	if (!IsValid(MyGameInstance))
	{
		UE_LOG(LogTemp, Warning, TEXT("Character::InitWeapon - GetGameInstance returned nothing"))
		return;
	}
	FWeaponInfo MyWeaponInfo;
	if (!MyGameInstance->GetWeaponInfoByName(WeaponId, MyWeaponInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("Character::InitWeapon - Weapon not found in table"));
		return;
	}
	if (!IsValid(MyWeaponInfo.WeaponClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("Character::InitWeapon - Weapon info does not define class!"));
		return;
	}
	
	const FVector SpawnLocation = FVector(0);
	const FRotator SpawnRotation = FRotator(0);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();

	AWeaponDefault* MyWeapon = Cast<AWeaponDefault>(
		GetWorld()->SpawnActor(MyWeaponInfo.WeaponClass, &SpawnLocation, &SpawnRotation, SpawnParams));
	if (!IsValid(MyWeapon))
	{
		UE_LOG(LogTemp, Warning, TEXT("Character::InitWeapon - Could not spawn weapon!"));
		return;
	}
	const FAttachmentTransformRules Rule(EAttachmentRule::SnapToTarget, false);
	MyWeapon->AttachToComponent(GetMesh(), Rule, FName("WeaponSocketRightHand"));
	CurrentWeapon = MyWeapon;

	MyWeapon->WeaponSetting = MyWeaponInfo;
	MyWeapon->WeaponAdditionalInfo.Round = MyWeaponInfo.MaxRound;

	// Remove!!!Debug
	MyWeapon->ReloadTime = MyWeaponInfo.ReloadTime;
	
	MyWeapon->UpdateStateWeapon(MovementState);

	MyWeapon->OnWeaponReloadStart.AddDynamic(this, &ASkillBox_UE2_TDSCharacter::WeaponReloadStart);
	MyWeapon->OnWeaponReloadEnd.AddDynamic(this, &ASkillBox_UE2_TDSCharacter::WeaponReloadEnd);
}
