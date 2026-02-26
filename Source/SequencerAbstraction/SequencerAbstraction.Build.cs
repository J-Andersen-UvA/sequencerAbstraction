using UnrealBuildTool;

public class SequencerAbstraction : ModuleRules
{
    public SequencerAbstraction(ReadOnlyTargetRules Target) : base(Target)
    {
        // Do NOT set: Type = ModuleType.Editor;  (not valid in UE 5.7 rules)

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[]
            {
                "UnrealEd",
                "AssetTools",
                "AssetRegistry",
                "ContentBrowser",

                "LevelSequence",
                "MovieScene",
                "MovieSceneTracks",
                "MovieSceneTools",
                "Sequencer",
                "EditorScriptingUtilities",
                "LevelSequenceEditor",
                "SequencerScripting",
                "SequencerScriptingEditor",

                "RigVM",
                "ControlRig",
                "ControlRigEditor",
            });
        }
    }
}