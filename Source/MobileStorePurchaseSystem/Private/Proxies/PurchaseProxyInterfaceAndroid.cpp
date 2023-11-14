// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Proxies/PurchaseProxyInterfaceAndroid.h"

#include "Log.h"
#include "Log/Details/LocalLogCategory.h"
#include "Module/MobileStorePurchaseSystemModule.h"

DEFINE_LOG_CATEGORY_LOCAL(LogMobileStorePurchaseSystem);

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

	LOG(Display, "{} Store Products Requested", ProductsID.Num());
	
	if(UAndroidBillingHelper* Billing = UAndroidBillingHelper::Get())
	{
		LOG(Display, "Adnroid Billing Helper Products Request");
		Billing->OnProductInfoReceive.AddUniqueDynamic(this, &UPurchaseProxyInterfaceAndroid::ReceiveProduct);
		Billing->RequestProducts(ProductsID);

		return;
	}

	LOG(Display, "Adnroid Billing Helper Products Request Failed");
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
	LOG(Display, "Process purchase: {}", *PurchaseInfo.ProductID);
	
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
	LOG(Display, "Process purchase failed: {}", *Error);
	
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