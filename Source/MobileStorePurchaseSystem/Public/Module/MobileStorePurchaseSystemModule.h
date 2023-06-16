// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Modules/ModuleManager.h"

MOBILESTOREPURCHASESYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogMobileStorePurchaseSystem, All, Log);

class UAndroidBillingHelper;

class FMobileStorePurchaseSystemModule : public IModuleInterface
{
private:

	UAndroidBillingHelper* AndroidBillingHelper;
	
public:

	FMobileStorePurchaseSystemModule():AndroidBillingHelper(nullptr){}

	UAndroidBillingHelper* GetAndroidBillingHelper() const { return AndroidBillingHelper; }

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:

#if UE_EDITOR
	void RegisterSystemSettings() const;
	void UnregisterSystemSettings() const;
#endif
};
