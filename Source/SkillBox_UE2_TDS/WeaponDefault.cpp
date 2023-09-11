// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponDefault.h"

#include "DrawDebugHelpers.h"
#include "ProjectileDefault.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AWeaponDefault::AWeaponDefault()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	RootComponent = SceneComponent;

	SkeletalMeshWeapon = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skeletal Mesh"));
	SkeletalMeshWeapon->SetGenerateOverlapEvents(false);
	SkeletalMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshWeapon->SetupAttachment(RootComponent);

	StaticMeshWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Static Mesh "));
	StaticMeshWeapon->SetGenerateOverlapEvents(false);
	StaticMeshWeapon->SetCollisionProfileName(TEXT("NoCollision"));
	StaticMeshWeapon->SetupAttachment(RootComponent);

	ShootLocation = CreateDefaultSubobject<UArrowComponent>(TEXT("ShootLocation"));
	ShootLocation->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AWeaponDefault::BeginPlay()
{
	Super::BeginPlay();

	WeaponInit();
}

// Called every frame
void AWeaponDefault::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	FireTick(DeltaTime);
	ReloadTick(DeltaTime);
	DispersionTick(DeltaTime);
}

void AWeaponDefault::FireTick(const float DeltaTime)
{
	if (!bWeaponFiring)
	{
		return;
	}
	
	if (bWeaponReloading)
	{
		return;
	}
	
	if (FireTimer > 0.f)
	{
		FireTimer -= DeltaTime;
		return;
	}

	if (GetWeaponRounds() > 0)
	{
		Fire();
	}
	else
	{
		InitReload();
	}
}

void AWeaponDefault::ReloadTick(float DeltaTime)
{
	if (!bWeaponReloading)
	{
		return;
	}
	
	if (ReloadTimer < 0.0f)
	{
		FinishReload();
	}
	else
	{
		ReloadTimer -= DeltaTime;
	}
}

void AWeaponDefault::DispersionTick(float DeltaTime)
{
	if (bWeaponReloading)
	{
		return;
	}

	if (!bWeaponFiring)
	{
		if (ShouldReduceDispersion)
		{
			CurrentDispersion -= CurrentDispersionReduction;
		}
		else
		{
			CurrentDispersion += CurrentDispersionReduction; // FIXME: "reduction" and "increase" can be different
		}

		CurrentDispersion = FMath::Clamp(CurrentDispersion, CurrentDispersionMin, CurrentDispersionMax);
	}
	
	if (bShowDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dispersion: [%f -> %f -> %f]"), CurrentDispersionMin, CurrentDispersion, CurrentDispersionMax);
	}
}


void AWeaponDefault::WeaponInit()
{
	if (SkeletalMeshWeapon && !SkeletalMeshWeapon->SkeletalMesh)
	{
		SkeletalMeshWeapon->DestroyComponent(true);
	}

	if (StaticMeshWeapon && !StaticMeshWeapon->GetStaticMesh())
	{
		StaticMeshWeapon->DestroyComponent();
	}

	UpdateStateWeapon(EMovementState::Run_State);
}

void AWeaponDefault::SetWeaponStateFire(const bool bIsWeaponFiring)
{
	if (CheckWeaponCanFire())
	{
		bWeaponFiring = bIsWeaponFiring;
	}
	else
	{
		bWeaponFiring = false;
		FireTimer = 0.01f; /// ?!?!?!?!?
	}
}

bool AWeaponDefault::CheckWeaponCanFire() const
{
	return !bBlockFire;
}

FProjectileInfo AWeaponDefault::GetProjectile()
{	
	return WeaponSetting.ProjectileSetting;
}

void AWeaponDefault::Fire()
{
	--WeaponAdditionalInfo.Round;
	FireTimer = WeaponSetting.RateOfFire;

	UGameplayStatics::SpawnSoundAtLocation(
		GetWorld(),
		WeaponSetting.SoundOnFire,
		ShootLocation->GetComponentLocation()
	);
	UGameplayStatics::SpawnEmitterAtLocation(
		GetWorld(),
		WeaponSetting.EffectOnFire,
		ShootLocation->GetComponentTransform()
	);
	
	if (!IsValid(ShootLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("AWeaponDefault::Fire - No shoot location to shoot from!"));
		return;
	}
	
	
	const FProjectileInfo ProjectileInfo{GetProjectile()};

	const FVector SpawnLocation{ShootLocation->GetComponentLocation()};
	FRotator SpawnRotation = ShootLocation->GetComponentRotation();
	FVector FireEndLocation;
	int8 NumberProjectile = GetNumberProjectileByShot();
	for (int8 i = 0; i < NumberProjectile; ++i)
	{
		FireEndLocation = GetFireEndLocation();
		FVector ShootDirection{FireEndLocation - SpawnLocation};
		ShootDirection.Normalize();
		
		FMatrix TempMatrix(ShootDirection, FVector(0, 1, 0), FVector(0, 0, 1), FVector::ZeroVector);
		SpawnRotation = TempMatrix.Rotator();
		
		if (ProjectileInfo.Projectile)
		{
			//Projectile Init ballistic fire
			
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = GetInstigator();

			AProjectileDefault* myProjectile = Cast<AProjectileDefault>(GetWorld()->SpawnActor(ProjectileInfo.Projectile, &SpawnLocation, &SpawnRotation, SpawnParams));
			if (!IsValid(myProjectile))
			{
				UE_LOG(LogTemp, Warning, TEXT("AWeaponDefault::Fire - Could not spawn projectile!"));
			}

			myProjectile->InitProjectile(WeaponSetting.ProjectileSetting);
		}
		else
		{
			//ToDo Projectile null Init trace fire
			// GetWorld()->LineTraceSingleByChannel()
		}
	}
}

void AWeaponDefault::UpdateStateWeapon(EMovementState NewMovementState)
{
	//ToDo Dispersion
	bBlockFire = false;

	switch (NewMovementState)
	{
	case EMovementState::Aim_State:
		CurrentDispersionMax = WeaponSetting.Dispersion.AimState_MaxDispersion;
		CurrentDispersionMin = WeaponSetting.Dispersion.AimState_MinDispersion;
		CurrentDispersionRecoil = WeaponSetting.Dispersion.AimState_Recoil;
		CurrentDispersionReduction = WeaponSetting.Dispersion.AimState_ReductionSpeed;
		break;
	case EMovementState::AimWalk_State:
		CurrentDispersionMax = WeaponSetting.Dispersion.AimWalkState_MaxDispersion;
		CurrentDispersionMin = WeaponSetting.Dispersion.AimWalkState_MinDispersion;
		CurrentDispersionRecoil = WeaponSetting.Dispersion.AimWalkState_Recoil;
		CurrentDispersionReduction = WeaponSetting.Dispersion.AimWalkState_ReductionSpeed;
		break;
	case EMovementState::Walk_State:
		CurrentDispersionMax = WeaponSetting.Dispersion.WalkState_MaxDispersion;
		CurrentDispersionMin = WeaponSetting.Dispersion.WalkState_MinDispersion;
		CurrentDispersionRecoil = WeaponSetting.Dispersion.WalkState_Recoil;
		CurrentDispersionReduction = WeaponSetting.Dispersion.WalkState_ReductionSpeed;
		break;
	case EMovementState::Run_State:
		CurrentDispersionMax = WeaponSetting.Dispersion.RunState_MaxDispersion;
		CurrentDispersionMin = WeaponSetting.Dispersion.RunState_MinDispersion;
		CurrentDispersionRecoil = WeaponSetting.Dispersion.RunState_Recoil;
		CurrentDispersionReduction = WeaponSetting.Dispersion.RunState_ReductionSpeed;
		break;
	case EMovementState::Sprint_State:
		bBlockFire = true;
		SetWeaponStateFire(false);
		break;
	default:
		break;
	}
}

void AWeaponDefault::ChangeDispersionByShot()
{
	CurrentDispersion += CurrentDispersionRecoil;
}

int32 AWeaponDefault::GetWeaponRounds()
{
	return WeaponAdditionalInfo.Round;
}

void AWeaponDefault::InitReload()
{
	bWeaponReloading = true;

	ReloadTimer = WeaponSetting.ReloadTime;

	// TODO: Start reload animation
	if (WeaponSetting.ReloadAnimation)
	{
		OnWeaponReloadStart.Broadcast(WeaponSetting.ReloadAnimation);
	}
}

void AWeaponDefault::FinishReload()
{
	bWeaponReloading = false;
	WeaponAdditionalInfo.Round = WeaponSetting.MaxRound;

	OnWeaponReloadEnd.Broadcast();
}

float AWeaponDefault::GetCurrentDispersion() const
{
	return CurrentDispersion;
}

FVector AWeaponDefault::ApplyDispersionToShoot(FVector DirectionShoot) const
{
	return FMath::VRandCone(DirectionShoot, GetCurrentDispersion() * PI / 180.0f);
}

FVector AWeaponDefault::GetFireEndLocation() const
{
	const FVector ShootDirection{ShootEndLocation - ShootLocation->GetComponentLocation()};
	const FVector NormalizedShotDirection{
		(ShootDirection.Size() > SizeVectorToChangeShotDirectionLogic)
			? ShootDirection.GetSafeNormal()
			: ShootLocation->GetForwardVector()
	};
	
	const FVector EndLocation{
		ShootLocation->GetComponentLocation() + ApplyDispersionToShoot( NormalizedShotDirection ) * 20000.0f
	};

	if (bShowDebug)
	{
		DrawDebugCone(
			GetWorld(),
			ShootLocation->GetComponentLocation(),
			NormalizedShotDirection,
			WeaponSetting.HitScanDistance,
			GetCurrentDispersion() * PI / 180.f,
			GetCurrentDispersion() * PI / 180.f,
			32,
			FColor::Emerald,
			false,
			.1f,
			0,
			1.0f
		);

		// Weapon direction
		DrawDebugLine(
			GetWorld(),
			ShootLocation->GetComponentLocation(),
			ShootLocation->GetComponentLocation() + ShootLocation->GetForwardVector() * 500.0f,
			FColor::Cyan,
			false,
			5.f,
			0,
			.5f
		);

		// Targeting direction (ignoring dispersion)
		DrawDebugLine(
			GetWorld(),
			ShootLocation->GetComponentLocation(),
			ShootEndLocation,
			FColor::Red,
			false,
			5.f,
			0,
			.5f
		);

		// Actual bullet direction (after applying dispersion)
		DrawDebugLine(
			GetWorld(),
			ShootLocation->GetComponentLocation(),
			EndLocation,
			FColor::Black,
			false,
			5.f,
			0,
			.5f
		);
		
		// TODO: Draw debug sphere if the projectile is grenade
		//DrawDebugSphere(GetWorld(), ShootLocation->GetComponentLocation() + ShootLocation->GetForwardVector()*SizeVectorToChangeShootDirectionLogic, 10.f, 8, FColor::Red, false, 4.0f);
	}

	return EndLocation;
}

int8 AWeaponDefault::GetNumberProjectileByShot() const
{
	return WeaponSetting.NumberProjectileByShot;
}
