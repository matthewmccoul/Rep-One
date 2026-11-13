// death3.cpp: morgue, revival, raise and records rules (chapter 18 part c18p3).
//
// Definitions NOT in this file (declared in death3.h but owned by death1b.h per the API INDEX):
//   i64 hallStatField(i64 value);
//   i64 hallDateField(i64 daysSince1900);
// CONFLICT: death3.h redeclares these two; the API INDEX assigns them to death1b. They are not
// defined here to keep every function defined exactly once.
//
// Delegations to other modules (API INDEX):
//   morgueCounterChance(i32 naturalCon, DeathState cause)   [death1a.h]
//   spellRaiseChance(i32 naturalCon, RaiseKind, bool causeIsRock) [death1b.h]

#include "mordor/death3.h"

#include <cmath>

#include "mordor/death1a.h"
#include "mordor/death1b.h"
#include "mordor/events.h"
#include "mordor/rng.h"

namespace mordor {
namespace {

// R-rule constants local to this file.
// FIXED: renamed to avoid ambiguity with same-named declarations in mordor:: (values unchanged).
constexpr i32 kLocalRaiseChanceMin = 10;   // clamp floor for morgue thresholds (engine, c19)
constexpr i32 kLocalRaiseChanceMax = 98;   // clamp ceiling for morgue thresholds (engine, c19)
// UNKNOWN: race maximum-age table is not in the input. 100 is a placeholder (c25 notes lifespan 100).
constexpr i32 kDefaultRaceMaxAge = 100;
// UNKNOWN: unit of ageDays vs the age used by the complication test (years assumed).
// FIXED: renamed to avoid ambiguity with same-named declarations in mordor:: (value unchanged).
constexpr i32 kLocalDaysPerYear = 365;

}  // namespace

// R18 age added by cause: 54 quiet, 84 stoned, 124 rock (c27, note 5).
// UNKNOWN: port equivalents for causes 3-7 (Destroyed, Immolated, Frozen, Melted, Annihilated);
// they take the quiet-failure value here.
i32 raiseAgeCost(DeathState cause) {
    if (cause == DeathState::Stoned) {
        return kAgeStoned;
    }
    if (cause == DeathState::Rock) {
        return kAgeRock;
    }
    return kAgeQuietFailure;
}

// hallDateField and hallStatField: see the note at the top of this file (owned by death1b).

}  // namespace mordor
