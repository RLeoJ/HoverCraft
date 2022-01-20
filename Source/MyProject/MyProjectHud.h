// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "GameFramework/HUD.h"
#include "MyProjectHud.generated.h"


UCLASS(config = Game)
class AMyProjectHud : public AHUD
{
	GENERATED_BODY()

public:
	AMyProjectHud();

	/** Font used to render the vehicle info */
	UPROPERTY()
	UFont* HUDFont;

	// Begin AHUD interface
	virtual void DrawHUD() override;
	// End AHUD interface
};
