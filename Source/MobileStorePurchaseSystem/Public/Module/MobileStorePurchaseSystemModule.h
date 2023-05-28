// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#pragma once

#include "Modules/ModuleManager.h"

MOBILESTOREPURCHASESYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogMobileStorePurchaseSystem, All, Log);

class FMobileStorePurchaseSystemModule : public IModuleInterface
{
public:

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:

#if UE_EDITOR
	void RegisterSystemSettings() const;
	void UnregisterSystemSettings() const;
#endif
};
