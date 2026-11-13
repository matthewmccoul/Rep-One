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

}  // namespace mordor
