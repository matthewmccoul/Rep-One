#pragma once
// SRD part c13p3: items and equipment. Carrying capacity, item generation,
// identification, guild use, charge and stat items, Thieving Ability.
// Declarations only. Sources are archive, forum and wiki material; no engine
// source was provided, so every routine here is unverified against code.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"

namespace mordor {

// ---- constants ----

// R1, R2, F1: player-reported magic sack slot count.
// UNKNOWN: 39 is a forum figure (attributed to a developer comment in one
// source and to a player post in another). Original count not confirmed.
inline constexpr i32 kSackSlotsReported = 39;

// F1: deduction for "one pair of hands".
// CONFLICT: Inventory.freeHands is 2 (one per hand). F1 text subtracts one
// pair. Header follows the F1 text literally; implementers may compare with
// Inventory.freeHands when the original behaviour is confirmed.
inline constexpr i32 kHandPairDeduction = 1;

// R6: number of category values in the item drop row.
inline constexpr i32 kItemCategoryCount = 10;

// R6: category value meaning "any random category".
inline constexpr i32 kAnyCategoryValue = -1;

// F2: lowest item level ever produced.
inline constexpr i32 kMinItemLevel = 1;

// F2: Asmodeus chest fixed level band.
inline constexpr i32 kAsmodeusMinLevel = 14;
inline constexpr i32 kAsmodeusMaxLevel = 16;

// F2: chest raises the minimum level by one.
inline constexpr i32 kChestMinLevelShift = 1;

// F2: container lowers both bounds by one.
inline constexpr i32 kContainerLevelShift = 1;

// F4: base of the identification difficulty formula.
inline constexpr double kIdentifyBaseDifficulty = 25.0;

// F4: additive bias inside the difficulty bracket.
inline constexpr double kIdentifyLnBias = 0.3;

// F4: guild level offset and divisor in the skill formula.
inline constexpr double kIdentifyGuildOffset = 5.0;
inline constexpr double kIdentifyGuildDivisor = 10.0;

// R11: MOSTLY needs skill >= difficulty and dungeon level at or below this.
// UNKNOWN: result when skill >= difficulty below level 1 or above this level
// is not stated. Header falls through to the gap test (see identifyLevelFromSkill).
inline constexpr i32 kIdentifyMostlyMaxDungeonLevel = 5;

// R11: PARTIAL when (difficulty - skill) is below this; BARELY at or above it.
inline constexpr double kIdentifyPartialGap = 20.0;

// F5: scale in the guild use-level formula.
inline constexpr double kGuildUseScale = 3.97;

// F5: floor of the guild use level.
inline constexpr i32 kMinUseLevel = 1;

// R16: spell points added on use.
inline constexpr i32 kScrollOfSpellsSpBonus = 50;
inline constexpr i32 kTomeOfSpellsSpBonus = 200;

// ---- enums ----

// F2 input: which container or chest modifier applies to the item level range.
enum class ItemSourceKind : i32 {
    Plain = 0,
    Chest = 1,
    Container = 2,
    AsmodeusChest = 3
};

// R6 outcome of choosing a category from the drop row.
enum class CategoryPickKind : i32 {
    Category = 0,
    AnyCategory = 1,
    FixedItem = 2
};

// ---- result and input structs ----

// F2 output: inclusive item level band used by R7.
struct ItemLevelRange {
    i32 minLevel = kMinItemLevel;
    i32 maxLevel = kMinItemLevel;
};

// R6 input: one row of Wabbit's Editor item drop data.
// categories[k] is the k-th of the 10 values below Item Drop Level.
struct ItemDropRow {
    i32 itemDropLevel = 0;
    Arr<i32, kItemCategoryCount> categories{};
};

// R6 output.
// UNKNOWN: mapping of a fixed-item value (below -1) to an item id. Header
// assumes fixedItemId = -value.
struct CategoryPick {
    CategoryPickKind kind = CategoryPickKind::Category;
    i32 category = 0;
    i32 fixedItemId = -1;
};

// R8 output: one candidate with its Chance and derived spawn percentage (F3).
struct ItemSpawnEntry {
    i32 itemId = 0;
    double chance = 0.0;
    double percent = 0.0;
};

// R9 output. roll is the draw in [0, 100).
struct ItemSpawnPick {
    bool spawned = false;
    i32 itemId = -1;
    double roll = 0.0;
};

// R15 output.
struct ChargeCastResult {
    bool cast = false;
    i32 spellId = 0;
    i32 spellLevel = 0;
    i32 timesCast = 0;
    i32 chargesLeft = 0;
};

// ---- rule routines ----


// R2 same deduction as R1: wielded items and one pair of hands come off the slot count.
// Declared as R1 (magicSackCapacity); no separate routine.

// R3 worn Blessings of Morash limit. UNKNOWN: limit for Demise not stated.
// No routine declared until the limit is known.

// R4 cards summoned from the questmaster's house. UNCONFIRMED player claim
// (one post). No routine declared.


















}  // namespace mordor
