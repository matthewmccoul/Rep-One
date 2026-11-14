#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Race {
    cs::String Name{};
    cs::List<int32_t> Min;
    cs::List<int32_t> Max;
    RaceSize Size{};
    int32_t AlignMask = 7;
    int32_t BonusPoints = 0;
    int32_t MaxAge = 0;
    float ExpFactor = 0;

    cs::List<int32_t> Resistances;

    // constructor sets Resistances = new int[Effects.ResistanceKinds] (Effects is not complete here)
    Race();

    cs::List<Alignment> Alignments();

    int32_t MinTotal();
    int32_t MaxTotal();
};

struct RaceData {
    inline static cs::List<cs::String> StatNames = cs::List<cs::String>::Of({"STR", "INT", "WIS", "CON", "CHA", "DEX"});

    inline static cs::List<cs::String> StatNamesLong =
        cs::List<cs::String>::Of({"Strength", "Intelligence", "Wisdom", "Constitution", "Charisma", "Dexterity"});

    inline static cs::List<cs::Ref<Race>> All = cs::List<cs::Ref<Race>>::New();

    static void LoadFrom(cs::String gameDir);

    static cs::Ref<Race> ByName(cs::String name);

    static int32_t IndexOf(cs::String name);
};

}  // namespace mc
