#pragma once
// City part 3b (SRD c16p3): store stock replenish, starting companion stock,
// companion buy price, store alignment test, sighting merge rules, Guild Log constants.
// Declared elsewhere (API INDEX, not redeclared here): priceToRealign (items2b.h),
// restInCity (progression2.h), identifyFee and sellPrice (items1a.h / items3a.h),
// Seer locate routines (spells1b.h, LocateKind / LocateLine).

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"

namespace mordor {

// ---- constants ----

// [c9] companion buy price: value = max(BaseValue, 10)
inline constexpr i64 kCompanionPriceFloor = 10;
// [c9] companion buy price: stock = min(penned(align) - 1, 9)
inline constexpr i32 kCompanionStockCap = 9;
// [c9] starting companions: ten creature indices
inline constexpr i32 kStartingCompanionTypes = 10;
// [c9] starting companions: two of each index
inline constexpr i32 kStartingCompanionPerType = 2;
// [c22] store replenish chance operand: days / 100 * 100
// UNKNOWN: literal or integer division for "/" [c22]. Implemented as integer division (simplest).
inline constexpr i32 kStoreReplenishStep = 100;
// [c49] Guild Log clean keeps the newest entries; docs say fifty
// UNKNOWN: KeepOnClean value (dialog text uses KeepOnClean) [c49]. Documented value only.
inline constexpr i32 kGuildLogKeepDocumented = 50;

// ---- store stock and replenish ----




// ---- companion pricing ----


// ---- sighting library merge rules ----




// ---- deliberately not declared (insufficient rule data in this part) ----
// Realign fee: covered by priceToRealign (items2b.h). Store vs Confinement factor and
//   same-alignment behaviour are CONFLICT [c9, c29, c65]; not redeclared.
// Confinement stock draw, pool counts and mode meanings: UNKNOWN [c94].
// Monster refresh on rest and buyout redraw: UNKNOWN [c95].
// Stock refresh arrival order (Hall, rest, replenish, lost flags): UNKNOWN [c48, c2, c29].
// Seer fee, success roll, DeepestHeCanReach, SearchFailsEarly, Blur mapping: UNKNOWN [c25, c50, c87, c91].
// Guild Log NeedsCleaning comparison: UNKNOWN [c49].
// Library last-seen clock, IdLevelRoll arithmetic, listing order: UNKNOWN [c12, c51, c57].
// Companion number entry from typed value: formula not stated; example in [c50] is wrong.
// Store price rounding (SellPrice, IdentifyFee, PriceOf): CONFLICT [c34]; see identifyFee / sellPrice.
// Town switch vs rest gate (DungeonLevel <= 0 vs == 0): UNKNOWN [c68, c48]; see restInCity.
// Confinement sale price, bind-raise fail chance, purchase price, price floor 1000: UNKNOWN [c83, c92].
// Fit A/B/C and ECV model: no game source [c92].

}  // namespace mordor
