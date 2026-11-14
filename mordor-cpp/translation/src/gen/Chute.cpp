#include "core.h"

namespace mc {

bool Chute::Floats(cs::Ref<Character> m, cs::Ref<cs::Random> rng, int32_t dungeonLevel) {
    return m->Levitating && Rules::HoversOverPit(rng, dungeonLevel);
}

cs::String Chute::Land(cs::Ref<Character> m, bool onScreen, bool onLeadersFloor, int32_t landedLevel, cs::Ref<cs::Random> rng) {
    if (m->Levitating) return cs::String();
    int32_t hits = Rules::ChuteDamage(rng, landedLevel, static_cast<int32_t>(RaceData::ByName(m->Race)->Size));
    m->CurHits = cs::Wrap<int32_t>((int64_t)m->CurHits - (int64_t)hits);
    if (onScreen) return "You have fallen into a chute for " + cs::ToStr(hits) + " hits.";
    return onLeadersFloor ? "\"" + cs::Str::TrimEnd(m->Name) + "\" has fallen into a chute for " + cs::ToStr(hits) + " hits." : cs::String();
}

cs::String Chute::Disappeared(cs::Ref<Character> m) {
    return "\"" + cs::Str::TrimEnd(m->Name) + "\" has disappeared from view!";
}

void Chute::MoveDown(cs::Ref<Character> m, int32_t level, int32_t x, int32_t y, cs::Ref<Floor> landed) {
    m->DungeonLevel = level; m->X = x; m->Y = y;
    m->Area = landed->AreaIdAt(x, y);
}

}  // namespace mc
