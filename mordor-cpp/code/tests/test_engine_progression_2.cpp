#include "test_support.h"
#include "mordor/progression2.h"

using namespace mordor;

// B_0061 chest experience cases, mapped to chestAward(partySize, disarmChance, monsterGuildLevel, firstCharGuildLevel).
// The 1000 base award is passed as disarmChance; monster guild level is 2 in every case.

TEST(engine_B_0061_115_chest_award_guild1_party1) {
    CHECK_EQ(chestAward(1, 1000.0, 2, 1), 997);
}

TEST(engine_B_0061_116_chest_award_guild333_party1) {
    CHECK_EQ(chestAward(1, 1000.0, 2, 333), 500);
}

TEST(engine_B_0061_117_chest_award_guild999_party1) {
    CHECK_EQ(chestAward(1, 1000.0, 2, 999), 250);
}

TEST(engine_B_0061_118_chest_award_guild0_party1) {
    CHECK_EQ(chestAward(1, 1000.0, 2, 0), 1000);
}

TEST(engine_B_0061_119_chest_award_party2) {
    CHECK_EQ(chestAward(2, 1000.0, 2, 0), 960);
}

TEST(engine_B_0061_120_chest_award_party3) {
    CHECK_EQ(chestAward(3, 1000.0, 2, 0), 920);
}

TEST(engine_B_0061_121_chest_award_party4) {
    CHECK_EQ(chestAward(4, 1000.0, 2, 0), 880);
}

// SKIP B_0061#142: needs RaceData MaxAge for Human; race lifespan field names are not in the provided headers
// SKIP B_0061#143: needs RaceData MaxAge for Elf; race lifespan field names are not in the provided headers
// SKIP B_0061#144: needs RaceData MaxAge for Dwarf; race lifespan field names are not in the provided headers
