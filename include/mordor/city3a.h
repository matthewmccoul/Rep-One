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
i32 drawRoundedIndex(Ctx& ctx, i32 n);

// ---- Companion pens (R12, R16, quirks) ----

// R12 pen buy price from BaseValue and pen count (ln form, floor 10). Empty pen gives 0.
// UNKNOWN: unit of BaseValue; implementer treats the result as Currency per currencyFromWhole.
Currency penBuyPrice(i64 baseValue, i32 penned);
// Pen buy handler guard: buying is refused when the pen is empty (quirk: sell Take has no floor).
bool penBuyGuard(i32 penCount);
// Companion sell Take: decrements the pen with no floor check; may reach -1 (quirk).
void penTakeOne(i32& penCount);
// Restock cap test in the add path reads the Good pen only; may exceed 9 (quirk).
bool penAddWithinCap(i32 goodPenCount);

// ---- Store stock (R22, R28, quirks) ----

// Store off-by-one: deepest floor the store ever stocks (cap 14, game has 15 floors). R28
i32 deepestStockedFloor(i32 dungeonFloors);
// Item draw for one floor: whole-percent weights, may return -1 (no item). carryLast keeps the
// last value across passes when a run reaches the end of the file (quirk).
i32 storeItemDraw(Ctx& ctx, i32 floor, i32& carryLast);
// Full store restock across floors 1..deepestStockedFloor. Uses Ctx rng.
void restockStore(Ctx& ctx, Vec<Shelf>& shelves, i32 dungeonFloors);
// Replenish branch when someone bought one; only the Unaligned count is checked (quirk).
void replenishStoreAfterPurchase(Ctx& ctx, Vec<Shelf>& shelves, i32 floor);
// R10 starting-stock alignment test: numeric "AlignmentFlags > 1" (engine).
bool startingStockAlignmentOk(i32 alignFlags);
// Store combining at zero charge: allowed for kZeroChargeCombineKinds (port requires Charges > 0). CONFLICT
bool storeAllowsZeroChargeCombine(i32 kind);
// Store choice is cleared when a lit row is hidden by a mod; no mod: nothing cleared.
bool storeChoiceClears(bool rowHiddenByMod);
// R13, R14 store realign fee: Round(0.72 * Price), doubled Good to Evil. UNKNOWN: rounding mode.
Currency storeRealignFee(Currency price, ItemAlign from, ItemAlign to);

// ---- Companion roster and confinement (R16, R17, R20) ----

// R17 companion number entry: index = round_half_even(typed - 1). Halves go to even.
i32 companionIndexFromTyped(double typed);
// R20 confinement sale scope. UNKNOWN: sources disagree; manual form used (free-will or bound).
bool canSellFromConfinement(const Companion& c);

// ---- Bank and gold entry (R8, quirks) ----

// R8 shortfall covered from other members' bank accounts only; returns the uncovered remainder.
// CONFLICT: port addition; original had no cover.
Currency coverShortfallFromBanks(Party& party, i32 payer, Currency shortfall);
// Gold entry via CLng: "2.5" gives 2, "3.5" gives 4, "&HFFFF" gives -1 (16-bit signed).
i64 clngFromEntry(const Str& text);

// ---- Seer (quirks, R18) ----

// Seer fee is charged even when the search fails (quirk).
bool seerFeeCharged(bool searchFound);

// ---- Library sightings (R23-R27, quirks) ----

// R24, R25 place written by a sighting. Lost sighting writes "?". City reading (level 0) writes the City.
// UNKNOWN: exact conditions for the City case are not stated.
SightingPlaceKind sightingPlace(bool lost, i32 leadLevel);
// R27 invisible creature skipped only when reader's DungeonLevel > 0. CONFLICT with Set 2.
bool sightingSkipsInvisible(bool invisible, i32 readerDungeonLevel);
// R23 Known is only raised: returns max(known, grade).
i32 sightingKnownAfter(i32 known, i32 grade);
// R26 monster merge replaces the name only when the existing Known is 0. CONFLICT with Set 2.
bool mergeReplacesName(i32 existingKnown);
// Filing a partial item read: below Mostly keeps grade 0 (item hidden from listing); else the read level.
// UNKNOWN: mapping from IdLevel to grade beyond the threshold.
i32 libraryItemGrade(IdLevel read);
// Library item page: damage modifier shown only for weapons and only at grade 4.
bool showsDamageModifier(bool isWeapon, i32 grade);
// Library restock: an existing graded record is raised to grade 4 and keeps its first
// recorder's name; a grade-0 record loses its name.
void libraryRestockRecord(LibraryRecord& rec);
// Legacy "m:307" merge into the Morgul record: grade rises, counts add, newer place and
// date replace, first recorder's name kept. UNKNOWN: which place and date count as newer.
void mergeLegacyLibraryRecord(LibraryRecord& into, const LibraryRecord& legacy);

}  // namespace mordor
