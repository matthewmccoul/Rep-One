// monsters1b.cpp
// Monster attack resolution (R37-R38), breath/acid/electrocution formulas (R39),
// step-up (R40), fear trap (R41), thief retarget (R40), dice and bit helpers (W15-W17).
// Functions declared in monsters1b.h are defined here or in monsters1b_2.cpp, each once.
#include "mordor/monsters1b.h"

#include <cmath>

#include "mordor/clock.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {

constexpr i32 kFoeGroupSlots = 4;  // SquareState::foes is Arr<FoeGroup, 4>

}  // namespace

}  // namespace mordor
