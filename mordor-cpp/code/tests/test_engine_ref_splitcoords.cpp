#include "test_support.h"
#include "mordor/ref_splitcoords.h"

// B_0066#70: Rescue.SplitCoords given "15,5,3" returns 15|5|3.
TEST(engine_ref_splitcoords_b0066_70_three_pieces) {
    mordor::Vec<mordor::i32> r = mordor::splitCoords(mordor::Str("15,5,3"));
    CHECK_EQ(static_cast<mordor::i32>(r.size()), 3);
    if (r.size() == 3) {
        CHECK_EQ(r[0], 15);
        CHECK_EQ(r[1], 5);
        CHECK_EQ(r[2], 3);
    }
}

// B_0066#71: Rescue.SplitCoords given "15,5,3," (trailing comma) returns 15|5|3.
TEST(engine_ref_splitcoords_b0066_71_trailing_comma_three_pieces) {
    mordor::Vec<mordor::i32> r = mordor::splitCoords(mordor::Str("15,5,3,"));
    CHECK_EQ(static_cast<mordor::i32>(r.size()), 3);
    if (r.size() == 3) {
        CHECK_EQ(r[0], 15);
        CHECK_EQ(r[1], 5);
        CHECK_EQ(r[2], 3);
    }
}

// B_0066#72: Rescue.SplitCoords given "1,2," (trailing comma) returns 2 pieces.
TEST(engine_ref_splitcoords_b0066_72_trailing_comma_two_pieces) {
    mordor::Vec<mordor::i32> r = mordor::splitCoords(mordor::Str("1,2,"));
    CHECK_EQ(static_cast<mordor::i32>(r.size()), 2);
    if (r.size() == 2) {
        CHECK_EQ(r[0], 1);
        CHECK_EQ(r[1], 2);
    }
}
