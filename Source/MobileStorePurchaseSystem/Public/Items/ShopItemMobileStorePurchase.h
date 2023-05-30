// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Items/ShopItem.h"

#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Managers/ManagerMobileStorePurchase.h"

#include "ShopItemMobileStorePurchase.generated.h"

class UPurchaseWidget;

UCLASS()
class MOBILESTOREPURCHASESYSTEM_API UShopItemMobileStorePurchase : public UShopItem
{
	GENERATED_BODY()
	
protected:

	UPROPERTY(BlueprintReadOnly, Category = "Shop|MobileStorePurchase")
	UPurchaseWidget* PurchaseWidget;

	TSharedPtr<FOnlineStoreOffer> StoreOfferInfo;

	bool bStoreInfoRecieved;

	FPurchaseReceipt PurchaseReceipt;

	UPROPERTY()
	FTimerHandle FinalizeTimer;
	
public:

	virtual void Init_Implementation() override;

	virtual void Buy_Implementation() override;

	virtual void Finish_Implementation() override;

	virtual int GetPrice_Implementation() const override;
	
	//virtual FText GetPriceText_Implementation() const override;

	//virtual int GetOldPrice_Implementation() const override;
	
	//virtual FText GetOldPriceText_Implementation() const override;
	
	//virtual bool HasDiscount_Implementation() const override;
	//virtual int GetDiscountPercent_Implementation() const override;
	
	virtual bool CanBeBought_Implementation() const override;

	UFUNCTION(BlueprintPure, Category="Shop|MobileStorePurchase")
	bool IsStoreInfoReady() const;

	UFUNCTION(BlueprintPure, Category = "Shop|MobileStorePurchase")
	UManagerMobileStorePurchase* GetMobileStorePurchaseManager() const;

	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category="Shop|MobileStorePurchase")
	FString GetProductID() const;

protected:

	UFUNCTION()
	void ProcessPurchaseComplete(bool Success, FPurchaseReceiptInfo Reciept);

	void CheckProduct();
	
private:

	UFUNCTION()
	void StartRealBuyProcess();

	void OpenPurchaseWidget();
	void ClosePurchaseWidget();
};
