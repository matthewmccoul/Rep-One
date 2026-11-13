#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

// progression1b.h: SRD part c05p1, rules R13 to R27.
// Functions already in the API INDEX (ageYears, describeDays, withering,
// complication, complicationChance, restInCity, forfeitCost, Forfeit,
// goldForNextLevel, expForNextLevel, guildLevelFactor) are not redeclared here.
// All gold values in this module are WHOLE gold (SRD units). Convert with
// currencyFromWhole() before storing in Currency (1/10000 gold).

namespace mordor {

// ---- Constants and SRD tables ----

// R13 closed-form constants (test form is the reference)
inline constexpr double kGefDivisor = 8.0;                  // R13 GEF / 8





// R18 Cost = CInt((CCur((Each non-zero guild ability + 1) * 1.25) + CCur(spell class max / 3))^2 * 5356).
// CInt is roundHalfEven. UNKNOWN: how "each non-zero guild ability" and spell class maximum are formed.
i64 guildJoinCost(const Arr<f32, 6>& abilityRatings, i32 spellClassMaxLevel);

// ---- R19 Starting age and age display ----
// (ageYears and describeDays are in the API index. Constants are declared above.)
// Starting age is kAgeStartDays in state_character.h.

// ---- R20 Withering (trap 10) ----
// R20 AgeBy(days, party): adds days to the character's age and prints the aging message.
// partyMany is party.Count > 1. (withering(ctx, ch) is in the API index and calls this with kAgeDaysWithering.)
void ageBy(Ctx& ctx, Character& ch, i32 days, bool partyMany);

// R21 poison roll (draw 0..199 + 2*(effectiveCon-10) - 2*poisonLevel), then disease roll, then old-age roll.
// Paralysis uses a separate roll and is not part of this routine.
// FIXED: AilmentTickResult does not exist in any header; replaced with i32 (closest plain type).
i32 ailmentTick(Ctx& ctx, Character& ch, i32 effectiveCon, double agePercent);

// R22 Aging wiring: no routine declared. Engine Withering uses ageBy; other wiring is UNKNOWN.



// ---- R25 Failed raise (complications) ----
// R25 chance: complicationChance() in the API index.
// R25 effects: complication() in the API index.
// Constants kComplicationStatLoss, kComplicationMaxHpFactor and kAgeYearsComplication are above.



}  // namespace mordor
