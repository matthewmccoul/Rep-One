// spells2.cpp: spell magnitude and range, sheet constants, targeting caps, damage and heal
// rolls, protection values, and non-damage spell effects.
// R-numbers refer to SRD Part B; c-numbers refer to the source chunk ids in Part A.
//
// BLOCKED: the 12 errors reported for this file are all "SpellDef has no member named ...".
// This file uses these SpellDef members, which game_data.h must declare:
//   type (compared to "Detrimental" / "Beneficial"), baseBlank (bool), base, power,
//   hpFlag (compared to "Y"), numM, numG, verbIndex (0-8).
// The member `effect` is not in the error list, so it is already declared.
// This file cannot supply these members: it has no data source for their values and must not
// invent them. Until game_data.h declares them, this file will not compile.
//
// NOTE: spells2.h uses SpellUse without including casting1.h, so casting1.h is included
// before spells2.h in this translation unit.
#include <cmath>

#include "mordor/casting1.h"
#include "mordor/spells2.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {
namespace {

// ASSUMED: the original spell-tool spreadsheet columns are not in the data file; they are derived from the fields SpellDef really has.
struct SpellSheet {
  Str type;
  bool baseBlank = false;
  i32 base = 0;
  i32 power = 0;
  Str hpFlag;
  i32 numM = 0;
  i32 numG = 0;
  i32 verbIndex = 0;
};

// R16: charm casts take 4 targets in 1 group. CONFLICT: header kCharmCastPlan has 0/0; R16 values are used.
constexpr i32 kCharmTargets = 4;
constexpr i32 kCharmGroups = 1;
// R19: Sanctuary ages the caster "about one year". c21: current value is 365 days.
constexpr i32 kSanctuaryAgeDays = 365;
// UNKNOWN: "never set" sentinel for SanctuaryStore (R19: Sanctuary does nothing if never set).
constexpr i32 kSanctuaryUnsetLevel = -1;

// UNKNOWN: map axis orientation is assumed (N = -y, E = +x, S = +y, W = -x).
// Right of facing is the clockwise neighbour (R25 / c25: right and forwards relative to facing).
struct FacingVectors {
    i32 fx;
    i32 fy;
    i32 rx;
    i32 ry;
};

constexpr SpellVerb kVerbByIndex[9] = {
    SpellVerb::NONE,
    SpellVerb::FRIED,        // CONFLICT: test D maps verb index 1 to "Fried"; enum order has KILLED here.
    SpellVerb::KILLED,
    SpellVerb::FROZEN,
    SpellVerb::ELECTROCUTED,
    SpellVerb::CHOKED,
    SpellVerb::DISPELLED,
    SpellVerb::CHARMED,
    SpellVerb::SLEPT,
};

}  // namespace

// ---------------------------------------------------------------------------
// Lookup and magnitude (R1-R5)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Sheet constants (R6, R7)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Targeting and classification (R8, R9, R14)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Archive damage and heal (R10-R13)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Protection (R15)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Charm, bind, control (R16-R18)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Sanctuary and movement (R19, R20)
// ---------------------------------------------------------------------------

}  // namespace mordor
