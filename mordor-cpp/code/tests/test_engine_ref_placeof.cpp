#include "test_support.h"
#include "mordor/ref_placeof.h"

// B_0057#78: The place text is built from the live square (10,11) on level 3 (lead not lost).
// The expected value "11,12,3" is what the record holds after Librarian.FileSighting, which is not ported here.
// SKIP B_0057#78: expected value is the record content after Librarian.FileSighting, which is not ported; PlaceOf alone is covered by #79-#82 below.

// B_0057#79: dungeon level 2, square (0,0), not lost.
TEST(engine_ref_placeof_b0057_79_level2_origin) {
    mordor::Str out;
    CHECK(mordor::placeOf(false, false, 2, 0, 0, out));
    CHECK(out == "1,1,2");
}

// B_0057#80: dungeon level 0 (the City), square (3,3), not lost.
TEST(engine_ref_placeof_b0057_80_city_level0_null) {
    mordor::Str out;
    CHECK(!mordor::placeOf(false, false, 0, 3, 3, out));
}

// B_0057#81: dungeon level 2, square (3,3), level lost.
TEST(engine_ref_placeof_b0057_81_lost_level_null) {
    mordor::Str out;
    CHECK(!mordor::placeOf(true, false, 2, 3, 3, out));
}

// B_0057#82: dungeon level 2, square (3,3), square (position) lost.
TEST(engine_ref_placeof_b0057_82_lost_position_null) {
    mordor::Str out;
    CHECK(!mordor::placeOf(false, true, 2, 3, 3, out));
}
