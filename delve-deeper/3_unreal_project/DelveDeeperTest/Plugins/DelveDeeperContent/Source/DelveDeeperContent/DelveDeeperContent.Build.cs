// Hand-written. PROPOSAL, NOT RUN - Unreal missing (never compiled).
using System.IO;
using UnrealBuildTool;

public class DelveDeeperContent : ModuleRules
{
    public DelveDeeperContent(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // The generated core copies are plain standard C++17: no exceptions, no RTTI.
        // LOCAL TEST FIX: UE 5.8 rejects Cpp17; use the engine default (C++20).
        // CppStandard = CppStandardVersion.Cpp17;
        bEnableExceptions = false;
        bUseRTTI = false;

        // The core .cpp files keep file-local helper names; unity blobs could make them collide.
        bUseUnity = false;

        // Core headers are included project-rooted as "dd/<name>.h".
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public", "Core"));

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Projects" });

        // LOCAL TEST FIX: editor notifications and viewport framing for Build Floor (editor builds only).
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Slate", "SlateCore" });
        }
    }
}
