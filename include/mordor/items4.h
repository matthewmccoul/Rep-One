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









}  // namespace mordor
