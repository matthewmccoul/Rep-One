// Tests for city2b: ratio, rest days, shelf/pen caps, vault slots, library
// day phrasing, companion price bands, and the constant table values.
// Expected values are derived from the SRD text in the module notes.
#include "test_support.h"
#include "mordor/city2b.h"

#include <cmath>
#include <cstring>

using namespace mordor;

// T-K rest constants as documented.
TEST(rest_constants_match_srd) {
    CHECK(kRestNumerator == 100);
    CHECK(kRestDivisor == 3.25);
}

// T-J / R16 / R18: shelf cap 9, deepest stocked 14 = floor limit 15 - 1.
TEST(shelf_and_floor_constants) {
    CHECK_EQ(kShelfCap, 9);
    CHECK_EQ(kFloorLimit, 15);
    CHECK_EQ(kDeepestStocked, 14);
    CHECK_EQ(kDeepestStocked, kFloorLimit - 1);
}

// T-R table: entries are numbered 1..20 in order, and lowest never exceeds highest.
TEST(companion_price_table_shape) {
    for (i32 i = 0; i < 20; ++i) {
        CHECK_EQ(kCompanionPriceBands[i].level, i + 1);
        CHECK(kCompanionPriceBands[i].lowest <= kCompanionPriceBands[i].highest);
    }
}

// Buy and realign coefficients from T-K.
TEST(price_coefficients_match_srd) {
    CHECK(kBuyFactor == 1.5);
    CHECK(kBuyDivisor == 1.15);
    CHECK(kRealignFactor == 0.72);
}

