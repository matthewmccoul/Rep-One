#include "test_support.h"
#include "mordor/ref_fn1485.h"

// B_0061#58: Fn1485Price(1, 110, 1.0f, 1) -> 208
TEST(engine_ref_fn1485_price_rate_one_whole_floors_down) {
    mordor::i32 rated = 0;
    mordor::i32 price = mordor::fn1485Price(1, 110, 1.0f, 1, rated);
    CHECK_EQ(price, 208);
}

// B_0061#59: Fn1485Price(1, 100, 1.1f, 1) -> 209
TEST(engine_ref_fn1485_price_rate_1_1_just_above) {
    mordor::i32 rated = 0;
    mordor::i32 price = mordor::fn1485Price(1, 100, 1.1f, 1, rated);
    CHECK_EQ(price, 209);
}

// B_0061#60: Fn1485Price(1, 0, 1.0f, 1) -> 0
TEST(engine_ref_fn1485_price_zero_u4_is_zero) {
    mordor::i32 rated = 0;
    mordor::i32 price = mordor::fn1485Price(1, 0, 1.0f, 1, rated);
    CHECK_EQ(price, 0);
}

// B_0061#107: Fn1485Price(6, 40, 1.25f, 20) rated out parameter -> 8
TEST(engine_ref_fn1485_rated_level_half_even) {
    mordor::i32 rated = 0;
    mordor::fn1485Price(6, 40, 1.25f, 20, rated);
    CHECK_EQ(rated, 8);
}
