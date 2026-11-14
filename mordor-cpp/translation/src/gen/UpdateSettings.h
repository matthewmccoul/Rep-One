#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct UpdateSettings {
    inline static const cs::String Section = "Updates";
    inline static const cs::String CheckKey = "Check";
    inline static const cs::String PrereleasesKey = "Prereleases";

    static bool CheckEnabled(cs::String iniPath = {});
    static bool CheckEnabled(cs::Ref<Ini> ini);
    static bool Prereleases(cs::String currentVersion, cs::String iniPath = {});
    static bool Prereleases(cs::String currentVersion, cs::Ref<Ini> ini);
    static bool IsPrerelease(cs::String version);
    static bool IsYes(cs::String s);
    static bool IsNo(cs::String s);
};

}  // namespace mc
