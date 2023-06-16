// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Proxies/PurchaseProxyInterface.h"

#include "Proxies/AndroidBillingHelper.h"

#include "PurchaseProxyInterfaceAndroid.generated.h"

UCLASS()
class MOBILESTOREPURCHASESYSTEM_API UPurchaseProxyInterfaceAndroid : public UPurchaseProxyInterface
{
	GENERATED_BODY()

public:
	
	virtual void Purchase(FString ProductID) override;
	
	virtual void RequestProducts(TArray<FString> ProductsID) override;

	virtual void FinalizePurchase(const FPurchaseInfoRaw& PurchaseInfo) override;

	UFUNCTION()
	void ProcessPurchase(FAndroidPurchaseInfo PurchaseInfo);
	
	UFUNCTION()
	void ProcessPurchaseFail(FString PurchaseID, FString Error);
	
	UFUNCTION()
	void ReceiveProduct(const FAndroidProductInfo& ProductInfo);
};
