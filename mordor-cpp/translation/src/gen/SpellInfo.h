#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SpellInfo {
    static constexpr int32_t MassOffset = 200;
    static constexpr int32_t OnlyAffectsBase = 75;
    static constexpr int32_t ResistBase = 50;
    static constexpr int32_t HealClass = 9;
    static constexpr int32_t CharmClass = 7;

    static cs::String Special(cs::Ref<Spell> s);
    static cs::String SpecialFor(cs::Ref<Spell> s, int32_t e);
    static cs::List<CharStatFile_Piece> Paint(cs::Ref<Spell> s, cs::Ref<Character> c);
};

}  // namespace mc
