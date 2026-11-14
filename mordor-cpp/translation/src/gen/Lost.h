#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Lost {
    static constexpr int32_t Perception = 5;
    static cs::String Realisation(cs::Ref<Character> c);
    static bool LoseDirection(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static cs::String DirectionLostText(bool inAParty);
    static cs::List<cs::String> TryToWorkItOut(cs::Ref<Character> c, cs::Ref<cs::Random> rng);
    static cs::List<cs::String> TryToWorkItOutWith(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear);
    static cs::String OnKnownGround(cs::Ref<Character> c);
    static cs::List<cs::String> TryToWorkItOut(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear);
};

}  // namespace mc
