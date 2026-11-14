#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionMishap {
    using Kind = CompanionMishap_Kind;

    static int32_t BodiesOnYourSide(cs::Ref<Party> party);
    static double Chance(CompanionMishap_Kind kind, int32_t companionGuildLevel, int32_t bodies);
    static cs::Nullable<int32_t> Victim(cs::Ref<cs::Random> rng, CompanionMishap_Kind kind, int32_t companionGuildLevel, cs::Ref<Party> party, int32_t owner);
};

}  // namespace mc
