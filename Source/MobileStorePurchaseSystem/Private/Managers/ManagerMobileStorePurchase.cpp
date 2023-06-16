// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Managers/ManagerMobileStorePurchase.h"

#include "LogSystem.h"
#include "ManagersSystem.h"
#include "Data/MobileStorePurchaseShopItemData.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Managers/DataManager.h"
#include "Module/MobileStorePurchaseSystemModule.h"
#include "Module/MobileStorePurchaseSystemSettings.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "PlatformTypePurchases/PlatformTypePurchase.h"

#if PLATFORM_IOS
	#include "MetaManager/Controllers/ShopController/IOSPurchase.h"
#endif

void UManagerMobileStorePurchase::InitManager()
{
	Super::InitManager();
	
	InitPlatformInterface();

	OnlineSubsystem = IOnlineSubsystem::GetByPlatform();
	if (!OnlineSubsystem) return;

	OnlineIdentity = OnlineSubsystem->GetIdentityInterface();
	if (!OnlineIdentity) return;

	UniqueNetId = OnlineIdentity->GetUniquePlayerId(0);
	OnlinePurchase = OnlineSubsystem->GetPurchaseInterface();

// TODO: Move IOS implementation to new interface
#if PLATFORM_IOS
	PlatformImpl = MakeUnique<class FIOSPurchase>(this);
#endif

	if(!PlatformImpl.IsValid())
	{
		// todo PlatformImpl Is Not Valid
	}
}

void UManagerMobileStorePurchase::InitPlatformInterface()
{
	if(const UMobileStorePurchaseSystemSettings* Settings = GetDefault<UMobileStorePurchaseSystemSettings>())
	{
		if(const TSoftClassPtr<UPurchaseProxyInterface>* ProxyClass =
			Settings->PlatformsPurchaseInterfaceClasses.Find(FPlatformProperties::IniPlatformName()))
		{
			PurchaseInterface = NewObject<UPurchaseProxyInterface>(this, ProxyClass->LoadSynchronous());

			PurchaseInterface->OnProductReceive.AddUObject(this, &UManagerMobileStorePurchase::ReceiveProductInfo);
			PurchaseInterface->OnProductPurchased.AddUObject(this, &UManagerMobileStorePurchase::ProcessPurchase);
			PurchaseInterface->OnProductPurchaseError.AddUObject(this, &UManagerMobileStorePurchase::ProcessPurchaseError);
			
			return;
		}
	}
}

TSharedPtr<FOnlineStoreOffer> UManagerMobileStorePurchase::GetProduct(FString ProductId) const
{
	if (const TSharedPtr<FOnlineStoreOffer>* ProductPtr = StoreProducts.Find(ProductId))
	{
		return *ProductPtr;
	}
	return nullptr;
}

void UManagerMobileStorePurchase::RequestAllProducts()
{
	const UMobileStorePurchaseSystemSettings* Settings = GetDefault<UMobileStorePurchaseSystemSettings>();
	checkf(Settings, TEXT("Not able to get settings"));

	for (FString ProductID : Settings->StoreProductIDs)
	{
		UE_LOG(LogTemp, Log, TEXT("Add pending ProductID to request - %s"), *ProductID);

		PendingProductIdRequests.Add(ProductID);
	}

	RequestProducts();
}

void UManagerMobileStorePurchase::RequestProductId(FString ProductId)
{
	PendingProductIdRequests.Add(ProductId);

	if (ProductIdRequestsInProgress.Num() <= 0)
	{
		RequestProducts();
	}
}

UMobileStorePurchaseShopItemData* UManagerMobileStorePurchase::FindShopItemByProductId(FString ProductId) const
{
	const UManagersSystem* ManagersSystem = GetManagerSystem();
	if(!ManagersSystem) return nullptr;

	const UDataManager* DataManager = ManagersSystem->GetManager<UDataManager>();
	if(!DataManager) return nullptr;
	
	TArray<UMobileStorePurchaseShopItemData*> DataAssets = DataManager->GetDataAssets<UMobileStorePurchaseShopItemData>();

	for (UMobileStorePurchaseShopItemData* Data : DataAssets)
	{
		if (Data && Data->ProductID == ProductId) return Data;
	}
	
	return nullptr;
}

void UManagerMobileStorePurchase::StartPurchase(FString ProductID, bool Consumable)
{
	if(PurchaseInterface)
	{
		PurchaseInterface->Purchase(ProductID);
		return;
	}
	if(PlatformImpl)
	{
		PlatformImpl->Purchase(ProductID, Consumable);
	}
}
void UManagerMobileStorePurchase::RestorePurchases()
{
	if(PlatformImpl)
	{
		PlatformImpl->RestorePurchases();
	}
}

void UManagerMobileStorePurchase::FinalizePurchase(FPurchaseReceiptInfo PurchaseReceiptInfo)
{
	DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>(),
		LogMobileStorePurchaseSystem,
		"Finalize purchase: %s",
		*PurchaseReceiptInfo.ProductID
	);
	
	if(PurchaseInterface)
	{
		FPurchaseInfoRaw PurchaseInfoRaw;
		PurchaseInfoRaw.ProductID = PurchaseReceiptInfo.ProductID;
		PurchaseInfoRaw.TransactionID = PurchaseReceiptInfo.TransactionID;
		PurchaseInfoRaw.CustomData = PurchaseReceiptInfo.CustomData;

		// We need to consume or acknowledge, so we need to place info in custom data
		PurchaseInfoRaw.CustomData.Add(
			"FinalizeType",
			PurchaseReceiptInfo.ShopItemData ? PurchaseReceiptInfo.ShopItemData->bIsConsumable ? "Consume" : "Acknowledge": "Consume"
		);
		
		PurchaseInterface->FinalizePurchase(PurchaseInfoRaw);
		
		return;
	}

	if(OnlinePurchase) OnlinePurchase->FinalizePurchase(*UniqueNetId, PurchaseReceiptInfo.TransactionID);
}

void UManagerMobileStorePurchase::RequestProducts()
{
	if(PendingProductIdRequests.Num() <= 0) return;
	
	if(PurchaseInterface)
	{
		PurchaseInterface->RequestProducts(PendingProductIdRequests);
		return;
	}
	
	if (PlatformImpl && PendingProductIdRequests.Num() > 0)
	{
		PlatformImpl->RequestProducts();
	}
}

void UManagerMobileStorePurchase::ReceiveProductInfo(TSharedPtr<FOnlineStoreOffer> ProductInfo)
{
	DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
		LogMobileStorePurchaseSystem,
		"Product info received: %s -- %s",
		*ProductInfo->OfferId,
		*ProductInfo->PriceText.ToString()
	);
	
	StoreProducts.Add(ProductInfo->OfferId, ProductInfo);
	
	OnProductsReceived.Broadcast();
}

void UManagerMobileStorePurchase::ProcessPurchase(FPurchaseInfoRaw PurchaseInfo)
{
	DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
		LogMobileStorePurchaseSystem,
		"Process purchase in manager: %s",
		*PurchaseInfo.ProductID
	);
	
	FPurchaseReceiptInfo PurchaseReceiptInfo;
	PurchaseReceiptInfo.ProductID = PurchaseInfo.ProductID;
	PurchaseReceiptInfo.TransactionID = PurchaseInfo.TransactionID;
	PurchaseReceiptInfo.CustomData = PurchaseInfo.CustomData;
	PurchaseReceiptInfo.ShopItemData = FindShopItemByProductId(PurchaseInfo.ProductID);
	OnPurchaseComplete.Broadcast(true, PurchaseReceiptInfo);
}

void UManagerMobileStorePurchase::ProcessPurchaseError(FString Error)
{
	OnPurchaseComplete.Broadcast(false, FPurchaseReceiptInfo());
}
