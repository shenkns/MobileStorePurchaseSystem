// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "UObject/Object.h"

#include "MobileStorePurchaseSystemSettings.generated.h"

UCLASS(Config=Game, DefaultConfig)
class MOBILESTOREPURCHASESYSTEM_API UMobileStorePurchaseSystemSettings : public UObject
{
	GENERATED_BODY()

public:

	// Debug
	UPROPERTY(EditDefaultsOnly, Config, Category = "Debug")
	bool bShowDebugMessages = false;
};
