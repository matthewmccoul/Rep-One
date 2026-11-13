#pragma once

// items2a: SRD c13p2 routines not already in the API INDEX.
// The source excerpt gives tables T-1..T-11 and notes, with no R-numbers,
// so comments cite the table id (T-n) instead.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/attributes.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"

namespace mordor {

// ---- constants ----

// T-1 pack slot numbering. Internal index 0 = player slot 1 (Hands).
inline constexpr i32 kHandsIndex = 0;
// T-1 usable pack items (array size kPackSlots = 40, usable 39).
inline constexpr i32 kUsableItemSlots = 39;

// Theft slot = Int(Rnd * kTheftSpan) + kTheftFirstSlot, giving 2..40.
inline constexpr i32 kTheftFirstSlot = 2;
inline constexpr i32 kTheftSpan = 39;

// Slime: roll 1..100; no effect when roll >= kSlimeBaseChance + kSlimePerLevel * dungeonLevel.
inline constexpr i32 kSlimeBaseChance = 50;
inline constexpr i32 kSlimePerLevel = 2;

// T-8 Seer band and carrier count.
inline constexpr i32 kSeerBands = 3;
inline constexpr i32 kSeerMaxCarriers = 3;

// T-8 stat ceiling = race Max + 5 (silent cap). Floor = race minimum.
inline constexpr i32 kStatCeilingAboveRace = 5;

// T-8 Asmodeus band: ItemDropLevel - 2 .. ItemDropLevel.
inline constexpr i32 kAsmodeusBandWidth = 2;

// T-9 spell buffer encoding.
inline constexpr i32 kBufferEmpty = -1;
inline constexpr i32 kBufferSpellBase = 40;

// T-4 SpecialType values (MDATA3).
inline constexpr i32 kSpecialStatMods = 1;
inline constexpr i32 kSpecialScrollOfSpells = 11;
inline constexpr i32 kSpecialTomeOfSpells = 12;
inline constexpr i32 kSpecialPotionOfYouth = 13;
inline constexpr i32 kSpecialDragonsBlood = 14;
// UNKNOWN: 25 may be a SpecialType or an item type. [c11] vs [c26].
inline constexpr i32 kSpecialAgeTypeTwentyFive = 25;

// T-4 spell point gains and age effects (days). 1 year = 365 days (kAgeStartDays = 16 * 365).
inline constexpr i32 kScrollSpellPoints = 50;
inline constexpr i32 kTomeSpellPoints = 200;

// T-7 guild item-use factors (other guilds UNKNOWN).
inline constexpr double kNomadUseFactor = 0.8;
inline constexpr double kHealerUseFactor = 1.5;
// Guild use level = max(1, (f + f * kGuildUseLevelAdd) * item.LevelScale).
inline constexpr double kGuildUseLevelAdd = 2.97;

inline constexpr i32 kRealignFarMultiplier = 2;

// T-8 chance clamps, indexed by ItemChance.
inline constexpr i32 kItemChanceMin[4] = {10, 5, 10, 25};
inline constexpr i32 kItemChanceMax[4] = {90, 80, 90, 90};

// ---- enums ----

// T-8 chance kinds.
enum class ItemChance : i32 {
    Identify = 0,
    Recognise,
    Rename,
    Destroy
};

// T-9 classification of a spell buffer value.
enum class BufferKind : i32 {
    Empty = 0,
    ForItem,
    ForSpell
};

// ---- rule routines ----

i32 packIndexFromPlayerSlot(i32 playerSlot);  // player slot 1..40 -> internal index 0..39
i32 playerSlotFromPackIndex(i32 index);       // internal index 0..39 -> player slot 1..40




i32 adjustStatModForFit(FitClass fit, i32 raw); // Full raw; Half floor(raw / 2.0); Subtract -raw



i32 rollTheftSlot(Ctx& ctx);                    // Int(Rnd * 39) + 2 -> 2..40

i32 bufferForSpell(i32 spellIndex);             // 40 + spellIndex


}  // namespace mordor
