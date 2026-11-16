// Hand-written helper. PROPOSAL, NOT RUN - Unreal missing.
#include "DDText.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool DDText::LoadDataFile(const FString& FileName, FString& OutText, FString& OutError)
{
    if (FileName.IsEmpty() || FileName.Contains(TEXT("..")) || FileName.Contains(TEXT("/")) || FileName.Contains(TEXT("\\")))
    {
        OutError = FString::Printf(TEXT("DATA_FILE_NAME: bad file name '%s'"), *FileName);
        return false;
    }
    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("DelveDeeperContent"));
    if (!Plugin.IsValid())
    {
        OutError = TEXT("DATA_PLUGIN: plugin not found by the plugin manager");
        return false;
    }
    const FString Path = FPaths::Combine(Plugin->GetContentDir(), TEXT("Data"), FileName);
    if (!FFileHelper::LoadFileToString(OutText, *Path))
    {
        OutError = FString::Printf(TEXT("DATA_READ: cannot read %s"), *Path);
        return false;
    }
    return true;
}
