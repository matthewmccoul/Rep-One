#include "test_support.h"
#include "mordor/city1.h"
#include "mordor/city3a.h"

using namespace mordor;

// Documented constants (module header and SRD quirks).
TEST(city3a_documented_constants) {
    CHECK_EQ(kStoreFloorCap, 14);
    CHECK_EQ(kPenRestockCap, 9);
    CHECK_EQ(kPenStockCap, 9);
    CHECK_EQ(kPenPriceMinBase, 10);
    CHECK_EQ(kStartingCompanionCount, 10 * 2);
    CHECK_EQ(kRealignCrossFactor, 2);
}

// Rounded index bias: index 0 covers only the lower half-band.
TEST(rounded_index_low_draw_gives_zero) {
    tsup::Env env({0.04, 0.9});
    CHECK_EQ(roundedIndex(env.ctx, 10), 0);
}

TEST(rounded_index_band_of_one) {
    // 0.1 * 10 = 1.0 rounds to 1.
    tsup::Env env({0.1, 0.9});
    CHECK_EQ(roundedIndex(env.ctx, 10), 1);
}

// Index n matches nothing and is retried.
TEST(rounded_index_retries_when_index_equals_n) {
    // 0.96 * 10 = 9.6 rounds to 10 == n, so it is retried.
    // 0.21 * 10 = 2.1 rounds to 2.
    tsup::Env env({0.96, 0.21});
    CHECK_EQ(roundedIndex(env.ctx, 10), 2);
}

// Empty pen gives price 0 (original Log(0) trap reproduced).
TEST(pen_buy_price_empty_pen_is_zero) {
    CHECK_EQ(penBuyPrice(100, 0), 0);
    CHECK_EQ(penBuyPrice(500, 0), 0);
}

// Base value is floored at 10 before pricing.
TEST(pen_buy_price_base_floored_at_ten) {
    CHECK_EQ(penBuyPrice(3, 1), penBuyPrice(10, 1));
    CHECK_EQ(penBuyPrice(0, 4), penBuyPrice(10, 4));
}

// Stock is pen count less one, capped at 9, so pens of 10 and up price the same.
TEST(pen_buy_price_stock_capped_at_nine) {
    CHECK_EQ(penBuyPrice(100, 10), penBuyPrice(100, 11));
    CHECK_EQ(penBuyPrice(100, 10), penBuyPrice(100, 40));
}

// More penned companions means a larger ln term and a lower price.
TEST(pen_buy_price_falls_as_pen_grows) {
    CHECK(penBuyPrice(100, 2) < penBuyPrice(100, 1));
    CHECK(penBuyPrice(100, 5) < penBuyPrice(100, 2));
}

