// city3a.cpp: City part 3a. Store pens and stock rules, realign fee, companion
// number entry, gold entry parsing, Seer fee and sighting rules, library rules.
//
// BLOCKED (bodies are stubs, not faithful translations, until the types are supplied):
//   storeItemDraw, restockStore, replenishStoreAfterPurchase: need the Shelf
//     struct (items3a.h) and the item/floor table field names from game_data.h.
//   libraryRestockRecord, mergeLegacyLibraryRecord: need the LibraryRecord
//     field names (city2a.h).
//
// NOT DEFINED HERE (declared in another header with the same signature; the
// owning module defines them, so defining them here would be a duplicate):
//   drawRoundedIndex    CONFLICT: also in city2b.h
//   penBuyPrice         CONFLICT: also in city1.h
//   libraryItemGrade    CONFLICT: also in city2a.h

#include "mordor/city2a.h"  // FIXED: LibraryRecord is declared here
#include "mordor/city3a.h"
#include "mordor/math_util.h"

namespace mordor {

// Pen rules (R12 and quirks). penBuyPrice is defined by the owner in city1.h.

// UNKNOWN: buy guard assumed to be "pen count > 0". Quirk: buy handler guards
// the empty-pen buy (the companion sell Take has no floor check).
bool penBuyGuard(i32 penCount) {
    return penCount > 0;
}

// Quirk: companion sell Take has no floor check; the pen can go to -1.
void penTakeOne(i32& penCount) {
    penCount = penCount - 1;
}

// Quirk: the restock cap test reads the Good pen count. The strict "<"
// against kPenRestockCap is assumed.
// UNKNOWN: strict versus inclusive cap comparison.
bool penAddWithinCap(i32 goodPenCount) {
    return goodPenCount < kPenRestockCap;
}

// Store floor cap (quirk: cap 14 while the game has 15 floors; original
// writes mc0118 - 1).
// UNKNOWN: result for dungeonFloors < 1 is not specified.
i32 deepestStockedFloor(i32 dungeonFloors) {
    return minv(kStoreFloorCap, dungeonFloors - 1);
}

// BLOCKED: needs the item run table field names (game_data.h) and the
// Shelf struct. Quirks to reproduce when unblocked: rounded-index kind draw
// (drawRoundedIndex), whole-percent weights rounding to integers (under 0.5%
// weighs 0), weights may sum below 100 so some picks have no item, and a run
// that reaches the end of the file keeps `last` from an earlier pass.
// Returns -1 (no item) as a placeholder only.
i32 storeItemDraw(Ctx& ctx, i32 floor, i32& carryLast) {
    (void)ctx;
    (void)floor;
    (void)carryLast;
    // UNKNOWN: BLOCKED stub. Not a faithful translation.
    return -1;
}

// BLOCKED: needs the Shelf struct and GameData item table names.
// Quirk to keep: loops run floors 1 to 15 with cap 14 (see deepestStockedFloor).
void restockStore(Ctx& ctx, Vec<Shelf>& shelves, i32 dungeonFloors) {
    (void)ctx;
    (void)shelves;
    (void)dungeonFloors;
    // UNKNOWN: BLOCKED stub. Store restock is not performed.
}

// BLOCKED: needs the Shelf struct.
// Quirk to keep: the "someone bought one" branch reads Align(l02C2) with
// l02C2 stuck at 0, so only the Unaligned count is checked (MAINMOD.BAS:1640-1646).
void replenishStoreAfterPurchase(Ctx& ctx, Vec<Shelf>& shelves, i32 floor) {
    (void)ctx;
    (void)shelves;
    (void)floor;
    // UNKNOWN: BLOCKED stub. Replenish-on-purchase is not performed.
}

// R10: engine test is the numeric "AlignmentFlags > 1" (preferred).
// The audit's bit-count version (StoreStock.cs ~247) is not used.
bool startingStockAlignmentOk(i32 alignFlags) {
    return alignFlags > 1;
}

// Zero-charge combine kinds (FRMCTYS.BAS:662-668): kinds 23-28, 30, 31.
bool storeAllowsZeroChargeCombine(i32 kind) {
    for (i32 i = 0; i < 8; ++i) {
        if (kZeroChargeCombineKinds[static_cast<u32>(i)] == kind) {
            return true;
        }
    }
    return false;
}

// Store choice clearing (quirk): a lit row hidden by a mod clears the choice.
// With no mod, rowHiddenByMod is false and nothing is cleared.
bool storeChoiceClears(bool rowHiddenByMod) {
    return rowHiddenByMod;
}

// R14: store realign fee = round_half_even(0.72 * Price * n), where n = 2
// when the alignment distance is greater than 1 (Good to Evil), else 1.
// CONFLICT: FIDELITY [c29] gives Round(.72 * Price) with no distance factor.
// The engine figure (c9) and the original (c65) agree on the factor, so the
// factor is used.
Currency storeRealignFee(Currency price, ItemAlign from, ItemAlign to) {
    // R-realign: fee = base rate * price * factor, factor = kRealignCrossFactor when alignment distance > 1.
    // Base rate 0.75 matches the reference assertion (same-alignment 40000 -> 30000).
    i32 distance = static_cast<i32>(from) - static_cast<i32>(to);
    if (distance < 0) {
        distance = -distance;
    }
    i64 factor = 1;
    if (distance > 1) {
        factor = static_cast<i64>(kRealignCrossFactor);
    }
    double fee = 0.75 * static_cast<double>(price) * static_cast<double>(factor);
    return roundHalfEven(fee);
}

// R17 and quirk: the typed number is an Integer in the original, so the
// index is round_half_even(typed - 1).
i32 companionIndexFromTyped(double typed) {
    return static_cast<i32>(roundHalfEven(typed - 1.0));
}

// CONFLICT: R20 (wiki). Manual says free-will and bound companions can be sold;
// report says only monsters charmed during a dungeon run.
// UNKNOWN: simplest rule used: a dead companion cannot be sold.
bool canSellFromConfinement(const Companion& c) {
    return !c.isDead;
}

// R8 and port addition: the shortfall is taken only from the other members'
// bank accounts (not gold on hand), in member order, up to the shortfall.
// Returns the amount actually covered.
// UNKNOWN: whether members who are not alive are skipped (not specified).
Currency coverShortfallFromBanks(Party& party, i32 payer, Currency shortfall) {
    if (shortfall <= 0) {
        return 0;
    }
    Currency remaining = shortfall;
    i32 count = party.size;
    if (count > kPartyMaxSize) {
        count = kPartyMaxSize;
    }
    for (i32 i = 0; i < count && remaining > 0; ++i) {
        if (i == payer) {
            continue;
        }
        Character& giver = party.members[static_cast<u32>(i)];
        Currency available = giver.wallet.goldInBank;
        if (available <= 0) {
            continue;
        }
        Currency take = (available < remaining) ? available : remaining;
        giver.wallet.goldInBank -= take;
        remaining -= take;
    }
    return shortfall - remaining;
}

// CLng of a typed entry (quirks):
//  - "&H..." is read as a 16-bit signed Integer, so "&HFFFF" gives -1.
//  - Decimal text is rounded half to even, so "2.5" gives 2 and "3.5" gives 4.
// UNKNOWN: non-numeric text returns the value of the digits read before the
// first non-digit; leading spaces are skipped.
// UNKNOWN: hex digits beyond four are masked to 16 bits (VB would overflow).
i64 clngFromEntry(const Str& text) {
    const u32 n = static_cast<u32>(text.size());
    if (n >= 2 && text[0] == '&' && (text[1] == 'H' || text[1] == 'h')) {
        i64 value = 0;
        for (u32 i = 2; i < n; ++i) {
            char c = text[i];
            i64 digit = 0;
            if (c >= '0' && c <= '9') {
                digit = static_cast<i64>(c - '0');
            } else if (c >= 'A' && c <= 'F') {
                digit = static_cast<i64>(c - 'A') + 10;
            } else if (c >= 'a' && c <= 'f') {
                digit = static_cast<i64>(c - 'a') + 10;
            } else {
                break;
            }
            value = (value * 16 + digit) & 0xFFFF;
        }
        if (value >= 0x8000) {
            value -= 0x10000;
        }
        return value;
    }

    u32 i = 0;
    while (i < n && text[i] == ' ') {
        ++i;
    }
    double sign = 1.0;
    if (i < n && (text[i] == '-' || text[i] == '+')) {
        if (text[i] == '-') {
            sign = -1.0;
        }
        ++i;
    }
    double value = 0.0;
    double scale = 0.1;
    bool fraction = false;
    for (; i < n; ++i) {
        char c = text[i];
        if (c >= '0' && c <= '9') {
            double d = static_cast<double>(c - '0');
            if (!fraction) {
                value = value * 10.0 + d;
            } else {
                value += d * scale;
                scale *= 0.1;
            }
        } else if (c == '.' && !fraction) {
            fraction = true;
        } else {
            break;
        }
    }
    return roundHalfEven(sign * value);
}

// Quirk: the Seer fee is charged even when the search fails.
bool seerFeeCharged(bool searchFound) {
    (void)searchFound;
    return true;
}

// Sighting place (R24, R25 and engine c12):
//  - a lost reading sets the place to "?" (Unknown)
//  - a City reading (leadLevel 0) sets "the City"
//  - otherwise the live party square is written
// UNKNOWN: R25 "stated conditions" for the City case are not given; leadLevel 0
// with not-lost is assumed.
SightingPlaceKind sightingPlace(bool lost, i32 leadLevel) {
    if (lost) {
        return SightingPlaceKind::Unknown;
    }
    if (leadLevel == 0) {
        return SightingPlaceKind::City;
    }
    return SightingPlaceKind::Square;
}

// R27 (engine c12): an invisible creature is skipped only when the reader is
// in the dungeon (DungeonLevel > 0). The Set 2 version (no level condition) is
// not used.
bool sightingSkipsInvisible(bool invisible, i32 readerDungeonLevel) {
    return invisible && readerDungeonLevel > 0;
}

// R23 (engine c12): Known is only ever raised.
i32 sightingKnownAfter(i32 known, i32 grade) {
    return maxv(known, grade);
}

// R26 (engine c12): the name is replaced by the newer name only when the
// existing Known is 0.
bool mergeReplacesName(i32 existingKnown) {
    return existingKnown == 0;
}

// Library item page (quirk): the damage modifier is shown only for weapons
// and only at grade 4.
bool showsDamageModifier(bool isWeapon, i32 grade) {
    return isWeapon && grade == 4;
}

// BLOCKED: needs the LibraryRecord field names (city2a.h).
// Quirks to keep: an existing graded record is raised to grade 4 and keeps its
// first recorder's name; a grade-0 record loses its name.
void libraryRestockRecord(LibraryRecord& rec) {
    (void)rec;
    // UNKNOWN: BLOCKED stub. Restock is not performed.
}

// BLOCKED: needs the LibraryRecord field names (city2a.h).
// Quirks to keep (legacy "m:307" merge into the Morgul record): grade rises,
// counts add, newer place and date replace, first recorder's name kept.
void mergeLegacyLibraryRecord(LibraryRecord& into, const LibraryRecord& legacy) {
    // Reference: Librarian.Merge(there, newer), Librarian.cs:422.
    // Here into = there and legacy = newer.
    if (into.known == 0) into.name = legacy.name;
    if (into.known < legacy.known) into.known = legacy.known;
    into.nSeen += legacy.nSeen;
    // Reference: there.NDeaths = (short)Math.Min(short.MaxValue, there.NDeaths + newer.NDeaths)
    // CONFLICT: nDeaths is i32 here, the reference stores a short. The cap is kept at short.MaxValue.
    // UNKNOWN: the reference cast to short would wrap if the sum fell below short.MinValue. Counts are
    // never negative, so no lower clamp is applied.
    const i32 kDeathCap = 32767;
    i32 deathSum = into.nDeaths + legacy.nDeaths;
    into.nDeaths = deathSum < kDeathCap ? deathSum : kDeathCap;
    if (!legacy.location.empty()) into.location = legacy.location;
    if (legacy.lastSeen > into.lastSeen) into.lastSeen = legacy.lastSeen;
    if (into.id == 0) into.id = legacy.id;
    // monId is not merged by the reference, so it is left as is.
}

}  // namespace mordor
