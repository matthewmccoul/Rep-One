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
double oldAgePercent(const Character& ch, i32 raceLifespanYears);

// R36 death threshold 5 + Ln(pct-98) for pct >= 98.
// UNKNOWN: at exactly pct == 98 the original's Ln(0) is a runtime error. The port guards it by
// dropping the Ln term at pct <= 98, so the threshold is 5 there.
double oldAgeThreshold(double pct);

// R36 whether the random old-age check applies: character alive, in the dungeon, and pct >= 98
bool oldAgeCheckApplies(const Character& ch, i32 raceLifespanYears, bool inDungeon);

// R36 random old-age death in the dungeon. Rolls ctx.rng.below(100) only when the check applies.
// On death, marks the character dead through setDead (declared in status1b.h) and emits an event.
// Death from old age is an ordinary death and can be raised; the archive says it "still [is] not a permanent one".
// FIXED: OldAgeCheck was not declared in any header; replaced by bool (true = the character died of old age).
bool rollOldAgeDeath(Ctx& ctx, Character& ch, i32 raceLifespanYears, bool inDungeon);

// R37 port helper: index in party.members of the party's own copy of a picked character; -1 if none.
// UNKNOWN: the match key is not given in the SRD. This declaration matches on name.
// UNKNOWN: whether the original does the same is not stated.
i32 partyCopyIndexOf(const Party& party, const Character& picked);

}  // namespace mordor
