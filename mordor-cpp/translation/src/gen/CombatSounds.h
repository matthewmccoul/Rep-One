#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CombatSounds {
    static cs::List<cs::String> Of(CombatEngine_CombatLine line, cs::Ref<Character> watched, bool noPartySound,
                                   bool swing = true, bool blow = true);
    static cs::List<cs::String> OfLines(cs::List<CombatEngine_CombatLine> lines, cs::Ref<Character> watched, bool noPartySound);
};

}  // namespace mc
