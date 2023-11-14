// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Items/ShopItemMobileStorePurchase.h"

#include "Log.h"
#include "ManagersSystem.h"
#include "Data/StoreShopCustomData.h"
#include "Data/ShopItemData.h"
#include "Managers/ShopManager.h"
#include "Managers/StatsManager.h"
#include "Module/MobileStorePurchaseSystemModule.h"
#include "Module/ShopSystemSettings.h"

#include "VaRestSubsystem.h"
#include "VaRestRequestJSON.h"
#include "VaRestTypes.h"
#include "VaRestJsonObject.h"
#include "Log/Details/LocalLogCategory.h"

DEFINE_LOG_CATEGORY_LOCAL(LogMobileStorePurchaseSystem);

void UShopItemMobileStorePurchase::Init_Implementation()
{
	LOG(Display, "SKU {}: Init", ShopData->Tag);

	const UManagersSystem* ManagersSystem = GetManagersSystem();
	if(!ManagersSystem) return;
	
	UManagerMobileStorePurchase* ManagerMobileStorePurchase =  ManagersSystem->GetManager<UManagerMobileStorePurchase>();
	if(!ManagerMobileStorePurchase) return;
	
	if(GetShopData<UShopItemData>() && GetShopData<UShopItemData>()->GetCustomData<UStoreShopCustomData>())
	{
		CheckProduct();

		if (!StoreOfferInfo)
		{
			ManagerMobileStorePurchase->OnProductsReceived.AddUObject(this, &UShopItemMobileStorePurchase::CheckProduct);
			
			LOG(Display, "Waiting {} Store Product For {} Shop Item", *GetProductID(), this);
		}
	}
	else
	{
		Super::Init_Implementation();
	}
}

bool UShopItemMobileStorePurchase::Buy_Implementation()
{
	if(GetShopData<UShopItemData>() && GetShopData<UShopItemData>()->GetCustomData<UStoreShopCustomData>())
	{
		OpenPurchaseWidget();

		FTimerHandle PurchaseTimer;

		GetWorld()->GetTimerManager().SetTimer(PurchaseTimer, this, &UShopItemMobileStorePurchase::StartRealBuyProcess, 1.f);

		return true;
	}

	return Super::Buy_Implementation();
}

void UShopItemMobileStorePurchase::VerifyPurchase_Implementation(const FString& TransactionID)
{
	LOG(Display, "Purchase verification started, item: {}, transaction: {}", GetShopData<UShopItemData>()->Tag, *TransactionID);
	
	UVaRestSubsystem* VaRest = GetVaRest();
	if(!VaRest) FinishPurchase(false);

	UVaRestRequestJSON* Request = VaRest->ConstructVaRestRequest();
	if(!Request) FinishPurchase(false);

	Request->SetVerb(EVaRestRequestVerb::POST);
	Request->SetContentType(EVaRestRequestContentType::json);

	Request->SetHeader(FString("Authorization"),
		FString::Printf(TEXT("Bearer %s"), *UShopSystemSettings::GetBackendAuthToken())
	);

	Request->GetRequestObject()->SetStringField(FString("tag"), GetShopData<UShopItemData>()->Tag.ToString());
	
#if WITH_EDITOR
	Request->GetRequestObject()->SetBoolField(FString("fakePurchase"), true);
#else
#if UE_BUILD_DEVELOPMENT
	if(const UMobileStorePurchaseSystemSettings* Settings = GetDefault<UMobileStorePurchaseSystemSettings>())
	{
		if(Settings->bFakeInAppPurchasesInDevBuild)
		{
			Request->GetRequestObject()->SetBoolField(FString("fakePurchase"), true);
		}
		else
		{
			Request->GetRequestObject()->SetStringField(FString("purchaseToken"), TransactionID);
		}
	}
#else
	Request->GetRequestObject()->SetStringField(FString("purchaseToken"), TransactionID);
#endif
#endif

	Request->OnRequestComplete.AddUniqueDynamic(this, &UShopItemMobileStorePurchase::OnPurchaseVerified);
	
	Request->ProcessURL(UShopSystemSettings::GetPurchaseVerificationUrl());

	LOG(Display, "Purchase verification request sent \n to: {}, \n with content: {}", *UShopSystemSettings::GetPurchaseVerificationUrl(), *Request->GetRequestObject()->EncodeJson());
}

int UShopItemMobileStorePurchase::GetPrice_Implementation() const
{
	if (GetShopData<UShopItemData>() && GetShopData<UShopItemData>()->GetCustomData<UStoreShopCustomData>())
	{
#if PLATFORM_ANDROID
		// We divide price by 100 because it include cents
		return bStoreInfoRecieved ? StoreOfferInfo->NumericPrice/1000000 : 0;
#else
		return bStoreInfoRecieved ? StoreOfferInfo->NumericPrice : 0;
#endif
	}
	else
	{
		return Super::GetPrice_Implementation();
	}
}

FText UShopItemMobileStorePurchase::GetPriceText_Implementation() const
{
	if(ShopData && ShopData->GetCustomData<UStoreShopCustomData>())
	{
		return bStoreInfoRecieved ? StoreOfferInfo->PriceText : FText();
	}
	else
	{
		return Super::GetPriceText_Implementation();
	}
}

bool UShopItemMobileStorePurchase::CanBeBought_Implementation() const
{
	if(GetShopData<UShopItemData>() && GetShopData<UShopItemData>()->GetCustomData<UStoreShopCustomData>())
	{
		if(FinalizeTimer.IsValid()) return false;

		if (GetShopData<UShopItemData>() && GetShopData<UShopItemData>()->GetCustomData<UStoreShopCustomData>())
		{
			return IsStoreInfoReady();
		}
		else
		{
			return Super::CanBeBought_Implementation();
		}
	}

	return Super::CanBeBought_Implementation();
}

bool UShopItemMobileStorePurchase::IsStoreInfoReady() const
{
#if PLATFORM_ANDROID || PLATFORM_IOS

#if UE_BUILD_SHIPPING
	return bStoreInfoRecieved;
#else
	return GetDefault<UMobileStorePurchaseSystemSettings>()->bFakeInAppPurchasesInDevBuild || bStoreInfoRecieved;
#endif
	
#else
	return true;
#endif
}

UManagerMobileStorePurchase* UShopItemMobileStorePurchase::GetMobileStorePurchaseManager() const
{
	if(!GetManagersSystem()) return nullptr;

	return GetManagersSystem()->GetManager<UManagerMobileStorePurchase>();
}

FString UShopItemMobileStorePurchase::GetProductID_Implementation() const
{
	if(const UShopItemData* StoreShopData = GetShopData<UShopItemData>())
	{
		if(const UStoreShopCustomData* StoreShopCustomData = StoreShopData->GetCustomData<UStoreShopCustomData>())
		{
			return StoreShopCustomData->ProductID;
		}
	}
	
	return FString();
}

void UShopItemMobileStorePurchase::ProcessPurchaseComplete(bool Success, FPurchaseReceiptInfo Reciept)
{
	LOG(Display, "Purchase complete: {}", *Reciept.ProductID);

	if(!GetMobileStorePurchaseManager()) return;
	
	GetMobileStorePurchaseManager()->OnPurchaseComplete.RemoveDynamic(this, &UShopItemMobileStorePurchase::ProcessPurchaseComplete);
	
	if(Success)
	{
		// TODO: Move platform finalizing after applying sku content
		GetMobileStorePurchaseManager()->FinalizePurchase(Reciept);
	}
	
	LOG(Display, "Finish SKU: {}", ShopData->Tag);

	if(Success)
	{
		if(const UShopSystemSettings* Settings = GetDefault<UShopSystemSettings>())
		{
			if(Settings->bEnableBackendPurchaseVerification && !GetShopData<UShopItemData>()->bSkipPurchaseVerification)
			{
				VerifyPurchase(Reciept.TransactionID);
				return;
			}
		}
	}

	FinishPurchase(Success);

	const UManagersSystem* ManagersSystem = GetManagersSystem();
	if(!ManagersSystem) return;

	UStatsManager* StatsManager = ManagersSystem->GetManager<UStatsManager>();
	if(!StatsManager) return;
	
	StatsManager->SaveStats();
}

void UShopItemMobileStorePurchase::CheckProduct()
{
	if (!GetMobileStorePurchaseManager() || bStoreInfoRecieved) return;

	StoreOfferInfo = GetMobileStorePurchaseManager()->GetProduct(GetProductID());
	
	if (StoreOfferInfo.IsValid())
	{
		bStoreInfoRecieved = true;

		LOG(Display, "SKU ""{}"" received store info: {} -- {}", ShopData->Tag, StoreOfferInfo->Title, StoreOfferInfo->PriceText);
	}
	else
	{
		LOG(Display, "SKU ""{}"" not valid", ShopData->Tag);
	}
	
}

void UShopItemMobileStorePurchase::StartRealBuyProcess()
{
	if(const UShopItemData* StoreShopData = GetShopData<UShopItemData>())
	{
		if(const UStoreShopCustomData* StoreShopCustomData = StoreShopData->GetCustomData<UStoreShopCustomData>())
		{
			LOG(Display, "SKU {}: Buy", ShopData->Tag);
	
	#if WITH_EDITOR
			if(const UShopSystemSettings* Settings = GetDefault<UShopSystemSettings>())
			{
				if(Settings->bEnableBackendPurchaseVerification)
				{
					VerifyPurchase();
				}
				else
				{
					FinishPurchase(true);
				}
			}
	#else
	#if UE_BUILD_DEVELOPMENT
			if(const UMobileStorePurchaseSystemSettings* Settings = GetDefault<UMobileStorePurchaseSystemSettings>())
			{
				if(Settings->bFakeInAppPurchasesInDevBuild)
				{
					if(const UShopSystemSettings* ShopSystemSettings = GetDefault<UShopSystemSettings>())
					{
						if(ShopSystemSettings->bEnableBackendPurchaseVerification)
						{
							VerifyPurchase();
						}
						else
						{
							FinishPurchase(true);
						}
					}
					
					return;
				}
			}
	#endif
			if(IsStoreInfoReady())
			{
				GetMobileStorePurchaseManager()->OnPurchaseComplete.AddDynamic(this, &UShopItemMobileStorePurchase::ProcessPurchaseComplete);
				GetMobileStorePurchaseManager()->StartPurchase(GetProductID(), StoreShopCustomData->bIsConsumable);
			}
			else
			{
				FinishPurchase(false);
			}
	#endif
		}
		else
		{
			Super::Buy_Implementation();
		}
	}
}
