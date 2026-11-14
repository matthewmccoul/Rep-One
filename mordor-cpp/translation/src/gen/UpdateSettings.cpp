#include "core.h"

namespace mc {

bool UpdateSettings::CheckEnabled(cs::String iniPath) {
    return CheckEnabled(Ini::Read(iniPath.empty() ? GameFolder::LaunchIniPath() : iniPath));
}

bool UpdateSettings::CheckEnabled(cs::Ref<Ini> ini) {
    return !IsNo(ini->Text(Section, CheckKey));
}

bool UpdateSettings::Prereleases(cs::String currentVersion, cs::String iniPath) {
    return Prereleases(currentVersion, Ini::Read(iniPath.empty() ? GameFolder::LaunchIniPath() : iniPath));
}

bool UpdateSettings::Prereleases(cs::String currentVersion, cs::Ref<Ini> ini) {
    cs::String said = ini->Text(Section, PrereleasesKey);
    if (IsYes(said)) return true;
    if (IsNo(said)) return false;
    return IsPrerelease(currentVersion);
}

bool UpdateSettings::IsPrerelease(cs::String version) {
    if (cs::Str::IsNullOrWhiteSpace(version)) return false;
    cs::String v = cs::Str::Trim(version);
    int32_t plus = cs::Str::IndexOf(v, '+');
    if (plus >= 0) v = cs::Str::Substring(v, 0, plus);
    return cs::Str::Contains(v, '-');
}

bool UpdateSettings::IsYes(cs::String s) {
    cs::String t = cs::Str::ToLowerInvariant(cs::Str::Trim(s));
    return t == "true" || t == "1" || t == "yes" || t == "on";
}

bool UpdateSettings::IsNo(cs::String s) {
    cs::String t = cs::Str::ToLowerInvariant(cs::Str::Trim(s));
    return t == "false" || t == "0" || t == "no" || t == "off";
}

}  // namespace mc
