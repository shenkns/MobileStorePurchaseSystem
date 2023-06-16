// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "UObject/Object.h"

#include "AndroidBillingHelper.generated.h"

USTRUCT(BlueprintType)
struct MOBILESTOREPURCHASESYSTEM_API FAndroidProductInfo
{
	GENERATED_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString ProductID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	int MicrosPrice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString FormattedPrice;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString CurrencyCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString Name;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString Type;
};

USTRUCT(BlueprintType)
struct MOBILESTOREPURCHASESYSTEM_API FAndroidPurchaseInfo
{
	GENERATED_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString ProductID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString Token;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString Signature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	FString OrderID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Billing")
	TMap<FString, FString> Details;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAndroidProductQuery, const FAndroidProductInfo&, ProductInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAndroidPurchase, FAndroidPurchaseInfo, PurchaseInfo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAndroidPurchaseFail, FString, ProductID, FString, Error);

UCLASS()
class MOBILESTOREPURCHASESYSTEM_API UAndroidBillingHelper : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FOnAndroidProductQuery OnProductInfoReceive;

	UPROPERTY(BlueprintAssignable)
	FOnAndroidPurchase OnPurchaseSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnAndroidPurchaseFail OnPurchaseFail;

public:

	UFUNCTION(BlueprintPure, Category="Billing")
	static UAndroidBillingHelper* Get();
	
	UFUNCTION(BlueprintCallable, Category="Billing")
	void RequestProducts(TArray<FString> ProductIDs);

	UFUNCTION(BlueprintCallable, Category="Billing")
	void Purchase(FString ProductID);

	UFUNCTION(BlueprintCallable, Category="Billing")
	void FinalizePurchase(FAndroidPurchaseInfo PurchaseInfo, bool Consume);
};
