#pragma once
#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"
#include "mordor/companions1a.h"  // FIXED: MishapKind and SpawnWindow are declared here

namespace mordor {

// Base for starting and bought bindings. R8
inline constexpr i32 kBindingBaseLevel = 5;
// Cap on starting binding: min(5 + monsterFloor, cap). R8
inline constexpr i32 kBindingStartCap = 10;
// Binding cost: Cost = monsterGuildLevel * unit * f * current / divisor. R9
inline constexpr i32 kBindingCostUnit = 1237;
// Upper end of the binding-factor table (R54: f = a + 4 if a + 4 <= 12, else 12 - a).
inline constexpr i32 kBindingFactorMax = 12;
inline constexpr f64 kDriftBoundGuildBase = 3.0;
inline constexpr f64 kDriftBoundTopBase = 12.0;
inline constexpr f64 kDriftBoundGuildScale = 2.0;
inline constexpr f64 kDriftBoundBindingDivisor = 2.0;
inline constexpr i32 kJoinChaScale = 10;
inline constexpr i32 kJoinLevelScale = 10;
inline constexpr i32 kJoinLevelOffset = 1;
// Stone chance: con * (Log(Max(1, con)) / Log(2.5)) - LevelFound, capped. R30
inline constexpr i32 kCompanionStoneCap = 95;
inline constexpr i32 kStoneConFloor = 1;
// Raise chance: min(98, 85 + CON). R42
inline constexpr i32 kCompanionRaiseBase = 85;
inline constexpr i32 kCompanionRaiseCap = 98;
inline constexpr i32 kMishapLevelFloor = 1;
// Breath fire roll percentages by relative strength. R23 [c28]
inline constexpr i32 kBreathFireBase = 50;
inline constexpr i32 kBreathFireStrong = 80;
inline constexpr i32 kBreathFireWeak = 20;
// UNKNOWN: exact comparison for "by more than a quarter" (ratio or difference). [c28]
inline constexpr f64 kBreathLeadRatio = 1.25;
// Companion blow crit bits and bonuses. R28
inline constexpr u32 kSpecialBitCrit10 = 1u << 10;
inline constexpr u32 kSpecialBitCrit11 = 1u << 11;
inline constexpr i32 kCritBit10Bonus = 4;
inline constexpr i32 kCritBit11Bonus = 1;
// Breath gives up after the fifth kill. [c28]
// UNKNOWN: engine constant mc0230 = 4 (R23); docs give fifth kill. Docs used.
inline constexpr i32 kBreathSweepGiveUpKills = 5;
// Companion caster failure percentage. [c2][c65]  CONFLICT R56 (whether companions cast).
inline constexpr i32 kCompanionCastFailPercent = 50;
// Domination charms up to this many creatures in one group. [c83]
inline constexpr i32 kDominationTargets = 4;
// Item 307 split. [c8][c9][c55]
inline constexpr i32 kId307Item = 307;
inline constexpr i32 kId307MorgulRef = 278;
inline constexpr i32 kId307ShadowNinjaRef = 389;
// Companion kills count toward owner's Kills? Engine says no. R55 (engine preferred).
inline constexpr bool kCompanionKillsCountForOwner = false;


// Factor and clamp range for a mishap kind. R24 R57
struct MishapRange {
    f64 factor;
    f64 low;
    f64 high;
};

inline constexpr MishapRange kMishapRanges[3] = {
    {2.0, 2.0, 40.0},   // Breath
    {1.5, 1.0, 40.0},   // Spell (single application of 1.5, R57)
    {1.0, 0.5, 20.0},   // Spit
};

// Result of a mishap roll against a party. R24
struct MishapResult {
    bool happened = false;
    i32 victimIndex = -1;
    f64 chance = 0.0;
};


// ---- inline trivial accessors ----
// R7 binding classes: positive = bound; <= 0 = content (free will); kBindingMax = ForLife.
inline bool isBound(const Companion& c) { return c.bindingLevel > 0; }
inline bool isContent(const Companion& c) { return c.bindingLevel <= 0; }
inline bool isForLife(const Companion& c) { return c.bindingLevel == kBindingMax; }
// R11 binding given to a companion that joined of its own free will.
// FIXED: kFreeWillBinding is declared nowhere; value -10 taken from R10 (joiner enters slot with binding -10).
inline i32 joinerBinding() { return -10; }

// ---- rules ----























}  // namespace mordor
