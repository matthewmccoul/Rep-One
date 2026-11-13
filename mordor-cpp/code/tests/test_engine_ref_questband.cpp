#include "test_support.h"
#include "mordor/ref_questband.h"

using namespace mordor;

// B_0061#157: deepest item level a quest can send a character for, at tier 10.
TEST(engine_ref_questband_deepest_band_tier10) {
    CHECK_EQ(questItemBand(10), 7);
}

// B_0061#158: item band for tier 1.
TEST(engine_ref_questband_band_tier1) {
    CHECK_EQ(questItemBand(1), 1);
}
