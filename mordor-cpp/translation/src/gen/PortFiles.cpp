#include "core.h"

namespace mc {

cs::String PortFiles::PerUserRoot() {
    auto d = cs::Path::GetDirectoryName(Saves::PerUserDir());
    return !d.empty() ? d : Saves::PerUserDir();
}

cs::String PortFiles::BesideSaves(cs::String savesDir) {
    try { return cs::Path::GetDirectoryName(cs::Path::GetFullPath(savesDir)); }
    catch (...) { return cs::String{}; }
}

cs::List<cs::String> PortFiles::CrashLogPlaces(cs::String savesDir, cs::String perUserRoot) {
    auto l = cs::List<cs::String>::New();
    if (!savesDir.empty()) {
        auto b = BesideSaves(savesDir);
        if ((int32_t)b.size() > 0) l.Add(cs::Path::Combine(b, CrashLogName));
    }
    l.Add(cs::Path::Combine(perUserRoot, CrashLogName));
    l.Add(cs::Path::Combine(cs::Path::GetTempPath(), "mordor-crash.log"));
    return Distinct(l);
}

cs::List<cs::String> PortFiles::CrashLogPlaces(cs::String gameDir) {
    return CrashLogPlaces(cs::Str::IsNullOrWhiteSpace(gameDir) ? cs::String{} : Saves::DirFor(gameDir), PerUserRoot());
}

cs::List<cs::String> PortFiles::CharStatPlaces(cs::String gameDir, cs::String savesDir, cs::String perUserSaves, cs::String perUserRoot) {
    auto l = cs::List<cs::String>::New();
    if (!SamePath(savesDir, perUserSaves)) {
        auto b = BesideSaves(savesDir);
        if ((int32_t)b.size() > 0) l.Add(cs::Path::Combine(b, CharStatFile::FileName));
    }
    if (!cs::Str::IsNullOrWhiteSpace(gameDir)) l.Add(cs::Path::Combine(gameDir, CharStatFile::FileName));
    l.Add(cs::Path::Combine(perUserRoot, CharStatFile::FileName));
    return Distinct(l);
}

cs::List<cs::String> PortFiles::CharStatPlaces(cs::String gameDir) {
    return CharStatPlaces(gameDir, Saves::DirFor(gameDir), Saves::PerUserDir(), PerUserRoot());
}

cs::String PortFiles::Append(cs::List<cs::String> places, cs::String text) {
    for (auto& p : places) {
        try {
            auto dir = cs::Path::GetDirectoryName(p);
            if ((int32_t)dir.size() > 0) cs::Directory::CreateDirectory(dir);
            // File.AppendAllText
            {
                std::ofstream f(p, std::ios::binary | std::ios::app);
                if (!f) throw cs::IOException("cannot write " + p);
                f << text;
                if (!f) throw cs::IOException("cannot write " + p);
            }
            return p;
        } catch (...) { /* not writable here: the next place */ }
    }
    return cs::String{};
}

bool PortFiles::SamePath(cs::String a, cs::String b) {
    try { return cs::Str::EqualsIgnoreCase(cs::Str::TrimEnd(cs::Path::GetFullPath(a), "\\/"), cs::Str::TrimEnd(cs::Path::GetFullPath(b), "\\/")); }
    catch (...) { return false; }
}

cs::List<cs::String> PortFiles::Distinct(cs::List<cs::String> l) {
    // OrdinalIgnoreCase set: keys compared lower-cased
    auto seen = cs::HashSet<cs::String>::New();
    auto r = cs::List<cs::String>::New();
    for (auto& p : l) {
        cs::String key;
        try { key = cs::Path::GetFullPath(p); } catch (...) { key = p; }
        if (seen.Add(cs::Str::ToLower(key))) r.Add(p);
    }
    return r;
}

}  // namespace mc
