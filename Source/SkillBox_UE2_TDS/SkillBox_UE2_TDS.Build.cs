// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SkillBox_UE2_TDS : ModuleRules
{
	public SkillBox_UE2_TDS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "NavigationSystem", "AIModule", "PhysicsCore" });
    }
}
