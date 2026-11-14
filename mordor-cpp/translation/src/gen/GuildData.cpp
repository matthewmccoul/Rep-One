#include "core.h"

namespace mc {

float GuildDef::Thieving() { return Abilities[0]; }
float GuildDef::Backstab() { return Abilities[1]; }
float GuildDef::CriticalHit() { return Abilities[2]; }
float GuildDef::ExtraSwings() { return Abilities[3]; }
float GuildDef::Perception() { return Abilities[5]; }

bool GuildDef::HasSpellTable() { return SpellMax.Any([&](auto& v) { return v > 0; }); }

bool GuildDef::CanCast(int32_t category, int32_t spellLevel, int32_t guildLevel) {
    if (category < 0 || category >= SpellMax.Length()) return false;
    if (SpellMax[category] <= 0 || spellLevel > SpellMax[category]) return false;
    float rate = SpellRate[category] <= 0 ? 1.0f : SpellRate[category];
    return spellLevel * rate <= guildLevel / 2.0 + 0.5;
}

int32_t GuildDef::ShownSpellLevel(int32_t category, int32_t spellLevel) {
    float rate = category < 0 || category >= SpellRate.Length() || SpellRate[category] <= 0 ? 1.0f : SpellRate[category];
    return cs::CastDouble<int32_t>(spellLevel * rate);
}

bool GuildDef::AllowsAlignment(Alignment a) { return (AlignMask & (1 << static_cast<int32_t>(a))) != 0; }

bool GuildDef::AllowsRace(cs::String race) {
    int32_t i = RaceData::IndexOf(race);
    return i < 0 || (RaceMask & (1 << i)) != 0;
}

bool GuildDef::MeetsStats(cs::List<int32_t> stats) {
    return cs::Enumerable::Range(0, 6).All([&](auto& i) { return stats[i] >= ReqStats[i]; });
}

cs::List<cs::String> GuildDef::Unmet(cs::Ref<Character> c) {
    auto why = cs::List<cs::String>::New();
    for (int32_t i = 0; i < 6; i++)
        if (c->Stats[i] < ReqStats[i]) why.Add(RaceData::StatNames[i] + " " + cs::ToStr(ReqStats[i]));
    if (!AllowsAlignment(c->Alignment)) why.Add("not for " + ToStr(c->Alignment));
    if (!AllowsRace(c->Race)) why.Add("not for " + c->Race);
    return why;
}

void GuildData::LoadFrom(cs::String gameDir) {
    cs::String path = cs::Path::Combine(gameDir, "MDATA1.MDR");
    auto guilds = MdrLoader::LoadGuilds(path);
    if (guilds.Count() == 0) throw cs::InvalidDataException("No guilds could be read from " + path + ".");
    All = guilds;
}

cs::Ref<GuildDef> GuildData::ByName(cs::String name) {
    auto g = All.FirstOrDefault([&](auto& g) { return g->Name == name; });
    if (g) return g;
    throw cs::KeyNotFoundException("No guild named \"" + name + "\" in MDATA1 (" + cs::ToStr(All.Count()) + " guilds loaded).");
}

int32_t GuildData::IndexOf(cs::String name) {
    return All.ToList().FindIndex([&](auto& g) { return g->Name == name; });
}

bool GuildData::CanUse(cs::Ref<Item> it, cs::Ref<Character> c) {
    if (it->ClassRestricted != 0) {
        int32_t cur = IndexOf(c->PrimaryGuild());
        return cur >= 0 && cur < 31 && (it->Guilds & (1 << cur)) != 0;
    }
    for (auto& m : c->Guilds) {
        if (m->Level <= 0) continue;
        int32_t i = IndexOf(m->Guild);
        if (i >= 0 && i < 31 && (it->Guilds & (1 << i)) != 0) return true;
    }
    return false;
}

}  // namespace mc
