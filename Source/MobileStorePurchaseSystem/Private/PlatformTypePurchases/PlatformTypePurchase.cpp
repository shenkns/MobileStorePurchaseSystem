// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "PlatformTypePurchases/PlatformTypePurchase.h"

#include "Managers/ManagerMobileStorePurchase.h"

IPlatformTypePurchase::IPlatformTypePurchase(UManagerMobileStorePurchase* InManager) : Manager(InManager)
{
	OnlineSubsystem = IOnlineSubsystem::GetByPlatform();
	check(Manager);
	check(OnlineSubsystem);
}

IOnlinePurchasePtr IPlatformTypePurchase::GetOnlinePurchase() const
{
	return Manager->OnlinePurchase;
}

TArray<FString>& IPlatformTypePurchase::GetPendingProductIdRequests() const
{
	return Manager->PendingProductIdRequests;
}

TArray<FString>& IPlatformTypePurchase::GetProductIdRequestsInProgress() const
{
	return Manager->ProductIdRequestsInProgress;
}

TMap<FString, TSharedPtr<FOnlineStoreOffer>>& IPlatformTypePurchase::GetStoreProducts() const
{
	return Manager->StoreProducts;
}
