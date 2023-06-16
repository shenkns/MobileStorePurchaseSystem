// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"

class UManagerMobileStorePurchase;

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