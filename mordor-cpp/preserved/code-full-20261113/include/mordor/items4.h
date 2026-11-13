#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/state_dungeon.h"
#include "mordor/items3b.h"

namespace mordor {

// UNKNOWN: rule numbers (R#) are not given in this SRD part; declarations are grouped by table (T1-T6) instead.
// CONFLICT: carry limit (39 sack slots minus wielded items and one pair of hands) is unconfirmed forum/player text; not declared here.
// CONFLICT: Static Sash class restriction (restrict field blank vs "Class Restricted Item" text); no restriction routine declared.
// UNKNOWN: identification skill and difficulty formulas are not given; callers supply skill and difficulty.

// ---- constants ----

// T4 Author's min-level list for ItemDropLevel 1..16 (index 0 = level 1).
inline constexpr i32 kItemDropLevelMin = 1;
inline constexpr i32 kItemDropLevelMax = 16;
inline constexpr Arr<i32, 16> kItemDropMinLevel = {1, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7, 8, 8};

// T1/T2 Guild use-level lookup returns this when the item lists no use level for the guild.
// UNKNOWN: the SRD does not say how an unlisted guild is treated; -1 is used as "no use level".
inline constexpr i32 kNoGuildUse = -1;

// T2 Spell Points added on use.
inline constexpr i32 kScrollOfSpellsGain = 50;
inline constexpr i32 kTomeOfSpellsGain = 200;

// Category (drop): -1 means random; values below -1 name a fixed item.
inline constexpr i32 kDropCategoryRandom = -1;

// T5 Identification thresholds.
inline constexpr i32 kIdentifyShallowMaxLevel = 5;   // dungeon levels 1-5 use the MOSTLY row
inline constexpr i32 kIdentifyBarelyGap = 20;        // difficulty - skill >= 20 gives BARELY

// ---- result structs ----


// ---- rule routines ----

// T3 Item level range adjustment by chest or container.
// Chest: min +1, max unchanged. Box: unchanged. Container: min -1, max -1.
// Asmodeus chest: fixed min 14, max 16 (base values ignored).
// UNKNOWN: ChestClass::NONE is not listed in T3; treated as no adjustment.
ItemLevelRange adjustLevelRangeForContainer(ChestClass container, i32 baseMin, i32 baseMax);

// T4 Minimum item level for an ItemDropLevel.
// Values outside 1..16 are clamped to the table ends (UNKNOWN: out-of-range behaviour not stated).
// Rounding of the raw drop level to the table is banker's rounding (roundHalfEven); author says "probably".
i32 itemDropMinLevel(i32 itemDropLevel);

// T5 Identification result from skill and difficulty at a dungeon level.
// Row order: skill >= difficulty and level 1-5 -> MOSTLY; skill >= difficulty and level 6+ -> Unknown;
// difficulty - skill < 20 -> PARTIAL; difficulty - skill >= 20 -> BARELY.
// UNKNOWN: the result for skill >= difficulty at level 6+ is not stated; Unknown (no identification) is returned.
IdLevel identifyLevelFor(i32 skill, i32 difficulty, i32 dungeonLevel);

// T6 Cap an identification level by where it is obtained.
// Only the General Store reaches ALL; any other source is capped at MOSTLY.
// UNKNOWN: whether non-General-Store sources cap at MOSTLY is inferred from "only the General Store reaches ALL".
IdLevel capIdentifyLevel(IdLevel earned, bool generalStore);

// T5 Exchange-form identification recalculation.
// Through the exchange form (not drag and drop), identification is recalculated using the item whose ID equals levelScale.
// Example: a BARELY item passed to a PARTIAL-capable character can become MOSTLY.
// UNKNOWN: levelScale field name in ItemDef and the drag-and-drop behaviour are not stated.
void recalcExchangeIdentify(Ctx& ctx, const Character& recipient, Item& item);

// T1/T2 Guild use level of an item for a guild (minimum guild level needed to use or wield).
// Returns kNoGuildUse when the item lists no use level for that guild.
// Parenthesized numbers in T2 are read as guild levels; the SRD does not state this (UNKNOWN).
i32 itemUseLevel(const GameData& data, i32 itemId, Guild guild);

// T1/T2 True when the character's active guild level meets the item's use level for that guild.
// UNKNOWN: restriction logic for "class restricted" items is not defined; only the use-level check is declared.
bool meetsItemUseLevel(const GameData& data, const Character& c, i32 itemId);

// T2 On use, a spell-point item adds Spell Points to the character.
// Amounts are kScrollOfSpellsGain or kTomeOfSpellsGain.
// UNKNOWN: whether the gain is capped at maxSpellPoints is not stated.
void addSpellPointsFromItem(Ctx& ctx, Character& c, i32 amount);

}  // namespace mordor
