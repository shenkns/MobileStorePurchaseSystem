// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "PlatformTypePurchases/PlatformTypePurchase.h"

#include "Managers/MobileStorePurchaseManager.h"
#include "Interfaces/OnlineStoreInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"

class FPlatformTypePurchaseIOS final : public IPlatformTypePurchase
{
public:

	FInAppPurchaseProductRequest LastPurchaseProductRequest;

	explicit FPlatformTypePurchaseIOS(UMobileStorePurchaseManager* InManager) : IPlatformTypePurchase(InManager), LastPurchaseProductRequest()
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		StoreInterfaceV1 = OnlineSubsystem->GetStoreInterface();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	virtual ~FIOSPurchase() override = default;

private:

	//start IPlatformTypePurchase
	virtual void Purchase(FString ProductID, bool Consumable) override
	{
		UE_LOG(LogTemp, Log, TEXT("SKU %s: Buy IOS"), *ProductID);

		if (!StoreInterfaceV1)
		{
			UE_LOG(LogTemp, Error, TEXT("No Store Interface to purchase for IOS!  %s"), *ProductID);
			return;
		}

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		LastPurchaseProductRequest = FInAppPurchaseProductRequest();
		LastPurchaseProductRequest.bIsConsumable = Consumable;
		LastPurchaseProductRequest.ProductIdentifier = ProductID;

		IOSInAppPurchaseCompleteDelegate = FOnInAppPurchaseCompleteDelegate::CreateRaw(this, &FPlatformTypePurchaseIOS::OnPurchaseCheckoutIOS);
		IOSInAppPurchaseCompleteDelegateHandle = StoreInterfaceV1->AddOnInAppPurchaseCompleteDelegate_Handle(IOSInAppPurchaseCompleteDelegate);

		IOSPurchaseRequest = MakeShareable(new FOnlineInAppPurchaseTransaction());
		FOnlineInAppPurchaseTransactionRef PurchaseRequestRef = IOSPurchaseRequest.ToSharedRef();

		UE_LOG(LogTemp, Log, TEXT("SKU %s: Checkout"), *ProductID);

		StoreInterfaceV1->BeginPurchase(LastPurchaseProductRequest, PurchaseRequestRef);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	virtual void RestorePurchases() override
	{
		if (!StoreInterfaceV1) return;

		TArray<FInAppPurchaseProductRequest> Consumables;

		TArray<UMetaDataAsset*> MetaDataAssets = UMetaManager::Get(Manager)->GetMetaDataAssets(UMetaShopItemDataAsset::StaticClass(), true);

		if (MetaDataAssets.Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("SKU No shop datas found to restore"));
			return;
		}

		for (UMetaDataAsset* DataAsset : MetaDataAssets)
		{
			if (UMetaShopItemDataAsset* ShopItemDataAsset = Cast<UMetaShopItemDataAsset>(DataAsset))
			{
				if (ShopItemDataAsset->IsStorePurchase && !ShopItemDataAsset->IsConsumable)
				{
					UE_LOG(LogTemp, Log, TEXT("SKU: Add pending to request - %s"), *ShopItemDataAsset->Tag.ToString());

					FInAppPurchaseProductRequest RestoreRequest;

					RestoreRequest.bIsConsumable = false;
					RestoreRequest.ProductIdentifier = ShopItemDataAsset->ProductID;

					Consumables.Add(RestoreRequest);
				}
			}
		}

		PRAGMA_DISABLE_DEPRECATION_WARNINGS

		// Register the completion callback
		IOSInAppPurchaseRestoreCompleteDelegate = FOnInAppPurchaseCompleteDelegate::CreateRaw(this, &FPlatformTypePurchaseIOS::ProcessRestoreQueryIOS);
		IOSInAppPurchaseRestoreCompleteDelegateHandle =
			StoreInterfaceV1->AddOnInAppPurchaseRestoreCompleteDelegate_Handle(IOSInAppPurchaseRestoreCompleteDelegate);

		// Set-up, and trigger the transaction through the store interface
		IOSRestoreReadObject = MakeShareable(new FOnlineInAppPurchaseRestoreRead());
		FOnlineInAppPurchaseRestoreReadRef ReadObjectRef = IOSRestoreReadObject.ToSharedRef();
		StoreInterfaceV1->RestorePurchases(Consumables, ReadObjectRef);

		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	virtual void RequestProducts() override
	{
		if (!StoreInterfaceV1) return;

		TArray<FString> OfferIds = GetPendingProductIdRequests();

		for (const FString& Id : OfferIds)
		{
			UE_LOG(LogTemp, Log, TEXT("SKU: Request product - %s"), *Id);
		}

		GetProductIdRequestsInProgress() = GetPendingProductIdRequests();
		GetPendingProductIdRequests().Empty();

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		IOSInAppPurchaseReadCompleteDelegate = FOnQueryForAvailablePurchasesCompleteDelegate::CreateRaw(this, &FPlatformTypePurchaseIOS::WritePurchaseQueryIOS);
		IOSInAppPurchaseReadCompleteDelegateHandle =
			StoreInterfaceV1->AddOnQueryForAvailablePurchasesCompleteDelegate_Handle(IOSInAppPurchaseReadCompleteDelegate);

		IOSReadObject = MakeShareable(new FOnlineProductInformationRead());
		FOnlineProductInformationReadRef ReadObjectRef = IOSReadObject.ToSharedRef();

		StoreInterfaceV1->QueryForAvailablePurchases(OfferIds, ReadObjectRef);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	//end IPlatformTypePurchase

	// IOS StoreInterface
	IOnlineStorePtr StoreInterfaceV1;

	// IOS Query
	FOnQueryForAvailablePurchasesCompleteDelegate IOSInAppPurchaseReadCompleteDelegate;
	FDelegateHandle IOSInAppPurchaseReadCompleteDelegateHandle;
	FOnlineProductInformationReadPtr IOSReadObject;

	// IOS Purchase
	FOnInAppPurchaseCompleteDelegate IOSInAppPurchaseCompleteDelegate;
	FDelegateHandle IOSInAppPurchaseCompleteDelegateHandle;
	FOnlineInAppPurchaseTransactionPtr IOSPurchaseRequest;

	// IOS Restore
	FOnInAppPurchaseRestoreCompleteDelegate IOSInAppPurchaseRestoreCompleteDelegate;
	FDelegateHandle IOSInAppPurchaseRestoreCompleteDelegateHandle;
	FOnlineInAppPurchaseRestoreReadPtr IOSRestoreReadObject;

private:

	void ProcessRestoreQueryIOS(EInAppPurchaseState::Type CompletionState)
	{
		if (CompletionState != EInAppPurchaseState::Type::Success
			|| CompletionState == EInAppPurchaseState::Type::Restored
			|| CompletionState == EInAppPurchaseState::Type::AlreadyOwned)
		{
			UE_LOG(LogTemp, Log, TEXT("SKU: Restore FAIL"));

			IOSRestoreReadObject = nullptr;

			return;
		}

		UE_LOG(LogTemp, Log, TEXT("Restore out receipts: %i"), IOSRestoreReadObject->ProvidedRestoreInformation.Num());

		for (const FInAppPurchaseRestoreInfo& RestoreInfo : IOSRestoreReadObject->ProvidedRestoreInformation)
		{
			UE_LOG(LogTemp, Log, TEXT("%s"), *RestoreInfo.Identifier);

			if (UMetaShopItemDataAsset* ShopItemDataAsset = Manager->FindShopItemByProductId(RestoreInfo.Identifier))
			{
				FPurchaseReceiptInfo RestoreReceipt;
				RestoreReceipt.ProductID = RestoreInfo.Identifier;
				RestoreReceipt.TransactionID = RestoreInfo.TransactionIdentifier;
				RestoreReceipt.ShopItemData = ShopItemDataAsset;
				RestoreReceipt.CustomData.Add("ReceiptData", RestoreInfo.ReceiptData);

				Manager->OnPurchaseRestore.Broadcast(true,RestoreReceipt);
			}
		}

		IOSRestoreReadObject = nullptr;
	}

	void WritePurchaseQueryIOS(bool bWasSuccessful)
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		StoreInterfaceV1->ClearOnQueryForAvailablePurchasesCompleteDelegate_Handle(IOSInAppPurchaseReadCompleteDelegateHandle);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		if (!bWasSuccessful) return;

		GetStoreProducts().Reserve(IOSReadObject->ProvidedProductInformation.Num());
		for (const FInAppPurchaseProductInfo& Info : IOSReadObject->ProvidedProductInformation)
		{
			TSharedPtr<FOnlineStoreOffer> Offer = MakeShareable<FOnlineStoreOffer>(new FOnlineStoreOffer());
			Offer->Description = FText::FromString(Info.DisplayDescription);
			Offer->Title = FText::FromString(Info.DisplayName);
			Offer->RegularPriceText = FText::FromString(Info.DisplayPrice);
			Offer->RegularPrice = Info.RawPrice;
			Offer->PriceText = FText::FromString(Info.DisplayPrice);
			Offer->NumericPrice = Info.RawPrice;
			Offer->CurrencyCode = Info.CurrencyCode;

			GetStoreProducts().Add(Info.Identifier, Offer);

			UE_LOG(LogTemp, Log, TEXT("SKU: Receive product - %s"), *Info.Identifier);
		}

		GetProductIdRequestsInProgress().Empty();

		Manager->OnProductsReceived.Broadcast();

		RequestProducts();
	}

	void OnPurchaseCheckoutIOS(EInAppPurchaseState::Type CompletionState)
	{
		UE_LOG(LogTemp, Log, TEXT("SKU Purchase Complete"));

		if ((CompletionState == EInAppPurchaseState::Type::Success
			|| CompletionState == EInAppPurchaseState::Type::Restored
			|| CompletionState == EInAppPurchaseState::Type::AlreadyOwned
			)
			&& IOSPurchaseRequest)
		{
			UE_LOG(LogTemp, Log, TEXT("SKU Purchase SUCCESS: %s"), *IOSPurchaseRequest->ProvidedProductInformation.Identifier);

			FPurchaseReceiptInfo PurchaseReceipt;
			PurchaseReceipt.ProductID = IOSPurchaseRequest->ProvidedProductInformation.Identifier;
			PurchaseReceipt.TransactionID = IOSPurchaseRequest->ProvidedProductInformation.TransactionIdentifier;
			PurchaseReceipt.ShopItemData = Manager->FindShopItemByProductId(PurchaseReceipt.ProductID);
			PurchaseReceipt.CustomData.Add("ReceiptData", IOSPurchaseRequest->ProvidedProductInformation.ReceiptData);
			
			Manager->OnPurchaseComplete.Broadcast(true, PurchaseReceipt);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("SKU Purchase FAIL"));
			
			if(IOSPurchaseRequest)
			{
				UE_LOG(LogTemp, Log, TEXT("SKU Purchase fail broadcast with request"));
				Manager->OnPurchaseComplete.Broadcast(false, FPurchaseReceiptInfo());
			}
			else
			{
				Manager->OnPurchaseComplete.Broadcast(
					false, FPurchaseReceiptInfo());
			}
		}

		IOSPurchaseRequest = nullptr;

		LastPurchaseProductRequest = FInAppPurchaseProductRequest();
	}
};
