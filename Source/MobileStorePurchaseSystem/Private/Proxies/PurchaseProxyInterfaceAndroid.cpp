// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Proxies/PurchaseProxyInterfaceAndroid.h"

#include "LogSystem.h"
#include "Module/MobileStorePurchaseSystemModule.h"
#include "Module/MobileStorePurchaseSystemSettings.h"

void UPurchaseProxyInterfaceAndroid::Purchase(FString ProductID)
{
	Super::Purchase(ProductID);
	
	UAndroidBillingHelper* Billing = UAndroidBillingHelper::Get();
	if(!Billing)
	{
		OnProductPurchaseError.Broadcast("No Billing");
		return;
	}

	Billing->OnPurchaseSuccess.AddUniqueDynamic(this, &UPurchaseProxyInterfaceAndroid::ProcessPurchase);
	Billing->OnPurchaseFail.AddUniqueDynamic(this, &UPurchaseProxyInterfaceAndroid::ProcessPurchaseFail);
	
	UAndroidBillingHelper::Get()->Purchase(ProductID);
}

void UPurchaseProxyInterfaceAndroid::RequestProducts(TArray<FString> ProductsID)
{
	Super::RequestProducts(ProductsID);

	DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
		LogMobileStorePurchaseSystem,
		"%i Store Products Requested",
	ProductsID.Num())
	
	if(UAndroidBillingHelper* Billing = UAndroidBillingHelper::Get())
	{
		DEBUG_MESSAGE(GetDefault<UMobileStorePurchaseSystemSettings>()->bShowDebugMessages,
			LogMobileStorePurchaseSystem,
			"Adnroid Billing Helper Products Request"
		)
		Billing->OnProductInfoReceive.AddUniqueDynamic(this, &UPurchaseProxyInterfaceAndroid::ReceiveProduct);
		Billing->RequestProducts(ProductsID);

		return;
	}

	LOG(LogMobileStorePurchaseSystem, "Adnroid Billing Helper Products Request Failed")
	OnProductPurchaseError.Broadcast("No Billing");
}

void UPurchaseProxyInterfaceAndroid::FinalizePurchase(const FPurchaseInfoRaw& PurchaseInfo)
{
	Super::FinalizePurchase(PurchaseInfo);

	if(UAndroidBillingHelper* Billing = UAndroidBillingHelper::Get())
	{
		FAndroidPurchaseInfo AndroidPurchaseInfo;

		AndroidPurchaseInfo.Token = PurchaseInfo.TransactionID;
		AndroidPurchaseInfo.ProductID = PurchaseInfo.ProductID;
		AndroidPurchaseInfo.Signature = PurchaseInfo.CustomData.FindChecked("Signature");
		AndroidPurchaseInfo.OrderID = PurchaseInfo.CustomData.FindChecked("OrderID");

		for (const TTuple<FString, FString>& Detail : PurchaseInfo.CustomData)
		{
			AndroidPurchaseInfo.Details.Add(Detail.Key, Detail.Value);
		}

		const bool Consume = PurchaseInfo.CustomData.FindChecked("FinalizeType") == "Consume";
		
		Billing->FinalizePurchase(AndroidPurchaseInfo, Consume);
	}
}

void UPurchaseProxyInterfaceAndroid::ProcessPurchase(FAndroidPurchaseInfo PurchaseInfo)
{
	LOG(LogMobileStorePurchaseSystem, "Process purchase: %s", *PurchaseInfo.ProductID)
	
	FPurchaseInfoRaw Info;
	Info.ProductID = PurchaseInfo.ProductID;
	Info.TransactionID = PurchaseInfo.Token;
	Info.CustomData.Add("Signature", PurchaseInfo.Signature);
	Info.CustomData.Add("OrderID", PurchaseInfo.OrderID);
	
	for (const TTuple<FString, FString>& Detail : PurchaseInfo.Details)
	{
		Info.CustomData.Add(Detail.Key, Detail.Value);
	}
	
	OnProductPurchased.Broadcast(Info);
}

void UPurchaseProxyInterfaceAndroid::ProcessPurchaseFail(FString PurchaseID, FString Error)
{
	LOG(LogMobileStorePurchaseSystem, "Process purchase failed: %s", *Error)
	
	OnProductPurchaseError.Broadcast(Error);
}

void UPurchaseProxyInterfaceAndroid::ReceiveProduct(const FAndroidProductInfo& ProductInfo)
{
	TSharedPtr<FOnlineStoreOffer> Offer = MakeShareable(new FOnlineStoreOffer);
	Offer->Title = FText::FromString(ProductInfo.Name);
	Offer->Description = FText::FromString(ProductInfo.Description);
	Offer->PriceText = FText::FromString(ProductInfo.FormattedPrice);
	Offer->NumericPrice = ProductInfo.MicrosPrice;
	Offer->RegularPrice = ProductInfo.MicrosPrice;
	Offer->CurrencyCode = ProductInfo.CurrencyCode;
	Offer->OfferId = ProductInfo.ProductID;
	
	OnProductReceive.Broadcast(Offer);
}