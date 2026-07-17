using UnrealBuildTool;

public class ESPBlueprintAutomation : ModuleRules {
    public ESPBlueprintAutomation(ReadOnlyTargetRules Target) : base(Target) {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "UMG",
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "BlueprintGraph",
            "Slate",
            "SlateCore",
            "UnrealEd",
            "UMGEditor",
        });
    }
}
