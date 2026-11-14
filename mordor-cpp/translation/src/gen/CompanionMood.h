#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionMood_Step {
    cs::String Drifted{};
    bool Leaving = false;
    cs::String Left{};
};

struct CompanionMood {
    using Step = CompanionMood_Step;
    static cs::Ref<CompanionMood_Step> Round(cs::Ref<Character> who, int32_t index, cs::Ref<Monster> def, cs::Ref<cs::Random> rng, bool isLeader);
    static double Drift(cs::Ref<Character> who, cs::Ref<Companion> c, cs::Ref<Monster> def);
};

}  // namespace mc
