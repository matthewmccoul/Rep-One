#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GameFilesCheck {
    cs::String Dir{};
    GameFilesVerdict Verdict{};
    cs::String Reason{};
    cs::String Recognised{};
    bool IsFull();
    bool IsLimited();
    bool IsPlayable();
};

struct GameFiles {
    static constexpr int32_t FullLevels = 15;
    static constexpr int32_t SharewareLevels = 3;
    inline static cs::List<cs::String> Required =
        cs::List<cs::String>::Of({"MORDOR.EXE", "MDATA1.MDR", "MDATA2.MDR", "MDATA3.MDR", "MDATA5.MDR", "MDATA11.MDR"});
    // the C# dictionary compares keys ignoring case; the key is stored upper-case
    inline static cs::Dictionary<cs::String, cs::String> KnownExe = cs::Dictionary<cs::String, cs::String>::Of({
        {"5284A6A71A10259C97534FD38C3D22F61A3D792065C2999333FC63E80BA7D197", "Mordor 1.1 (full release)"},
    });
    static cs::Ref<GameFilesCheck> Check(cs::String dir);
    static bool IsWin16Program(cs::String path);
    static cs::String DataVersion(cs::String path);
    static cs::String Recognise(cs::String exe);
};

struct GameFolderResolution {
    cs::Ref<GameFilesCheck> Found{};
    cs::String How{};
    bool Explicit = false;
    cs::List<cs::Ref<GameFilesCheck>> Tried{};
    bool IsFull();
    bool IsPlayable();
    cs::Ref<GameFilesCheck> Nearest();
};

struct GameFolder {
    inline static const cs::String EnvDir = "MORDOR_GAMEDIR";
    inline static const cs::String EnvIni = "MORDOR_LAUNCH_INI";
    inline static const cs::String Section = "Loading Information";
    inline static const cs::String Key = "GameDir";
    static cs::String LaunchIniPath();
    static cs::String Configured(cs::String iniPath = {});
    static bool Remember(cs::String dir, cs::String iniPath = {});
    static cs::Ref<GameFolderResolution> Resolve(cs::String iniPath = {}, cs::String baseDir = {});
    static cs::List<Tup_Dir_How_a52cf> Candidates(cs::String baseDir);
    static cs::List<Tup_Dir_How_a52cf> Raw(cs::String baseDir);
    static cs::Nullable<cs::DirectoryInfo> SafeDir(cs::String path);
    static cs::List<cs::String> UninstallEntries();
    static cs::String ExeFolder(cs::String command);
};

}  // namespace mc
