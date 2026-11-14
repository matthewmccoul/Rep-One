#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SaveImport_CharacterChoice {
    cs::Ref<ImportedCharacter> Imported{};
    bool Exists = false;
    cs::String File{};
    SaveImport_CharacterChoice() = default;
    SaveImport_CharacterChoice(cs::Ref<ImportedCharacter> Imported, bool Exists, cs::String File)
        : Imported(Imported), Exists(Exists), File(File) {}
};

struct SaveImport_WorldChoice {
    cs::String File{};
    cs::String Holds{};
    bool Exists = false;
    SaveImport_WorldChoice() = default;
    SaveImport_WorldChoice(cs::String File, cs::String Holds, bool Exists)
        : File(File), Holds(Holds), Exists(Exists) {}
};

struct SaveImport_ImportPlan {
    cs::List<cs::Ref<SaveImport_CharacterChoice>> Characters{};
    cs::List<cs::Ref<SaveImport_WorldChoice>> World{};
    SaveImport_ImportPlan() = default;
    SaveImport_ImportPlan(cs::List<cs::Ref<SaveImport_CharacterChoice>> Characters, cs::List<cs::Ref<SaveImport_WorldChoice>> World)
        : Characters(Characters), World(World) {}
};

struct SaveImport_ImportResult {
    cs::String BackupDir{};
    cs::List<cs::String> Written{};
    cs::List<cs::String> Skipped{};
    SaveImport_ImportResult() = default;
    SaveImport_ImportResult(cs::String BackupDir, cs::List<cs::String> Written, cs::List<cs::String> Skipped)
        : BackupDir(BackupDir), Written(Written), Skipped(Skipped) {}
};

struct SaveImport {
    using CharacterChoice = SaveImport_CharacterChoice;
    using WorldChoice = SaveImport_WorldChoice;
    using ImportPlan = SaveImport_ImportPlan;
    using ImportResult = SaveImport_ImportResult;

    static cs::Ref<SaveImport_ImportPlan> Plan(cs::Ref<OriginalSaves> src, cs::String savesDir);
    static cs::HashSet<cs::String> ExistingNames(cs::String savesDir);
    static cs::String Backup(cs::String savesDir, cs::DateTime now);
    static cs::Ref<SaveImport_ImportResult> Apply(cs::Ref<OriginalSaves> src, cs::String savesDir, cs::List<cs::String> characters, bool world, cs::DateTime now);
    static cs::List<cs::String> Summary(cs::Ref<OriginalSaves> src);
};

}  // namespace mc
