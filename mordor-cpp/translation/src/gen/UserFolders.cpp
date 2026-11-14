#include "core.h"

namespace mc {

namespace {
// Environment.GetFolderPath as .NET resolves it (Windows: APPDATA/LOCALAPPDATA; Unix: XDG dirs or $HOME fallbacks)
cs::String UserFolders_EnvVar(const char* name) {
    const char* v = std::getenv(name);
    return v ? cs::String(v) : cs::String();
}
cs::String UserFolders_GetFolderPath(bool local) {
#ifdef _WIN32
    return UserFolders_EnvVar(local ? "LOCALAPPDATA" : "APPDATA");
#else
    cs::String xdg = UserFolders_EnvVar(local ? "XDG_DATA_HOME" : "XDG_CONFIG_HOME");
    if (!xdg.empty() && xdg[0] == '/') return xdg;
    cs::String home = UserFolders_EnvVar("HOME");
    if (home.empty()) return cs::String();
    return local ? cs::Path::Combine(home, ".local/share") : cs::Path::Combine(home, ".config");
#endif
}
}  // namespace

cs::String UserFolders::Roaming() { return In(RoamingParent()); }

cs::String UserFolders::Local() { return In(LocalParent()); }

cs::String UserFolders::RoamingParent() { return UserFolders_GetFolderPath(false); }

cs::String UserFolders::LocalParent() {
    cs::String local = UserFolders_GetFolderPath(true);
    return cs::Str::IsNullOrEmpty(local) ? cs::Path::GetTempPath() : local;
}

cs::String UserFolders::In(cs::String parent) {
    cs::String now = cs::Path::Combine(parent, Name), old = cs::Path::Combine(parent, OldName);
    if (!cs::Directory::Exists(now) && cs::Directory::Exists(old)) return old;
    return now;
}

cs::String UserFolders::Migrate(cs::String parent) {
    try {
        cs::String now = cs::Path::Combine(parent, Name), old = cs::Path::Combine(parent, OldName);
        if (!cs::Directory::Exists(now) && cs::Directory::Exists(old)) std::filesystem::rename(old, now);
    } catch (...) { /* in use or not allowed: the old folder goes on being read where it is */ }
    return In(parent);
}

void UserFolders::MigrateAll() {
    try { if (!cs::Str::IsNullOrEmpty(RoamingParent())) Migrate(RoamingParent()); } catch (...) { }
    try { Migrate(LocalParent()); } catch (...) { }
}

}  // namespace mc
