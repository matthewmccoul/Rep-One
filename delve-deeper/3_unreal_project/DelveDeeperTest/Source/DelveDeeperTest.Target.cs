using UnrealBuildTool;

public class DelveDeeperTestTarget : TargetRules
{
	public DelveDeeperTestTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("DelveDeeperTest");
	}
}
