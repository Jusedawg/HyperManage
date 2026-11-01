using UnrealBuildTool;

public class HyperManage : ModuleRules
{
    public HyperManage(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.NoPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "FactoryGame", "SML", "UMG", "AbstractInstance" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "Json", "JsonUtilities", "Projects" });
        RuntimeDependencies.Add("$(PluginDir)/Config/DefaultHyperManage.ini", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(PluginDir)/LICENSE", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(PluginDir)/Resources/Icon128.png", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(PluginDir)/Art/Clipboard/Kalam-OFL.txt", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(PluginDir)/Art/Clipboard/Kalam-Regular.ttf", StagedFileType.NonUFS);
    }
}
