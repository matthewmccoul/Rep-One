#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct PortFiles {
    inline static const cs::String CrashLogName = "crash.log";

    static cs::String PerUserRoot();
    static cs::String BesideSaves(cs::String savesDir);
    static cs::List<cs::String> CrashLogPlaces(cs::String savesDir, cs::String perUserRoot);
    static cs::List<cs::String> CrashLogPlaces(cs::String gameDir);
    static cs::List<cs::String> CharStatPlaces(cs::String gameDir, cs::String savesDir, cs::String perUserSaves, cs::String perUserRoot);
    static cs::List<cs::String> CharStatPlaces(cs::String gameDir);
    static cs::String Append(cs::List<cs::String> places, cs::String text);
    static bool SamePath(cs::String a, cs::String b);
    static cs::List<cs::String> Distinct(cs::List<cs::String> l);
};

}  // namespace mc
