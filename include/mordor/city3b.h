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

// [c22] store replenish chance in percent for a shelf, from days aged
// UNKNOWN: whether item replenish has the 500-pass cap that creature restock has [c9, c22].
i32 storeReplenishChance(i32 daysAged);

// [c22] one store replenish roll; consumes ctx.rng
bool rollStoreReplenish(Ctx& ctx, i32 daysAged);

// [c22] store alignment choice is needed when AlignmentFlags > 1 (numeric test)
// CONFLICT: [c34] audit says "more than one alignment bit"; preferred: engine numeric test [c22].
bool storeOffersAlignmentChoice(i32 alignmentFlags);

// ---- companion pricing ----

// [c9][c55] companion buy price for a monster base value and count penned in that alignment.
// Returns 0 when penned == 0.
// price = (1.5 * max(baseValue, 10)) / (ln(min(penned - 1, 9) + 1) + 1.15)
// UNKNOWN: units of baseValue and result (Currency is 1/10000 gold); rounding not stated.
// CONFLICT: port test form round_half_even(1.5 * BaseValue / 1.15) with no ln term or floor [c55].
// Preferred: engine form [c9]. Implementer rounds with roundHalfEven from math_util.h.
Currency companionBuyPrice(i64 baseValue, i32 penned);

// ---- sighting library merge rules ----

// [c12] sighting Known is only ever raised: returns the larger grade
// CONFLICT: [c57] "Set Known to the grade"; preferred: raise only [c12].
// UNKNOWN: partial-read grade values below 3 [c57].
i32 mergeSightingKnown(i32 knownGrade, i32 readGrade);

// [c12] monster merge takes the newer name only when the existing record has Known == 0
// CONFLICT: [c57] says first recorder's name is kept; preferred: engine [c12].
bool sightingTakesNewName(i32 existingKnown);

// [c12] an invisible creature is skipped only when the reader is in the dungeon
// (readerDungeonLevel > 0) and the reader lacks See Invisible.
// CONFLICT: [c57] skips with no level condition; preferred: engine [c12].
bool sightingSkipsInvisible(i32 readerDungeonLevel, bool creatureInvisible, bool readerSeesInvisible);

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
