#include "test_support.h"
#include "mordor/dungeon2a.h"

// Case ids contain '#', which is not valid in identifiers, so '#' is written as '_'.
// SKIP A_01_2: needs the shipped MDATA5.MDR/MDATA11.MDR files and EncounterLoopsProbe, which are not in the headers.
// SKIP A_02_8: needs a chute record on level 3 in the shipped data and a full party/floor setup for takeChute; not isolable with the pure helpers.
// SKIP A_02_9: needs a chute record on level 2 and level 3 in the shipped data and a full party/floor setup for takeChute; not isolable with the pure helpers.

TEST(engine_A_02_1_full_level_cap) {
    CHECK_EQ(mordor::kFullBuildLevelLimit, 15);
}

TEST(engine_A_02_2_limited_last_level) {
    CHECK_EQ(mordor::kLimitedBuildLevelLimit, 3);
}

