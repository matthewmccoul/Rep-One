#include "test_support.h"
#include "mordor/ref_goldfromtag.h"

using namespace mordor;

// B_0059#9: gold tag "50" -> 50.
TEST(engine_ref_goldfromtag_b0059_9_plain_integer) {
    i64 out = 0;
    bool ok = goldFromTag(Str("50"), out);
    CHECK(ok);
    CHECK_EQ(out, 50);
}

// B_0059#10: gold tag "2.5" -> 2 (CLng rounds half to even).
TEST(engine_ref_goldfromtag_b0059_10_half_to_even_down) {
    i64 out = 0;
    bool ok = goldFromTag(Str("2.5"), out);
    CHECK(ok);
    CHECK_EQ(out, 2);
}

// B_0055#103: GoldFromTag("10.5") -> 10 (CLng rounds half to even).
TEST(engine_ref_goldfromtag_b0055_103_half_to_even_even) {
    i64 out = 0;
    bool ok = goldFromTag(Str("10.5"), out);
    CHECK(ok);
    CHECK_EQ(out, 10);
}
