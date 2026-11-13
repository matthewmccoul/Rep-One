// combat2b: player and monster crit/backstab procs, companion breath sweep,
// combat-setting spell refusal, Asmodeus fight-end check, quest kill, kill XP.
// Source: SRD part c07p2 (R11, P2, R12-R17, P3-P4, R18-R24, P5-P6, R25-R28).

#include "mordor/combat2b.h"

#include <cmath>

#include "mordor/casting1.h"
#include "mordor/ctx.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

constexpr i32 kCoefficientRows = 2;  // kCritCoefficients and kBackstabCoefficients each have 2 rows

// UNKNOWN: Asmodeus monster id not supplied in SRD. -1 never matches a real monster.
constexpr i32 kAsmodeusMonsterId = -1;

}  // namespace

}  // namespace mordor
