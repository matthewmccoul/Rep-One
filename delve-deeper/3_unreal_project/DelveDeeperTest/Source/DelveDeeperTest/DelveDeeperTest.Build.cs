using UnrealBuildTool;

public class DelveDeeperTest : ModuleRules
{
	public DelveDeeperTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "DelveDeeperContent" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Json" });
	}
}
