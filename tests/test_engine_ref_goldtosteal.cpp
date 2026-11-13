#include "test_support.h"
#include "mordor/ref_goldtosteal.h"

#include <limits>

using namespace mordor;

// B_0055#93: purse 100.5, roll 40 -> 40.
TEST(engine_ref_goldtosteal_roll_within_purse) {
    i64 taken = -1;
    CHECK(goldToSteal(1005000, 40, taken));  // 100.5 gold in Currency
    CHECK_EQ(taken, 40);
}

// B_0055#94: purse 10.5, roll 99 -> 10 (half kept, rounded to even).
TEST(engine_ref_goldtosteal_purse_even_half) {
    i64 taken = -1;
    CHECK(goldToSteal(105000, 99, taken));  // 10.5 gold in Currency
    CHECK_EQ(taken, 10);
}

// B_0055#95: purse 11.5, roll 99 -> 12 (half rounded to even).
TEST(engine_ref_goldtosteal_purse_odd_half) {
    i64 taken = -1;
    CHECK(goldToSteal(115000, 99, taken));  // 11.5 gold in Currency
    CHECK_EQ(taken, 12);
}

// B_0055#140: purse MinValue, roll 40 -> null (subtraction overflows).
TEST(engine_ref_goldtosteal_minvalue_purse_null) {
    i64 taken = -1;
    CHECK(!goldToSteal(std::numeric_limits<Currency>::min(), 40, taken));
}
