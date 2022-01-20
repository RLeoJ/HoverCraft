// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include <Runtime/Engine/Public/DrawDebugHelpers.h>

AMyProjectPawn::AMyProjectPawn()
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		FConstructorStatics()
			: PlaneMesh(TEXT("/Game/Flying/Meshes/UFO.UFO"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	// Create static mesh component
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh0"));
	PlaneMesh->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());	// Set static mesh
	RootComponent = PlaneMesh;

	// Create a spring arm component
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm0"));
	SpringArm->SetupAttachment(RootComponent);	// Attach SpringArm to RootComponent
	SpringArm->TargetArmLength = 160.0f; // The camera follows at this distance behind the character	
	SpringArm->SocketOffset = FVector(0.f,0.f,60.f);
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 15.f;

	// Create camera component 
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera0"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);	// Attach the camera
	Camera->bUsePawnControlRotation = false; // Don't rotate camera with controller

	// Set handling parameters
	Acceleration = 600.f;
	TurnSpeed = 50.f;
	MaxSpeed = 1000.f;
	MinSpeed = 0.f;
	CurrentX = 500.f;
}

void AMyProjectPawn::Tick(float DeltaSeconds)
{
	

	// Move plan forwards (with sweep so we stop when we collide with things)	



	// Calculate change in rotation this frame
	FRotator DeltaRotation(0,0,0);
	DeltaRotation.Pitch = CurrentPitchSpeed * DeltaSeconds;
	DeltaRotation.Yaw = CurrentYawSpeed * DeltaSeconds;
	DeltaRotation.Roll = CurrentRollSpeed * DeltaSeconds;

	// change in transform per frame
	FVector DeltaLocation(0, 0, 0);
	DeltaLocation.X = CurrentX * DeltaSeconds;
	DeltaLocation.Y = CurrentY * DeltaSeconds;
	DeltaLocation.Z = CurrentZ * DeltaSeconds;

	const FVector gravity = FVector(0.f, 0.f, DeltaLocation.Z - 98.0 * DeltaSeconds);
	const FVector antigravity = FVector(0.f, 0.f, DeltaLocation.Z + 130.0 * DeltaSeconds);
	
	//Draw line between ship and ground
	FHitResult OutHit;
	origin = this->GetActorLocation();
	direction = FVector(0.f, 0.f, -5000.f);
	FCollisionQueryParams cp;
	cp.AddIgnoredActor(this);
	cast = origin + direction;
	DrawDebugLine(GetWorld(), origin, cast, FColor::Red, false, 1, 0, 1);
	IsHit = GetWorld()->LineTraceSingleByChannel(OutHit, origin, direction, ECC_Visibility, cp);
	//if (IsHit)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("hit = %f"), OutHit.Location.Z);
	//	UE_LOG(LogTemp, Warning, TEXT("origin = %f"), origin.Z);
	//	if (origin.Z - OutHit.Location.Z >= 200.f)
	//	{
	//		CurrentZ -= 980.0 * DeltaSeconds;
	//	}
	//	else
	//	{
	//		CurrentZ += 2048.0 * DeltaSeconds;
	//	}
	//}

	this->SetActorLocation(FVector(CurrentX, CurrentY, OutHit.Location.Z + 200.f));

	//Keep ship oriented upright
	if (FMath::Abs(GetActorRotation().Pitch) != 0)
	{
		float targetPitch = 0.f;
		this->SetActorRotation(FRotator(GetActorRotation().Roll, targetPitch, GetActorRotation().Yaw).Quaternion());
	}


	// Rotate plane
	AddActorLocalRotation(DeltaRotation, true);

	// Transform 
	AddActorLocalOffset(DeltaLocation, true);



	// Call any parent class Tick implementation
	Super::Tick(DeltaSeconds);
}

void AMyProjectPawn::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Deflect along the surface when we collide.
	FRotator CurrentRotation = GetActorRotation();
	SetActorRotation(FQuat::Slerp(CurrentRotation.Quaternion(), HitNormal.ToOrientationQuat(), 0.025f));
}


void AMyProjectPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
    // Check if PlayerInputComponent is valid (not NULL)
	check(PlayerInputComponent);

	// Bind our control axis' to callback functions
	PlayerInputComponent->BindAxis("Thrust", this, &AMyProjectPawn::ThrustInput);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyProjectPawn::MoveRightInput);
}

void AMyProjectPawn::HoverPhys()
{
	
}

void AMyProjectPawn::ThrustInput(float Val)
{
	// Is there any input?
	bool bHasInput = !FMath::IsNearlyEqual(Val, 0.f);
	// If input is not held down, reduce speed
	float CurrentAcc = bHasInput ? (Val * Acceleration) : (-0.5f * Acceleration);
	// Calculate new speed
	float NewX = CurrentX + (GetWorld()->GetDeltaSeconds() * CurrentAcc);
	// Clamp between MinSpeed and MaxSpeed
	CurrentX = FMath::Clamp(NewX, MinSpeed, MaxSpeed);
}

void AMyProjectPawn::MoveUpInput(float Val)
{
	// Target pitch speed is based in input
	float TargetPitchSpeed = (Val * TurnSpeed * -1.f);

	// When steering, we decrease pitch slightly
	TargetPitchSpeed += (FMath::Abs(CurrentYawSpeed) * -0.2f);

	// Smoothly interpolate to target pitch speed
	CurrentPitchSpeed = FMath::FInterpTo(CurrentPitchSpeed, TargetPitchSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}

void AMyProjectPawn::MoveRightInput(float Val)
{
	// Target yaw speed is based on input
	float TargetYawSpeed = (Val * TurnSpeed);

	// Smoothly interpolate to target yaw speed
	CurrentYawSpeed = FMath::FInterpTo(CurrentYawSpeed, TargetYawSpeed, GetWorld()->GetDeltaSeconds(), 2.f);

	// Is there any left/right input?
	const bool bIsTurning = FMath::Abs(Val) > 0.2f;

	 //If turning, yaw value is used to influence roll
	 //If not turning, roll to reverse current roll value.
	float TargetRollSpeed = bIsTurning ? (CurrentYawSpeed * 0.5f) : (GetActorRotation().Roll * -2.f);

	 //Smoothly interpolate roll speed
	CurrentRollSpeed = FMath::FInterpTo(CurrentRollSpeed, TargetRollSpeed, GetWorld()->GetDeltaSeconds(), 2.f);
}
