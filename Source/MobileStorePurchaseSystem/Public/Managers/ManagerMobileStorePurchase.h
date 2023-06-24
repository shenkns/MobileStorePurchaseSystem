// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Managers/Manager.h"

#include "OnlineSubsystem.h"
#include "PlatformTypePurchases/PlatformTypePurchase.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Proxies/PurchaseProxyInterface.h"

#include "ManagerMobileStorePurchase.generated.h"

class UShopItemData;
class UManagerMobileStorePurchase;
class UPurchaseProxyInterface;

USTRUCT(BlueprintType)
struct MOBILESTOREPURCHASESYSTEM_API FPurchaseReceiptInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|MobileStorePurchase")
	UShopItemData* ShopItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|MobileStorePurchase")
	FString ProductID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|MobileStorePurchase")
	FString TransactionID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|MobileStorePurchase")
	TMap<FString, FString> CustomData;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPurchaseEvent, bool, Success, FPurchaseReceiptInfo, Reciept);

DECLARE_MULTICAST_DELEGATE(FShopProductReceiveEvent);

UCLASS()
class MOBILESTOREPURCHASESYSTEM_API UManagerMobileStorePurchase : public UManager
{
	GENERATED_BODY()

public:

	friend IPlatformTypePurchase;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FPurchaseEvent OnPurchaseRestore;

	UPROPERTY(BlueprintAssignable, Category = "Shop")
	FPurchaseEvent OnPurchaseComplete;

	FShopProductReceiveEvent OnProductsReceived;

	UPROPERTY()
	UPurchaseProxyInterface* PurchaseInterface;

protected:

	// PlatformTypePurchase Implementation Object
	TUniquePtr<IPlatformTypePurchase> PlatformImpl = nullptr;

	TArray<FString> PendingProductIdRequests;
	TArray<FString> ProductIdRequestsInProgress;
	TMap<FString, TSharedPtr<FOnlineStoreOffer>> StoreProducts;

	// Interfaces
	IOnlineSubsystem* OnlineSubsystem = nullptr;
	IOnlineIdentityPtr OnlineIdentity;
	IOnlinePurchasePtr OnlinePurchase;

	TSharedPtr<const FUniqueNetId> UniqueNetId;
	//FOnlineInAppPurchaseTransactionPtr IOSPurchaseRequest;

public:

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RestorePurchases();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void FinalizePurchase(FPurchaseReceiptInfo PurchaseReceiptInfo);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestAllProducts();

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void RequestProductId(FString ProductId);

	UFUNCTION(BlueprintPure, Category = "Shop")
	UPurchaseProxyInterface* GetPurchaseInterface() const {return PurchaseInterface;}

	UFUNCTION(BlueprintPure, Category = "Shop")
	UShopItemData* FindShopItemByProductId(FString ProductId) const;

	virtual void InitManager() override;

	void InitPlatformInterface();

	TSharedPtr<const FUniqueNetId> GetUniqueNetId() const { return UniqueNetId; }

	TSharedPtr<FOnlineStoreOffer> GetProduct(FString ProductId) const;

	void StartPurchase(FString ProductID, bool Consumable);

	void ReceiveProductInfo(TSharedPtr<FOnlineStoreOffer> ProductInfo);
	void ProcessPurchase(FPurchaseInfoRaw PurchaseInfo);
	void ProcessPurchaseError(FString Error);

protected:

	void RequestProducts();
};
