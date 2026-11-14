#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SpecialHits {
    using Kind = SpecialHits_Kind;

    static constexpr int32_t Fire = 0;
    static constexpr int32_t Cold = 1;
    static constexpr int32_t Electrical = 2;
    static constexpr int32_t Acid = 10;

    static int32_t Floor(double v);
    static int32_t VbInt(double v);
    static int32_t Range(int32_t attackerGuildLevel, int32_t targetLevel);
    static int32_t Raw(cs::Ref<cs::Random> rng, SpecialHits_Kind kind, int32_t attackerGuildLevel, int32_t targetLevel);
    static int32_t AfterResistance(int32_t damage, int32_t resistancePercent);
    static int32_t ResistedBy(SpecialHits_Kind kind, bool attackerBreathesFire);
};

}  // namespace mc
