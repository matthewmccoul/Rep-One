#include "core.h"

namespace mc {

Race::Race() {
    Resistances = cs::List<int32_t>::New(Effects::ResistanceKinds);
}

cs::List<Alignment> Race::Alignments() {
    return cs::List<Alignment>::Of({Alignment::Good, Alignment::Neutral, Alignment::Evil})
        .Where([&](auto& a) { return (AlignMask & (1 << static_cast<int32_t>(a))) != 0; })
        .ToArray();
}

int32_t Race::MinTotal() { return Min.Sum(); }

int32_t Race::MaxTotal() { return Max.Sum(); }

void RaceData::LoadFrom(cs::String gameDir) {
    cs::String path = cs::Path::Combine(gameDir, "MDATA1.MDR");
    auto races = MdrLoader::LoadRaces(path);
    if (races.Count() == 0) throw cs::InvalidDataException("No races could be read from " + path + ".");
    All = races;
}

cs::Ref<Race> RaceData::ByName(cs::String name) {
    auto r = All.FirstOrDefault([&](auto& r) { return r->Name == name; });
    if (r) return r;
    throw cs::KeyNotFoundException("No race named \"" + name + "\" in MDATA1 (" + cs::ToStr(All.Count()) + " races loaded).");
}

int32_t RaceData::IndexOf(cs::String name) {
    for (int32_t i = 0; i < All.Count(); i++) if (All[i]->Name == name) return i;
    return -1;
}

}  // namespace mc
