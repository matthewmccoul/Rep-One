#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GuildTabPage {
    static cs::List<CharStatFile_Piece> Paint(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item, cs::List<cs::Ref<Spell>> spells);
};

}  // namespace mc
