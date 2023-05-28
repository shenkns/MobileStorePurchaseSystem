// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Data/ShopItemData.h"

#include "MobileStorePurchaseShopItemData.generated.h"

UCLASS()
class MOBILESTOREPURCHASESYSTEM_API UMobileStorePurchaseShopItemData : public UShopItemData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|MobileStorePurchase")
	FString ProductID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|MobileStorePurchase")
	bool bIsConsumable;
};
