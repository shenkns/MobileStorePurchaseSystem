// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

#include "Module/MobileStorePurchaseSystemModule.h"

#include "Module/MobileStorePurchaseSystemSettings.h"

#if UE_EDITOR
#include "ISettingsModule.h"
#include "Module/ShopSystemSettings.h"
#endif

IMPLEMENT_MODULE(FMobileStorePurchaseSystemModule, MobileStorePurchaseSystem);

MOBILESTOREPURCHASESYSTEM_API DEFINE_LOG_CATEGORY(LogMobileStorePurchaseSystem);

void FMobileStorePurchaseSystemModule::StartupModule()
{
#if UE_EDITOR
	RegisterSystemSettings();
#endif
}

void FMobileStorePurchaseSystemModule::ShutdownModule()
{
#if UE_EDITOR
	UnregisterSystemSettings();
#endif
}

#if UE_EDITOR
void FMobileStorePurchaseSystemModule::RegisterSystemSettings() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings(
			"Project",
			"Plugins",
			"Mobile Store Purchase System",
			FText::FromString(TEXT("Mobile Store Purchase System")),
			FText::FromString(TEXT("Configuration settings for Mobile Store Purchase System")),
			GetMutableDefault<UMobileStorePurchaseSystemSettings>()
		);
	}
}

void FMobileStorePurchaseSystemModule::UnregisterSystemSettings() const
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "Mobile Store Purchase System");
	}
}
#endif
