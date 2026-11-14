#include "core.h"

namespace mc {

bool Backups::IsGameFile(cs::String path) {
    cs::String name = cs::Path::GetFileName(path);
    return !cs::Str::EqualsIgnoreCase(name, GuildLog::BackupName)
        && !cs::Str::EqualsIgnoreCase(name, PortSettings::FileName);
}

cs::List<cs::String> Backups::GameFilesIn(cs::String dir) {
    try {
        // StringComparer.OrdinalIgnoreCase orders by upper-cased text
        return cs::Directory::GetFiles(dir)
            .Where([&](auto& f) { return IsGameFile(f); })
            .OrderBy([&](auto& f) { return cs::Str::ToUpper(f); })
            .ToList();
    } catch (...) {
        return cs::List<cs::String>::New();
    }
}

bool Backups::IsCharacterFile(cs::String path) { return IsCharacterFile(path, false); }

bool Backups::IsCharacterFile(cs::String path, bool readable) {
    cs::String name = cs::Path::GetFileName(path);
    if (!cs::Str::EndsWithIgnoreCase(name, ".json")) return false;
    if (cs::Str::StartsWithIgnoreCase(name, "party-")) return false;
    try {
        return !cs::Str::IsNullOrWhiteSpace(cs::deref(readable ? Character::Load(path) : Character::ReadRaw(path)).Name);
    } catch (...) {
        return false;
    }
}

cs::List<cs::String> Backups::CharacterFilesIn(cs::String dir) {
    return GameFilesIn(dir).Where([&](auto& f) { return IsCharacterFile(f); }).ToList();
}

bool Backups::Found(cs::String dir) {
    // a backup counts when it holds a character the menu's list would show (Roster.Saved)
    try {
        return cs::Directory::Exists(dir)
            && cs::Directory::GetFiles(dir, "*.json").Any([&](auto& f) { return IsCharacterFile(f, true); });
    } catch (...) {
        return false;
    }
}

int64_t Backups::Size(cs::String savesDir) {
    return GameFilesIn(savesDir).Sum([&](auto& f) -> int64_t {
        try { return (int64_t)std::filesystem::file_size(f); }
        catch (...) { return 0LL; }
    });
}

int64_t Backups::FreeSpace(cs::String dir) {
    try {
        cs::String root = std::filesystem::path(cs::Path::GetFullPath(dir)).root_path().string();
        return (int64_t)std::filesystem::space(root).available;
    } catch (...) {
        return -1;
    }
}

bool Backups::SameDir(cs::String a, cs::String b) {
    return cs::Str::EqualsIgnoreCase(cs::Str::TrimEnd(cs::Path::GetFullPath(a), "\\/"),
                                     cs::Str::TrimEnd(cs::Path::GetFullPath(b), "\\/"));
}

void Backups::Backup(cs::String savesDir, cs::String toDir) { Copy(savesDir, toDir); }

void Backups::Restore(cs::String fromDir, cs::String savesDir) { Copy(fromDir, savesDir); }

void Backups::Copy(cs::String fromDir, cs::String toDir) {
    auto sources = GameFilesIn(fromDir);
    // case-insensitive set: names kept upper-cased
    auto characters = cs::HashSet<cs::String>::From(
        sources.Where([&](auto& f) { return IsCharacterFile(f); })
            .Select([&](auto& f) { return cs::Str::ToUpper(cs::Path::GetFileName(f)); }));
    bool allCopied = true;
    for (auto& f : sources) {
        try { cs::File::Copy(f, cs::Path::Combine(toDir, cs::Path::GetFileName(f)), true); }
        catch (...) { allCopied = false; /* Resume Next */ }
    }
    if (!allCopied) return;
    // a source with no character at all never clears the far end's characters
    if (characters.Count() == 0) return;
    for (auto& f : CharacterFilesIn(toDir)) {
        if (characters.Contains(cs::Str::ToUpper(cs::Path::GetFileName(f)))) continue;
        try { cs::File::Delete(f); } catch (...) { }
    }
    // The parties are one file in the original too, MDATA12 (gc1C38, MAIN.TXT:590), which
    // mdrmulLoadParty reads (MULTIMOD.BAS:218) and fn0662 copies whole (MAINMOD.BAS:1002, 1021).
    auto parties = cs::HashSet<cs::String>::From(
        sources.Where([&](auto& f) { return IsPartyFile(f); })
            .Select([&](auto& f) { return cs::Str::ToUpper(cs::Path::GetFileName(f)); }));
    for (auto& f : GameFilesIn(toDir).Where([&](auto& x) { return IsPartyFile(x); })) {
        if (parties.Contains(cs::Str::ToUpper(cs::Path::GetFileName(f)))) continue;
        try { cs::File::Delete(f); } catch (...) { }
    }
}

bool Backups::IsPartyFile(cs::String path) {
    cs::String name = cs::Path::GetFileName(path);
    return cs::Str::StartsWithIgnoreCase(name, "party-")
        && cs::Str::EndsWithIgnoreCase(name, ".json");
}

}  // namespace mc
