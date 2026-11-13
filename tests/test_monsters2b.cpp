#include "test_support.h"
#include "mordor/monsters2b.h"

using namespace mordor;

// Stud squares raise the target floor by one.
TEST(encounter_target_floor_adds_one_on_stud) {
    CHECK_EQ(encounterTargetFloor(5, false), 5);
    CHECK_EQ(encounterTargetFloor(5, true), 6);
}

