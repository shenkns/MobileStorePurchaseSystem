// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "UObject/Object.h"

#include "Interfaces/OnlineStoreInterfaceV2.h"

#include "PurchaseProxyInterface.generated.h"

USTRUCT(BlueprintType)
struct MOBILESTOREPURCHASESYSTEM_API FPurchaseInfoRaw
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString ProductID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString TransactionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	TMap<FString, FString> CustomData;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FProductReceiveEvent, TSharedPtr<FOnlineStoreOffer> ProductInfo);
DECLARE_MULTICAST_DELEGATE_OneParam(FProductPurchaseEvent, FPurchaseInfoRaw PurchaseInfo);
DECLARE_MULTICAST_DELEGATE_OneParam(FProductPurchaseErrorEvent, FString Error);

UCLASS(Abstract)
class MOBILESTOREPURCHASESYSTEM_API UPurchaseProxyInterface : public UObject
{
	GENERATED_BODY()
	
public:

	UPurchaseProxyInterface(){}

public:

	FProductReceiveEvent OnProductReceive;
	
	FProductPurchaseEvent OnProductPurchased;

	FProductPurchaseErrorEvent OnProductPurchaseError;

	// Start purchase process
	virtual void Purchase(FString ProductID){};

	// Tell platform that we received a product. Place custom information into CustomData if necessary
	virtual void FinalizePurchase(const FPurchaseInfoRaw& PurchaseInfo){};

	// Request products info
	virtual void RequestProducts(TArray<FString> ProductsID){};

	// All other functions are platform related
};
