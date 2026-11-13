#pragma once
#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

namespace mordor {

// R36 constants
inline constexpr double kOldAgeStartPct = 98.0;   // R36 random old-age death begins at 98% of max age
inline constexpr double kOldAgeBaseChance = 5.0;  // R36 base term of the threshold: 5 + Ln(pct-98)

// R36 age as a percentage of max age: (ageDays/365) / raceLifespanYears * 100.
// CONFLICT: quick reference names this raceLifespan; the term list names it RaceMaxAge. Treated as the same value.
// UNKNOWN: race lifespan is not a field of GameData in this part; the caller supplies it in years.
// VB '/' is floating-point division, so the age in years is fractional.






}  // namespace mordor
