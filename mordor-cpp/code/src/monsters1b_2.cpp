// monsters1b_2.cpp
// Joining and charm (R42-R46), clearing, refill and respawn (R47-R52), quest pick (R53),
// companion spawn mode (W2), paralysis part (W7). Each function is defined once overall
// (see monsters1b.cpp for the rest).
#include "mordor/monsters1b.h"

#include "mordor/combat3.h"
#include "mordor/math_util.h"
#include "mordor/ref_respawn.h"

namespace mordor {

namespace {

constexpr i32 kFoeGroupSlots = 4;          // SquareState::foes is Arr<FoeGroup, 4>
constexpr i64 kSecondsPerDay = 86400;

}  // namespace

}  // namespace mordor
