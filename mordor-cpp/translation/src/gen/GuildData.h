#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GuildDef {
    cs::String Name{};
    int32_t MaxLevel = 999;
    cs::List<int32_t> ReqStats = cs::List<int32_t>::New(6);
    int32_t AlignMask = 7;
    int32_t RaceMask = 0;
    int32_t AvgHits = 0;
    int32_t HitsCapLevel = 0;
    int32_t LateHits = 0;

    float AttPerLevel = 0;
    float DefPerLevel = 0;
    int32_t GoldFactor = 0;
    float ItemUseFactor = 0;
    int32_t AdMaxLevel = 0;
    float AdEvery = 0;

    cs::List<float> Abilities = cs::List<float>::New(7);
    float Thieving();
    float Backstab();
    float CriticalHit();
    float ExtraSwings();
    float Perception();
    float ExpFactor = 0;
    int32_t QuestPercent = 0;

    cs::List<float> SpellRate = cs::List<float>::New(19);
    cs::List<int32_t> SpellMax = cs::List<int32_t>::New(19);

    bool HasSpellTable();

    bool CanCast(int32_t category, int32_t spellLevel, int32_t guildLevel);
    int32_t ShownSpellLevel(int32_t category, int32_t spellLevel);
    bool AllowsAlignment(Alignment a);
    bool AllowsRace(cs::String race);
    bool MeetsStats(cs::List<int32_t> stats);
    cs::List<cs::String> Unmet(cs::Ref<Character> c);
};

struct GuildData {
    inline static cs::List<cs::Ref<GuildDef>> All = cs::List<cs::Ref<GuildDef>>::New();
    inline static const cs::String StartingGuild = "Nomad";

    static void LoadFrom(cs::String gameDir);
    static cs::Ref<GuildDef> ByName(cs::String name);
    static int32_t IndexOf(cs::String name);
    static bool CanUse(cs::Ref<Item> it, cs::Ref<Character> c);
};

}  // namespace mc
