#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Backups {
    static bool IsGameFile(cs::String path);
    static cs::List<cs::String> GameFilesIn(cs::String dir);
    static bool IsCharacterFile(cs::String path);
    static bool IsCharacterFile(cs::String path, bool readable);
    static cs::List<cs::String> CharacterFilesIn(cs::String dir);
    static bool Found(cs::String dir);
    static int64_t Size(cs::String savesDir);
    static int64_t FreeSpace(cs::String dir);
    static bool SameDir(cs::String a, cs::String b);
    static void Backup(cs::String savesDir, cs::String toDir);
    static void Restore(cs::String fromDir, cs::String savesDir);
    static void Copy(cs::String fromDir, cs::String toDir);
    static bool IsPartyFile(cs::String path);
};

}  // namespace mc
