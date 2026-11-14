#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Teleporting {
    using Outcome = Teleporting_Outcome;

    static Teleporting_Outcome ForSpell(cs::List<cs::Ref<Floor>> dungeon, int32_t level, int32_t x, int32_t y);
    // (Level, X, Y)
    static cs::List<std::tuple<int32_t, int32_t, int32_t>> ForSquare(cs::List<cs::Ref<Floor>> dungeon, int32_t fromLevel,
                                                                     Floor_Teleporter t, int32_t members, cs::Ref<cs::Random> rng);
};

}  // namespace mc
