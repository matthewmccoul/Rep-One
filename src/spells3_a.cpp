// spells3_a.cpp: movement spell table lookups, stat requirements, and
// teleport-in-combat helpers for the spells3 module.
#include "mordor/spells3.h"
#include "mordor/ctx.h"
#include "mordor/math_util.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"

#include <cmath>

namespace mordor {

namespace {

// Sanctuary stat requirements, from the SRD table T-G "Req" column:
// 16 Int, 20 Wis, 15 Con.
inline constexpr i32 kSanctuaryReqInt = 16;
inline constexpr i32 kSanctuaryReqWis = 20;
inline constexpr i32 kSanctuaryReqCon = 15;

}  // namespace

}  // namespace mordor
