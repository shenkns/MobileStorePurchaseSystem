// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

using UnrealBuildTool;
using System.IO;

public class MobileStorePurchaseSystem : ModuleRules
{
	public MobileStorePurchaseSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[] {
				"MobileStorePurchaseSystem/Public/"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"LogSystem",
				"DataSystem",
				"ManagersSystem",
				"ShopSystem",
				"SerializationSystem",
				"OnlineSubsystem",
				"Json",
				"UMG"
			}
		);
		
		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"LogSystem",
				"DataSystem",
				"ManagersSystem",
				"ShopSystem",
				"SerializationSystem",
				"OnlineSubsystem"
			}
		);
		
		if(Target.Platform == UnrealTargetPlatform.Android)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Launch"
				}
			);

			string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "MobileStorePurchaseSystem_UPL_Android.xml"));
		}
	}
}
