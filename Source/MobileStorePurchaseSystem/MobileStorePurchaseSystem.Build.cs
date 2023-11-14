// Copyright shenkns Mobile Store Purchase System Developed With Unreal Engine. All Rights Reserved 2023.

using UnrealBuildTool;
using System.IO;

public class MobileStorePurchaseSystem : ModuleRules
{
	public MobileStorePurchaseSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp17;

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
				"SaveLoadSystem",
				"VaRest"
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
				"OnlineSubsystem",
				"VaRest"
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
