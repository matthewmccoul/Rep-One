#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Chute {
    inline static const cs::String TakeHint = "There is a chute here.  Select TAKE to use.";
    inline static const cs::String FollowQuestion = "Your party leader has fallen down a chute.  Do you wish the party to follow them?";
    inline static const cs::String FollowTitle = "Follow Leader?";

    static bool Floats(cs::Ref<Character> m, cs::Ref<cs::Random> rng, int32_t dungeonLevel);
    static cs::String Land(cs::Ref<Character> m, bool onScreen, bool onLeadersFloor, int32_t landedLevel, cs::Ref<cs::Random> rng);
    static cs::String Disappeared(cs::Ref<Character> m);
    static void MoveDown(cs::Ref<Character> m, int32_t level, int32_t x, int32_t y, cs::Ref<Floor> landed);
};

}  // namespace mc
