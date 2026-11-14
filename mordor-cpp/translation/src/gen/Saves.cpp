#include "core.h"
#include <cstdlib>

namespace mc {

cs::String Saves::PerUserDir() {
    return cs::Path::Combine(UserFolders::Roaming(), "saves");
}

cs::String Saves::LegacyDirFor(cs::String gameDir) {
    return cs::Path::Combine(gameDir, "..", "saves");
}

cs::String Saves::DirFor(cs::String gameDir) {
    // Environment.GetEnvironmentVariable: a missing variable (null) becomes ""
    const char* env = std::getenv(EnvDir.c_str());
    return Resolve(gameDir, env ? cs::String(env) : cs::String(), PerUserDir());
}

cs::String Saves::Resolve(cs::String gameDir, cs::String overridden, cs::String perUser) {
    if (!cs::Str::IsNullOrWhiteSpace(overridden)) {
        try { cs::Directory::CreateDirectory(overridden); } catch (...) { }
        return overridden;
    }
    cs::String legacy = LegacyDirFor(gameDir);
    if (HasAnything(legacy)) return legacy;
    return perUser;
}

bool Saves::HasAnything(cs::String dir) {
    try { return cs::Directory::Exists(dir) && cs::Directory::EnumerateFileSystemEntries(dir).Any(); }
    catch (...) { return false; }
}

}  // namespace mc
