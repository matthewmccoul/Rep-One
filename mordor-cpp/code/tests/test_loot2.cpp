#include "test_support.h"
#include "mordor/loot2.h"
#include <vector>  // FIXED: needed for std::vector<double> draw lists below

namespace m = mordor;

namespace {
// Currency is fixed point with 1/10000 of a gold piece per unit.
constexpr m::Currency kGp = 10000;

// FIXED: rollChestLock takes const ChestTuning&, not a bare i32; wrap the chance in a tuning.
m::ChestTuning tuningWithLockedChance(m::i32 lockedChance) {
    m::ChestTuning t;
    t.lockedChance = lockedChance;
    return t;
}
}  // namespace

// Documented header constants (SRD c14p2 notes and header comments).
TEST(loot2_documented_constants) {
    CHECK_EQ(m::kChestCountStart, 2);
    CHECK_EQ(m::kChestTrapTries, 200);
    CHECK_EQ(m::kLairBoxBonus, 25);
    CHECK_EQ(m::kLockGoldNum, 3);
    CHECK_EQ(m::kLockGoldDen, 2);
    CHECK_EQ(m::kItemRollCapThreshold, 90);
    CHECK_EQ(m::kWitheringWeeks, 4);
}

