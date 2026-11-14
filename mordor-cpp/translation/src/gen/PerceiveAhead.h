#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct PerceiveAhead {
    static cs::List<int32_t> Features;
    static cs::String Alone(int32_t bit);
    static cs::String InParty(cs::String who, int32_t bit);
    static cs::Nullable<Tup_x_y_82717> SquareAhead(cs::Ref<Floor> fl, int32_t x, int32_t y, Facing facing);
    static cs::List<cs::String> Look(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, int32_t x, int32_t y, Facing facing,
                                     cs::Ref<cs::Random> rng, double bestPerception, cs::String who, bool inParty);
    static Tup_best_who_d260c Best(cs::Ref<Party> party, int32_t level, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
};

}  // namespace mc
