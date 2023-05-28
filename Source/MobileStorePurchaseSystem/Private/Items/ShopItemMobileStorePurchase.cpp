// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Items/ShopItemMobileStorePurchase.h"

#include "LogSystem.h"
#include "ManagersSystem.h"
#include "Blueprint/UserWidget.h"
#include "Data/MobileStorePurchaseShopItemData.h"
#include "Data/ShopItemData.h"
#include "Managers/ShopManager.h"
#include "Module/MobileStorePurchaseSystemModule.h"
#include "Module/MobileStorePurchaseSystemSettings.h"
#include "Widgets/PurchaseWidget.h"

void UShopItemMobileStorePurchase::Init_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("SKU %s: Init"), *ShopData->GetName());

	const UManagersSystem* ManagersSystem = GetManagersSystem();
	if(!ManagersSystem) return;
	
	UManagerMobileStorePurchase* ManagerMobileStorePurchase =  ManagersSystem->GetManager<UManagerMobileStorePurchase>();
	if(!ManagerMobileStorePurchase) return;
	
	if (ShopData && Cast<UMobileStorePurchaseShopItemData>(ShopData))
	{
		CheckProduct();

		if (!StoreOfferInfo)
		{
			ManagerMobileStorePurchase->OnProductsReceived.AddUObject(this, &UShopItemMobileStorePurchase::CheckProduct);
		}
	}
}

void UShopItemMobileStorePurchase::Buy_Implementation()
{
	OpenPurchaseWidget();

	StartRealBuyProcess();
}

void UShopItemMobileStorePurchase::Finish_Implementation()
{
	Super::Finish_Implementation();

	ClosePurchaseWidget();
}

int UShopItemMobileStorePurchase::GetPrice_Implementation() const
{
	if (ShopData && Cast<UMobileStorePurchaseShopItemData>(ShopData))
	{
#if PLATFORM_ANDROID
		// We divide price by 100 because it include cents
		return StoreInfoReceived ? StoreOfferInfo->NumericPrice/1000000 : 0;
#else
		return StoreInfoReceived ? StoreOfferInfo->NumericPrice : 0;
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

	if (Cast<UMobileStorePurchaseShopItemData>(ShopData))
	{
#if WITH_EDITOR
		return true;
#endif
		
		return IsStoreInfoReady();
	}
	else
	{
		return Super::CanBeBought_Implementation();
	}
}

bool UShopItemMobileStorePurchase::IsStoreInfoReady() const
{
#if WITH_EDITOR
	return true;
#else
	return StoreInfoReceived;
#endif
}

UManagerMobileStorePurchase* UShopItemMobileStorePurchase::GetMobileStorePurchaseManager() const
{
	if(!GetManagersSystem()) return nullptr;

	return GetManagersSystem()->GetManager<UManagerMobileStorePurchase>();
}

FString UShopItemMobileStorePurchase::GetProductID_Implementation() const
{
	if(!ShopData) return FString();

	if(const UMobileStorePurchaseShopItemData* MobileStorePurchaseShopItemData = Cast<UMobileStorePurchaseShopItemData>(ShopData))
	{
		return MobileStorePurchaseShopItemData->ProductID;
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
	if (!GetMobileStorePurchaseManager() || StoreInfoReceived) return;

	StoreOfferInfo = GetMobileStorePurchaseManager()->GetProduct(GetProductID());
	
	if (StoreOfferInfo.IsValid())
	{
		StoreInfoReceived = true;

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
	PurchaseWidget = CreateWidget<UPurchaseWidget>(GetWorld(), GetDefault<UMobileStorePurchaseSystemSettings>()->PurchaseWidgetClass);
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
	if (ShopData && Cast<UMobileStorePurchaseShopItemData>(ShopData))
	{
		UE_LOG(LogTemp, Log, TEXT("SKU %s: Buy"), *ShopData->GetName());

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
			GetMobileStorePurchaseManager()->OnPurchaseComplete.AddDynamic(this, &UMetaShopMobilePurchaseSKU::ProcessPurchaseComplete);
			GetMobileStorePurchaseManager()->StartPurchase(GetProductID(), ShopData->IsConsumable);
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
