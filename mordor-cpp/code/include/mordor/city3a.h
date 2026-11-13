#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"
#include "mordor/items3a.h"
#include "mordor/city2a.h"

namespace mordor {

// ---- Constants (values the SRD part gives as numbers) ----

// Store off-by-one: cap is 14 while the game has 15 floors. Floor 15 never stocked.
inline constexpr i32 kStoreFloorCap = 14;
// Pen restock cap read in the add path (test reads the Good pen only).
inline constexpr i32 kPenRestockCap = 9;
// Price formula stock = min(penned - 1, kPenStockCap). R12
inline constexpr i32 kPenStockCap = 9;
// Price formula numerator and ln offset. R12
inline constexpr double kPenPriceNumerator = 1.5;
inline constexpr double kPenPriceLnOffset = 1.15;
// Price formula floor on BaseValue. R12
inline constexpr i64 kPenPriceMinBase = 10;
// Starting companion count: 10 indices x 2 copies. R16
inline constexpr i32 kStartingCompanionCount = 20;
inline constexpr i32 kRealignCrossFactor = 2;
// Store realign: Good to Evil doubles the fee. R14
// UNKNOWN: rounding mode of the fee is not stated; implementer must choose (see R13).
// R15 CONFLICT: port refuses same-alignment realign; original asks and takes the fee.
inline constexpr bool kRealignRefusesSameAlignment = true;
// Store choice rows change while choosing alignment. R3
inline constexpr bool kStoreAllowsRowChangeWhileChoosing = false;
// Seer locates other saved characters in the port. R18 UNKNOWN: original scope unclear.
inline constexpr bool kSeerLocatesCharacters = true;
// Store combining at zero charge is allowed for these kinds in the original. (FRMCTYS.BAS:662-668)
inline constexpr Arr<i32, 8> kZeroChargeCombineKinds = {23, 24, 25, 26, 27, 28, 30, 31};

// ---- Enums and small result structs ----

// Where a library sighting writes its Location. R24, R25
enum class SightingPlaceKind : i32 {
    Keep = 0,   // keep the old place
    Unknown,    // write "?"
    City,       // write "the City"
    Square      // write the live party square
};


// ---- Shared draw helper ----

// Rounded index bias: VB Round of Rnd*n, so index 0 comes up half as often.
// Index n matches nothing and is retried internally; returns 0..n-1. Used by store and pens.


// ---- Companion pens (R12, R16, quirks) ----

// R12 pen buy price from BaseValue and pen count (ln form, floor 10). Empty pen gives 0.
// UNKNOWN: unit of BaseValue; implementer treats the result as Currency per currencyFromWhole.
Currency penBuyPrice(i64 baseValue, i32 penned);

// ---- Store stock (R22, R28, quirks) ----

// Full store restock across floors 1..deepestStockedFloor. Uses Ctx rng.

// Replenish branch when someone bought one; only the Unaligned count is checked (quirk).

// R13, R14 store realign fee: Round(0.72 * Price), doubled Good to Evil. UNKNOWN: rounding mode.
Currency storeRealignFee(Currency price, ItemAlign from, ItemAlign to);

// ---- Companion roster and confinement (R16, R17, R20) ----


// ---- Bank and gold entry (R8, quirks) ----


// ---- Seer (quirks, R18) ----


// ---- Library sightings (R23-R27, quirks) ----


}  // namespace mordor
