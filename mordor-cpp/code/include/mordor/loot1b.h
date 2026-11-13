#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

// Part c14p1: chest generation, chest traps, chest gold and gold handling.
// Already declared elsewhere and called, not redeclared: withering() (progression2.h),
// setDead() (status1b.h), sub0930() (status1b.h), identifyFee() and sellPrice() (items1a.h).
// Slime item destruction wraps the SlimeInput/SlimeResult logic of items1b.h.
// Community named laired drops (T14) are unverified and carry no rule, so they are not declared.

namespace mordor {

// ---- Constants and tables ----

inline constexpr i32 kChestClassCount = 4;            // T2 classes 0..3
inline constexpr i32 kChestTrapBitCount = 11;         // T3 bits 0..10 (bit 11 UNKNOWN-2)

// FIXED: ChestClassMod is not in the API index; defined locally because only this file uses it.
// Field meanings are not given in this part, so they are named by position.
struct ChestClassMod {
    f64 goldFactor = 0.0;
    i32 rollAdjust = 0;
    i32 bandAdjust = 0;
    i32 trapChancePercent = 0;
};

// T2 class modifier table, index = ChestClass value.
inline constexpr Arr<ChestClassMod, kChestClassCount> kChestClassMods{
    ChestClassMod{0.5, 0, 0, 0},
    ChestClassMod{0.5, 15, -1, 25},
    ChestClassMod{1.0, 0, 0, 75},
    ChestClassMod{1.5, -25, 0, 90}
};

// T4 item band low end by drop level 1..16 (community tier). Equals
// max(1, roundHalfEven(level/2)); see itemBandLowEnd().
// FIXED: kItemBandLevels is not in the API index; the table has 16 entries (levels 1..16).
inline constexpr Arr<i32, 16> kItemBandLowEnd{
    1, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7, 8, 8
};

// T1 BoxChance thresholds as the rule uses them: BoxChance[1] and BoxChance[2].
struct BoxChanceThresholds {
    i32 chance1 = 0;
    i32 chance2 = 0;
};


// ---- Chest class and treasure generation ----


// T1/Q3 chest class from BoxChance thresholds; later k overwrites (no break); class 1 never produced
// CONFLICT-2: bonus target (which threshold(s) receive the bonus) as written in the table
ChestClass rollTreasureClass(Ctx& ctx, BoxChanceThresholds boxChance, bool lairMonster);











// ---- Chest opening ----















// ---- Chest traps (T3) ----
















// T3 Withering (index 10): handled by withering() in progression2.h (adds kWitheringDays)
// CONFLICT-28: message count (4 weeks older message is a third message)

// Q16/Q15 chest trap dispatch; the trap is resolved before any gold or items are paid
// Only the slime and fear messages come from the trap routine itself (plus the withering message via withering())
// FIXED: ChestTrapResult is not declared anywhere; replaced by plain i32 (closest available result type).
i32 resolveChestTrap(Ctx& ctx, ChestTrap trap, Party& party, i32 opener,
                     FloorState& floor, SquareState& chest);

// ---- Chest gold, split and gold handling (T7-T9) ----





// Q33 give takes CLng(amount) in whole gold; the fraction stays in the giver's purse
Currency giveGold(Ctx& ctx, Purse& giver, Purse& receiver, Currency amount);





// ---- Chest contents and charms (T10, T13, T15, Q47) ----





}  // namespace mordor
