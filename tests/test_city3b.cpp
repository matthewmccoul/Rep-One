// Tests for city3b: store replenish, companion pricing, sighting merge and
// invisibility rules. Expected values are derived from the SRD text.
// Avoided: the unresolved `days / 100 * 100` division (only days 0, 100, 200
// are used, where literal and integer division agree) and the other UNKNOWN items.
#include "test_support.h"
#include "mordor/city3b.h"

using namespace mordor;

// ---- Constants stated in the SRD ----

TEST(city3b_constants_match_srd) {
  CHECK_EQ(kCompanionPriceFloor, 10);   // engine: value = max(BaseValue, 10)
  CHECK_EQ(kCompanionStockCap, 9);      // engine: stock = min(penned - 1, 9)
  CHECK_EQ(kStoreReplenishStep, 100);   // chance expression uses days / 100 * 100
  CHECK_EQ(kGuildLogKeepDocumented, 50);  // docs: "keeps the newest fifty"
  CHECK_EQ(kStartingCompanionTypes, 10);  // ten indices
  CHECK_EQ(kStartingCompanionPerType, 2); // two of each
}

// ---- Store replenish chance ----

// ---- Alignment choice in store stock ----

// ---- Companion buy price ----

// ---- Sighting merge ----

// ---- Sighting name ----

// ---- Invisible sighting skip ----

