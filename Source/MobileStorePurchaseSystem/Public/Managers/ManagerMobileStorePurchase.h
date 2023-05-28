// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Managers/Manager.h"

#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "OnlineSubsystem.h"

#include "ManagerMobileStorePurchase.generated.h"

class UShopItemData;
class UMobileStorePurchaseShopItemData;
class UManagerMobileStorePurchase;

struct FPurchaseInfoRaw
{

	FString ProductID;

	FString TransactionID;

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

/** IPlatformTypePurchase interface */
class IPlatformTypePurchase
{
public:

	virtual ~IPlatformTypePurchase() = default;

	explicit IPlatformTypePurchase(UManagerMobileStorePurchase* InManager);

	virtual void Purchase(FString ProductID, bool Consumable) = 0;
	virtual void RestorePurchases() = 0;
	virtual void RequestProducts() = 0;

protected:

	UManagerMobileStorePurchase* Manager = nullptr;
	IOnlineSubsystem* OnlineSubsystem = nullptr;

	IOnlinePurchasePtr GetOnlinePurchase() const;
	TArray<FString>& GetPendingProductIdRequests() const;
	TArray<FString>& GetProductIdRequestsInProgress() const;
	TMap<FString, TSharedPtr<FOnlineStoreOffer>>& GetStoreProducts() const;
};

USTRUCT(BlueprintType)
struct MOBILESTOREPURCHASESYSTEM_API FPurchaseReceiptInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|MobileStorePurchase")
	UMobileStorePurchaseShopItemData* ShopItemData;

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

	friend class IPlatformTypePurchase;

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
	UMobileStorePurchaseShopItemData* FindShopItemByProductId(FString ProductId) const;

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
