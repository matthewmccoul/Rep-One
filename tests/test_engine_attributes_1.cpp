#include <cstdint>

#include "test_support.h"
#include "mordor/attributes.h"

// SKIP B_0058#34: CharacterPanel.cs is UI code in the port; no equivalent function in the attributes module.
// SKIP B_0062#19: RaceData.ByName and race max lookup are not in the headers; reaching the raise ceiling needs repeated raiseStat simulation.

// Stat order in the array follows StatIndex: Str, Int, Wis, Con, Cha, Dex.

TEST(engine_B_0061_26_statsad_str_int_wis_dex_12_14_14) {
    mordor::Arr<int32_t, mordor::kStatCount> s{12, 14, 14, 10, 10, 12};
    CHECK_EQ(mordor::statsAD(s), 0);
}

TEST(engine_B_0061_27_statsad_all_18) {
    mordor::Arr<int32_t, mordor::kStatCount> s{18, 18, 18, 10, 10, 18};
    CHECK_EQ(mordor::statsAD(s), 32);
}

TEST(engine_B_0061_28_statsad_all_30_20) {
    mordor::Arr<int32_t, mordor::kStatCount> s{30, 20, 20, 10, 10, 30};
    CHECK_EQ(mordor::statsAD(s), 84);
}

TEST(engine_B_0061_145_conlosschance_con24) {
    CHECK_EQ(mordor::conLossChance(24), 2);
}

TEST(engine_B_0061_146_conlosschance_con23) {
    CHECK_EQ(mordor::conLossChance(23), 4);
}

TEST(engine_B_0061_147_conlosschance_con21) {
    CHECK_EQ(mordor::conLossChance(21), 8);
}

TEST(engine_B_0061_148_conlosschance_con27) {
    CHECK_EQ(mordor::conLossChance(27), 0);
}

TEST(engine_B_0061_149_conlosschance_con25) {
    CHECK_EQ(mordor::conLossChance(25), 0);
}
