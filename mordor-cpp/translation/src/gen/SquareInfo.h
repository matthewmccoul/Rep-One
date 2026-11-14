#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SquareInfo {
    inline static const cs::String StairsUp = "There are Stairs Up here.";
    inline static const cs::String StairsDown = "There are Stairs Down here.";
    inline static const cs::String Chute = "There is a chute here.  Select TAKE to use.";
    inline static const cs::String HoveringOverPit = "You are hovering over a pit.";
    inline static const cs::String Drowning = "You are Drowning!";
    inline static const cs::String Sinking = "You are Sinking!";

    static cs::String FellIntoPit(int32_t hits);
    static cs::String TheyFellIntoPit(cs::String who, int32_t hits);
    static cs::String Of(cs::Ref<Floor> fl, int32_t x, int32_t y);
};

}  // namespace mc
