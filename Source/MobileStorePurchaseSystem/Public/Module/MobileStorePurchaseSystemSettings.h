// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "UObject/Object.h"
#include "Widgets/PurchaseWidget.h"

#include "MobileStorePurchaseSystemSettings.generated.h"

class UPurchaseProxyInterface;

UCLASS(Config=Game, DefaultConfig)
class MOBILESTOREPURCHASESYSTEM_API UMobileStorePurchaseSystemSettings : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, Config, Category = "MobileStorePurchase")
	TArray<FString> StoreProductIDs;

	UPROPERTY(EditDefaultsOnly, Config, Category = "MobileStorePurchase")
	TMap<FString, TSoftClassPtr<UPurchaseProxyInterface>>PlatformsPurchaseInterfaceClasses;

	// Debug
	UPROPERTY(EditDefaultsOnly, Config, Category = "Debug")
	bool bShowDebugMessages = false;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Debug")
	bool bFakeInAppPurchasesInDevBuild = false;
};
