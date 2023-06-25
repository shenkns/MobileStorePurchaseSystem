// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Items/ShopItemMobileStorePurchase.h"

#include "LogSystem.h"
#include "ManagersSystem.h"
#include "Blueprint/UserWidget.h"
#include "Data/StoreShopCustomData.h"
#include "Data/ShopItemData.h"
#include "Kismet/GameplayStatics.h"
#include "Managers/ShopManager.h"
#include "Module/MobileStorePurchaseSystemModule.h"
#include "Module/MobileStorePurchaseSystemSettings.h"
#include "Widgets/PurchaseWidget.h"

void UShopItemMobileStorePurchase::Init_Implementation()
{
	DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
		LogMobileStorePurchaseSystem,
		"SKU %s: Init",
		*ShopData->GetName()
	)

	const UManagersSystem* ManagersSystem = GetManagersSystem();
	if(!ManagersSystem) return;
	
	UManagerMobileStorePurchase* ManagerMobileStorePurchase =  ManagersSystem->GetManager<UManagerMobileStorePurchase>();
	if(!ManagerMobileStorePurchase) return;
	
	if (GetShopData<UShopItemData>() && GetShopData<UShopItemData>()->GetCustomData<UStoreShopCustomData>())
	{
		CheckProduct();

		if (!StoreOfferInfo)
		{
			ManagerMobileStorePurchase->OnProductsReceived.AddUObject(this, &UShopItemMobileStorePurchase::CheckProduct);
			
			DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
				LogMobileStorePurchaseSystem,
				"Waiting %s Store Product For %s Shop Item",
				*GetProductID(),
				*GetName()
			)
		}
	}
}

bool UShopItemMobileStorePurchase::Buy_Implementation()
{
	OpenPurchaseWidget();

	FTimerHandle PurchaseTimer;

	GetWorld()->GetTimerManager().SetTimer(PurchaseTimer, this, &UShopItemMobileStorePurchase::StartRealBuyProcess, 1.f);

	return true;
}

void UShopItemMobileStorePurchase::Finish_Implementation()
{
	Super::Finish_Implementation();

	ClosePurchaseWidget();
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

bool UShopItemMobileStorePurchase::CanBeBought_Implementation() const
{
	if (!ShopData) return false;

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
	DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
		LogMobileStorePurchaseSystem,
		"Purchase complete: %s",
		*Reciept.ProductID
	);

	if(!GetMobileStorePurchaseManager()) return;
	
	GetMobileStorePurchaseManager()->OnPurchaseComplete.RemoveDynamic(this, &UShopItemMobileStorePurchase::ProcessPurchaseComplete);
	
	if(Success)
	{
		// TODO: Move platform finalizing after applying sku content
		GetMobileStorePurchaseManager()->FinalizePurchase(Reciept);
	}
	
	DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
		LogMobileStorePurchaseSystem,
		"Finish SKU: %s",
		*ShopData->Tag.ToString()
	);

	FinishPurchase(Success);
}

void UShopItemMobileStorePurchase::CheckProduct()
{
	if (!GetMobileStorePurchaseManager() || bStoreInfoRecieved) return;

	StoreOfferInfo = GetMobileStorePurchaseManager()->GetProduct(GetProductID());
	
	if (StoreOfferInfo.IsValid())
	{
		bStoreInfoRecieved = true;

		DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
			LogMobileStorePurchaseSystem,
			"SKU ""%s"" received store info: %s -- %s",
			*ShopData->Tag.ToString(),
			*StoreOfferInfo->Title.ToString(),
			*StoreOfferInfo->PriceText.ToString()
		);
	}
	else
	{
		DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
			LogMobileStorePurchaseSystem,
			"SKU ""%s"" not valid",
			*ShopData->Tag.ToString()
		);
	}
	
}

void UShopItemMobileStorePurchase::OpenPurchaseWidget()
{
	PurchaseWidget = CreateWidget<UPurchaseWidget>(UGameplayStatics::GetPlayerController(this, 0),
		GetDefault<UMobileStorePurchaseSystemSettings>()->PurchaseWidgetClass
	);
	
	PurchaseWidget->Show();
}

void UShopItemMobileStorePurchase::ClosePurchaseWidget()
{
	if(PurchaseWidget)
	{
		PurchaseWidget->Hide();
	}
}

void UShopItemMobileStorePurchase::StartRealBuyProcess()
{
	if(const UShopItemData* StoreShopData = GetShopData<UShopItemData>())
	{
		if(const UStoreShopCustomData* StoreShopCustomData = StoreShopData->GetCustomData<UStoreShopCustomData>())
		{
			DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
				LogMobileStorePurchaseSystem, "SKU %s: Buy",
				*ShopData->GetName()
			)
	
	#if WITH_EDITOR
			FinishPurchase(true);
	#else
	#if UE_BUILD_DEVELOPMENT
			if(const UMobileStorePurchaseSystemSettings* Settings = GetDefault<UMobileStorePurchaseSystemSettings>())
			{
				if(Settings->bFakeInAppPurchasesInDevBuild)
				{
					FinishPurchase(true);
					
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
