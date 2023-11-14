// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Managers/ManagerMobileStorePurchase.h"

#include "Log.h"
#include "ManagersSystem.h"
#include "Data/ShopItemData.h"
#include "Data/StoreShopCustomData.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Managers/DataManager.h"
#include "Module/MobileStorePurchaseSystemModule.h"
#include "Module/MobileStorePurchaseSystemSettings.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Log/Details/LocalLogCategory.h"
#include "PlatformTypePurchases/PlatformTypePurchase.h"

#if PLATFORM_IOS
#include "PlatformTypePurchases/PlatformTypePurchaseIOS.h"
#endif

DEFINE_LOG_CATEGORY_LOCAL(LogMobileStorePurchaseSystem);

void UManagerMobileStorePurchase::InitManager()
{
	Super::InitManager();
	
	InitPlatformInterface();
	RequestAllProducts();

	OnlineSubsystem = IOnlineSubsystem::GetByPlatform();
	if (!OnlineSubsystem) return;

	OnlineIdentity = OnlineSubsystem->GetIdentityInterface();
	if (!OnlineIdentity) return;

	UniqueNetId = OnlineIdentity->GetUniquePlayerId(0);
	OnlinePurchase = OnlineSubsystem->GetPurchaseInterface();

// TODO: Move IOS implementation to new interface
#if PLATFORM_IOS
	PlatformImpl = MakeUnique<FPlatformTypePurchaseIOS>(this);

	if(!PlatformImpl.IsValid())
	{
		// todo PlatformImpl Is Not Valid
	}
#endif
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
			
			LOG(Display,
				"{} Platform Purchase Interface Initializaed: {}",
				ProxyClass->LoadSynchronous(),
				PurchaseInterface
			);
			
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
	LOG(Display, "Trying To Request Store Products");
	
	const UMobileStorePurchaseSystemSettings* Settings = GetDefault<UMobileStorePurchaseSystemSettings>();
	if(!Settings) return;

	for (FString ProductID : Settings->StoreProductIDs)
	{
		LOG(Display, "Add pending ProductID to request - {}", *ProductID);

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

UShopItemData* UManagerMobileStorePurchase::FindShopItemByProductId(FString ProductId) const
{
	const UManagersSystem* ManagersSystem = GetManagerSystem();
	if(!ManagersSystem) return nullptr;

	const UDataManager* DataManager = ManagersSystem->GetManager<UDataManager>();
	if(!DataManager) return nullptr;
	
	TArray<UShopItemData*> DataAssets = DataManager->GetDataAssets<UShopItemData>();

	for (UShopItemData* Data : DataAssets)
	{
		if(Data)
		{
			if(const UStoreShopCustomData* StoreShopCustomData = Data->GetCustomData<UStoreShopCustomData>())
			{
				if(StoreShopCustomData->ProductID == ProductId) 
				{
					return Data;
				}
			}
		}
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
	LOG(Display, "Finalize purchase: {}", *PurchaseReceiptInfo.ProductID);
	
	if(PurchaseInterface)
	{
		FPurchaseInfoRaw PurchaseInfoRaw;
		PurchaseInfoRaw.ProductID = PurchaseReceiptInfo.ProductID;
		PurchaseInfoRaw.TransactionID = PurchaseReceiptInfo.TransactionID;
		PurchaseInfoRaw.CustomData = PurchaseReceiptInfo.CustomData;

		// We need to consume or acknowledge, so we need to place info in custom data
		PurchaseInfoRaw.CustomData.Add(
			"FinalizeType",
			(PurchaseReceiptInfo.ShopItemData && PurchaseReceiptInfo.ShopItemData->GetCustomData<UStoreShopCustomData>()) ? 
				PurchaseReceiptInfo.ShopItemData->GetCustomData<UStoreShopCustomData>()->bIsConsumable ? 
				"Consume" : 
				"Acknowledge": 
				"Consume"
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
	
	if (PlatformImpl)
	{
		PlatformImpl->RequestProducts();
	}
}

void UManagerMobileStorePurchase::ReceiveProductInfo(TSharedPtr<FOnlineStoreOffer> ProductInfo)
{
	LOG(Display, "Product info received: {} -- {}", *ProductInfo->OfferId, *ProductInfo->PriceText.ToString());
	
	StoreProducts.Add(ProductInfo->OfferId, ProductInfo);
	
	OnProductsReceived.Broadcast();
}

void UManagerMobileStorePurchase::ProcessPurchase(FPurchaseInfoRaw PurchaseInfo)
{
	LOG(Display, "Process purchase in manager: {}", *PurchaseInfo.ProductID);
	
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
