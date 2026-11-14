#include "core.h"
#include <cstdlib>

namespace mc {

namespace {

cs::Ref<GameFilesCheck> MakeCheck(const cs::String& dir, GameFilesVerdict verdict, const cs::String& reason, const cs::String& recognised = cs::String()) {
    auto _o = cs::New<GameFilesCheck>();
    _o->Dir = dir;
    _o->Verdict = verdict;
    _o->Reason = reason;
    _o->Recognised = recognised;
    return _o;
}

cs::Ref<GameFolderResolution> MakeResolution(cs::Ref<GameFilesCheck> found, const cs::String& how, bool isExplicit, cs::List<cs::Ref<GameFilesCheck>> tried) {
    auto _o = cs::New<GameFolderResolution>();
    _o->Found = found;
    _o->How = how;
    _o->Explicit = isExplicit;
    _o->Tried = tried;
    return _o;
}

// Environment.GetEnvironmentVariable (null -> "")
cs::String GetEnv(const char* name) {
    const char* v = std::getenv(name);
    return v ? cs::String(v) : cs::String();
}

bool IsSep(char c) { return c == '/' || c == '\\'; }

// Path.TrimEndingDirectorySeparator: drops one trailing separator unless the path is a root
cs::String TrimEndingDirectorySeparator(const cs::String& p) {
    if (p.size() <= 1 || !IsSep(p.back())) return p;
    if (p.size() == 3 && p[1] == ':') return p;  // "C:\"
    if (std::filesystem::path(p).root_path().string() == p) return p;
    return p.substr(0, p.size() - 1);
}

// DirectoryInfo.Parent: null at a root
cs::Nullable<cs::DirectoryInfo> ParentOf(const cs::DirectoryInfo& d) {
    cs::String s = d.FullName;
    while (s.size() > 1 && IsSep(s.back()) && std::filesystem::path(s).root_path().string() != s) s.pop_back();
    cs::String parent = std::filesystem::path(s).parent_path().string();
    if (parent.empty() || parent == s) return {};
    return cs::DirectoryInfo{parent};
}

// AppContext.BaseDirectory: the program's folder, with a trailing separator
cs::String BaseDirectory() {
    std::error_code e;
    auto exe = std::filesystem::read_symlink("/proc/self/exe", e);
    cs::String dir = e ? std::filesystem::current_path(e).string() : exe.parent_path().string();
    if (!dir.empty() && !IsSep(dir.back())) dir += cs::Path::DirectorySeparatorChar();
    return dir;
}

// Environment.GetFolderPath for ProgramFilesX86 / ProgramFiles / LocalApplicationData
cs::String FolderPath(int which) {
#ifdef _WIN32
    if (which == 0) return GetEnv("ProgramFiles(x86)");
    if (which == 1) return GetEnv("ProgramFiles");
    return GetEnv("LOCALAPPDATA");
#else
    if (which == 0 || which == 1) return cs::String();
    cs::String xdg = GetEnv("XDG_DATA_HOME");
    if (!xdg.empty()) return xdg;
    cs::String home = GetEnv("HOME");
    return home.empty() ? cs::String() : cs::Path::Combine(home, ".local/share");
#endif
}

// Environment.SystemDirectory
cs::String SystemDirectory() {
#ifdef _WIN32
    cs::String root = GetEnv("SystemRoot");
    return root.empty() ? cs::String() : cs::Path::Combine(root, "system32");
#else
    return cs::String();
#endif
}

}  // namespace

bool GameFilesCheck::IsFull() { return Verdict == GameFilesVerdict::Full; }

bool GameFilesCheck::IsLimited() { return Verdict == GameFilesVerdict::Limited; }

bool GameFilesCheck::IsPlayable() { return IsFull() || IsLimited(); }

cs::Ref<GameFilesCheck> GameFiles::Check(cs::String dir) {
    if (cs::Str::IsNullOrWhiteSpace(dir))
        return MakeCheck(dir, GameFilesVerdict::NotFound, "No folder was given.");
    cs::String full;
    try { full = cs::Path::GetFullPath(dir); }
    catch (...) { return MakeCheck(dir, GameFilesVerdict::NotFound, "That is not a folder name."); }
    if (!cs::Directory::Exists(full))
        return MakeCheck(full, GameFilesVerdict::NotFound, "There is no such folder.");

    auto P = [&](const cs::String& f) { return cs::Path::Combine(full, f); };
    auto missing = Required.Where([&](auto& f) { return !cs::File::Exists(P(f)); }).ToList();
    if (missing.Count() == Required.Length())
        return MakeCheck(full, GameFilesVerdict::Missing, "This folder has none of Mordor's files in it (MORDOR.EXE, MDATA1.MDR ...).");
    if (missing.Count() > 0)
        return MakeCheck(full, GameFilesVerdict::Missing, "Mordor's files are not all here: " + cs::Str::Join(", ", missing) + " missing.");

    // Mordor 1.x is a Visual Basic 3 program for Windows 3.1: an MZ file with a New Executable header behind it
    if (!IsWin16Program(P("MORDOR.EXE")))
        return MakeCheck(full, GameFilesVerdict::NotMordor, "MORDOR.EXE here is not the Windows 3.1 program Mordor 1.x is.");

    // MDATA1, 2, 3 and 5 open with a version string ("1.171..." in 1.1); its number is not checked
    cs::String version;
    for (auto& f : cs::List<cs::String>::Of({"MDATA1.MDR", "MDATA2.MDR", "MDATA3.MDR", "MDATA5.MDR"})) {
        cs::String v = DataVersion(P(f));
        if (v.empty())
            return MakeCheck(full, GameFilesVerdict::NotMordor, f + " here is not a Mordor data file.");
        if (version.empty()) version = v;
    }

    int32_t levels = 0, deepestMonster = 0, deepestItem = 0;
    try {
        levels = MdrLoader::LoadDungeon(P("MDATA11.MDR")).Count();
        auto monsterLevels = MdrLoader::LoadMonsters(P("MDATA5.MDR")).Select([&](auto& m) { return (int32_t)m->LevelFound; });
        deepestMonster = monsterLevels.Count() == 0 ? 0 : monsterLevels.Max();
        auto itemFloors = MdrLoader::LoadItems(P("MDATA3.MDR")).Select([&](auto& i) { return (int32_t)i->Floor; });
        deepestItem = itemFloors.Count() == 0 ? 0 : itemFloors.Max();
        MdrLoader::LoadSpells(P("MDATA2.MDR"));
        if (MdrLoader::LoadRaces(P("MDATA1.MDR")).Count() == 0 || MdrLoader::LoadGuilds(P("MDATA1.MDR")).Count() == 0)
            throw cs::InvalidDataException("no races or guilds");
    } catch (const std::exception& ex) {
        return MakeCheck(full, GameFilesVerdict::Damaged, "Mordor's data files here do not read (" + cs::String(ex.what()) + ").");
    }

    // the shareware's data: played, as the limited build (LimitedVersion)
    cs::String known = Recognise(P("MORDOR.EXE"));
    cs::String share = "Mordor, shareware (Limited Version) data " + version;
    if (levels < FullLevels)
        return MakeCheck(full, GameFilesVerdict::Limited,
            "The shareware (Limited) version of Mordor: its dungeon has " + cs::ToStr(levels) + " level" + (levels == 1 ? "" : "s") + ", the full game's has " + cs::ToStr(FullLevels) + ".",
            !known.empty() ? known : share);
    if (deepestMonster < FullLevels)
        return MakeCheck(full, GameFilesVerdict::Limited,
            "The shareware (Limited) version of Mordor: no monster in it is found below level " + cs::ToStr(deepestMonster) + ".",
            !known.empty() ? known : share);
    if (deepestItem < FullLevels)
        return MakeCheck(full, GameFilesVerdict::Limited,
            "The shareware (Limited) version of Mordor: no item in it is found below level " + cs::ToStr(deepestItem) + ".",
            !known.empty() ? known : share);

    return MakeCheck(full, GameFilesVerdict::Full,
        "The full version of Mordor (data " + version + ", " + cs::ToStr(levels) + " levels).",
        !known.empty() ? known : "Mordor, full version (data " + version + ")");
}

bool GameFiles::IsWin16Program(cs::String path) {
    try {
        auto f = cs::File::ReadAllBytes(path);
        if (f.Count() < 64 || f[0] != 'M' || f[1] != 'Z') return false;
        int32_t ne = cs::BinaryPrimitives::ReadInt32LittleEndian(&f.Raw()[0x3C]);
        if (ne <= 0 || (int64_t)ne > (int64_t)f.Count() - 2) return false;
        return f[ne] == 'N' && f[ne + 1] == 'E';
    } catch (...) { return false; }
}

cs::String GameFiles::DataVersion(cs::String path) {
    try {
        auto all = cs::File::ReadAllBytes(path);
        if (all.Count() < 12) return {};
        auto b = all.Take(12);
        auto isDigit = [](uint8_t c) { return c >= '0' && c <= '9'; };
        if (!isDigit(b[2]) || b[3] != '.' || !isDigit(b[4])) return {};
        int32_t end = 4;
        while (end < b.Length() && (isDigit(b[end]) || b[end] == '.')) end++;
        return cs::Encoding::Latin1GetString(&b.Raw()[2], end - 2);
    } catch (...) { return {}; }
}

cs::String GameFiles::Recognise(cs::String exe) {
    try
    {
        cs::String name;
        return KnownExe.TryGetValue(cs::Convert::ToHexString(cs::SHA256::HashFile(exe)), name) ? name : cs::String();
    }
    catch (...) { return {}; }
}

bool GameFolderResolution::IsFull() { return (Found ? Found->IsFull() : false) == true; }

bool GameFolderResolution::IsPlayable() { return (Found ? Found->IsPlayable() : false) == true; }

cs::Ref<GameFilesCheck> GameFolderResolution::Nearest() {
    if (Found) return Found;
    auto a = Tried.FirstOrDefault([&](auto& t) { return t->Verdict == GameFilesVerdict::Damaged || t->Verdict == GameFilesVerdict::NotMordor; });
    if (a) return a;
    return Tried.FirstOrDefault([&](auto& t) { return t->Verdict == GameFilesVerdict::Missing && !cs::Str::StartsWith(t->Reason, "This folder has none"); });
}

cs::String GameFolder::LaunchIniPath() {
    cs::String own = GetEnv(EnvIni.c_str());
    if (!cs::Str::IsNullOrWhiteSpace(own)) return own;
    return cs::Path::Combine(UserFolders::Roaming(), "settings.ini");
}

cs::String GameFolder::Configured(cs::String iniPath) {
    cs::String v = Ini::Read(!iniPath.empty() ? iniPath : LaunchIniPath())->Text(Section, Key);
    return v == "" ? cs::String() : v;
}

bool GameFolder::Remember(cs::String dir, cs::String iniPath) {
    cs::String path = !iniPath.empty() ? iniPath : LaunchIniPath();
    try {
        cs::String parent = cs::Path::GetDirectoryName(path);
        if ((int32_t)parent.size() > 0) cs::Directory::CreateDirectory(parent);
    } catch (...) { return false; }
    auto ini = Ini::Read(path);
    ini->Set(Section, Key, dir);
    return ini->Write(path);
}

cs::Ref<GameFolderResolution> GameFolder::Resolve(cs::String iniPath, cs::String baseDir) {
    cs::String env = GetEnv(EnvDir.c_str());
    if (!cs::Str::IsNullOrWhiteSpace(env))
        return MakeResolution(GameFiles::Check(env), EnvDir, true, cs::List<cs::Ref<GameFilesCheck>>::New());
    cs::String configured = Configured(iniPath);
    if (!configured.empty())
        return MakeResolution(GameFiles::Check(configured), "settings (" + (!iniPath.empty() ? iniPath : LaunchIniPath()) + ")", true, cs::List<cs::Ref<GameFilesCheck>>::New());

    auto tried = cs::List<cs::Ref<GameFilesCheck>>::New();
    cs::Ref<GameFilesCheck> share = nullptr;
    cs::String shareHow;
    for (auto& c : Candidates(!baseDir.empty() ? baseDir : BaseDirectory())) {
        cs::String dir = c.Dir;
        cs::String how = c.How;
        auto check = GameFiles::Check(dir);
        if (check->IsFull()) return MakeResolution(check, how, false, tried);
        if (check->IsLimited()) {
            if (!share) { share = check; shareHow = how; }
        } else if (check->Verdict != GameFilesVerdict::NotFound) tried.Add(check);
    }
    if (share) return MakeResolution(share, shareHow, false, tried);
    return MakeResolution(nullptr, "not found", false, tried);
}

cs::List<Tup_Dir_How_a52cf> GameFolder::Candidates(cs::String baseDir) {
    cs::List<Tup_Dir_How_a52cf> _result = cs::List<Tup_Dir_How_a52cf>::New();
    // the C# set compares ignoring case: lower-cased keys
    auto seen = cs::HashSet<cs::String>::New();
    for (auto& c : Raw(baseDir)) {
        cs::String full;
        // a trailing separator goes, but not a root's: "C:" alone is the current directory on C:
        try { full = TrimEndingDirectorySeparator(cs::Path::GetFullPath(c.Dir)); }
        catch (...) { continue; }
        if (full != "" && seen.Add(cs::Str::ToLower(full))) _result.Add(Tup_Dir_How_a52cf{full, c.How});
    }
    return _result;
}

cs::List<Tup_Dir_How_a52cf> GameFolder::Raw(cs::String baseDir) {
    cs::List<Tup_Dir_How_a52cf> _result = cs::List<Tup_Dir_How_a52cf>::New();
    // beside the port, or beside any folder above it; and the port's own folder
    for (auto d = SafeDir(baseDir); d.has_value(); d = ParentOf(d.value())) {
        _result.Add(Tup_Dir_How_a52cf{cs::Path::Combine(d.value().FullName, "game_files"), "beside the port"});
        _result.Add(Tup_Dir_How_a52cf{d.value().FullName, "the port's folder"});
    }
    // the port's own unpacking of MordorSetup.exe or MordorDemoSetup.exe (the first-run window)
    for (auto& dir : InstallerUnpack::Unpacked()) _result.Add(Tup_Dir_How_a52cf{dir, "unpacked from the installer"});
    // what the original reads: [File Information] BasePath in MORDOR.INI in the Windows folder
    cs::String win = GetEnv("WINDIR");
    if (win != "") {
        cs::String basePath = Ini::Read(cs::Path::Combine(win, "MORDOR.INI"))->Text("File Information", "BasePath");
        if (basePath != "") _result.Add(Tup_Dir_How_a52cf{basePath, "MORDOR.INI's BasePath"});
    }
#ifdef _WIN32
    for (auto& dir : UninstallEntries()) _result.Add(Tup_Dir_How_a52cf{dir, "its uninstall entry"});
#endif
    cs::String local = FolderPath(2);
    for (auto& root : cs::List<cs::String>::Of({
             FolderPath(0),
             FolderPath(1),
             (int32_t)local.size() > 0 ? cs::Path::Combine(local, "Programs") : cs::String(""),
         })) {
        if (root == "") continue;
        for (auto& name : cs::List<cs::String>::Of({"Mordor", "Mordor v1.1", "Mordor v1.1 The Depths of Dejenol", "Mordor The Depths of Dejenol", "MakeItSo\\Mordor"}))
            _result.Add(Tup_Dir_How_a52cf{cs::Path::Combine(root, name), "an install folder"});
    }
    cs::String sysDir = SystemDirectory();
    cs::String sys = sysDir.empty() ? cs::String() : std::filesystem::path(sysDir).root_path().string();
    if (sys.empty()) sys = "C:\\";
    _result.Add(Tup_Dir_How_a52cf{cs::Path::Combine(sys, "MORDOR"), "an install folder"});
    _result.Add(Tup_Dir_How_a52cf{cs::Path::Combine(sys, "GAMES", "MORDOR"), "an install folder"});
    return _result;
}

cs::Nullable<cs::DirectoryInfo> GameFolder::SafeDir(cs::String path) {
    try {
        // new DirectoryInfo throws on an empty path
        if (path.empty()) throw cs::ArgumentException("path");
        return cs::DirectoryInfo{cs::Path::GetFullPath(path)};
    } catch (...) { return {}; }
}

cs::List<cs::String> GameFolder::UninstallEntries() {
    // the Windows registry is not available here; the C# swallows every failure and returns what it found
    auto found = cs::List<cs::String>::New();
    return found;
}

cs::String GameFolder::ExeFolder(cs::String command) {
    cs::String s = cs::Str::Trim(command);
    if (cs::Str::StartsWith(s, '"')) {
        int32_t q = cs::Str::IndexOf(s, '"', 1);
        if (q > 0) s = cs::Str::Substring(s, 1, q - 1);
    } else {
        int32_t exe = cs::Str::IndexOfIgnoreCase(s, ".exe");
        if (exe > 0) s = cs::Str::Substring(s, 0, exe + 4);
    }
    int32_t comma = cs::Str::LastIndexOf(s, ',');
    if (comma > 0 && !cs::Str::Contains(cs::Str::Substring(s, comma), '\\')) s = cs::Str::Substring(s, 0, comma);
    try { return cs::Path::GetDirectoryName(s); } catch (...) { return {}; }
}

}  // namespace mc
