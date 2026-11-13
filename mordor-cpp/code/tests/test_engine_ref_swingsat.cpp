#include "test_support.h"
#include "mordor/ref_swingsat.h"

// B_0061#122: Ninja extra swings at guild levels 56, 122, 267, 585, each vs one level lower.
TEST(engine_ref_swingsat_ninja_b0061_122) {
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 56), 1);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 55), 0);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 122), 2);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 121), 1);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 267), 3);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 266), 2);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 585), 4);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Ninja, 584), 3);
}

// B_0061#123: Warrior extra swings at guild levels 91 and 324, each vs one level lower.
TEST(engine_ref_swingsat_warrior_b0061_123) {
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Warrior, 91), 1);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Warrior, 90), 0);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Warrior, 324), 2);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Warrior, 323), 1);
}

// B_0061#124: Villain extra swings at guild levels 105 and 434, each vs one level lower.
TEST(engine_ref_swingsat_villain_b0061_124) {
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Villain, 105), 1);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Villain, 104), 0);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Villain, 434), 2);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Villain, 433), 1);
}

// B_0061#125: Paladin extra swings at guild levels 111 and 484, each vs one level lower.
TEST(engine_ref_swingsat_paladin_b0061_125) {
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Paladin, 111), 1);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Paladin, 110), 0);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Paladin, 484), 2);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Paladin, 483), 1);
}

// B_0061#126: Scavenger extra swings at guild levels 116 and 531, each vs one level lower.
TEST(engine_ref_swingsat_scavenger_b0061_126) {
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Scavenger, 116), 1);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Scavenger, 115), 0);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Scavenger, 531), 2);
    CHECK_EQ(mordor::swingsAt(mordor::Guild::Scavenger, 530), 1);
}
