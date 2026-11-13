#include "test_support.h"
#include "mordor/spells1a.h"
#include "mordor/companions1a.h"

using namespace mordor;

// Case ids are sanitized for C identifiers: B_0064#NN -> B_0064_NN.

TEST(engine_B_0064_61_special_range_grows_with_attacker) {
    CHECK(specialHitRange(300, 50) > specialHitRange(30, 50));
}

TEST(engine_B_0064_71_range_not_equal_monster_breath_range) {
    CHECK(!(specialHitRange(120, 40) == monsterBreathRange(120, 40)));
}

TEST(engine_B_0064_72_monster_range_exceeds_special_by_at_least_one) {
    CHECK(monsterBreathRange(120, 40) - specialHitRange(120, 40) >= 1);
}

// SKIP B_0064#62: compares total damage over 20000 simulated rolls; needs a simulation harness.
// SKIP B_0064#63: compares total electrocution damage over 20000 simulated rolls; needs a simulation harness.
// SKIP B_0064#73: counts breath firings over 3000 rounds of a party-of-three simulation; needs a simulation harness.
// SKIP B_0064#74: counts monster-directed breaths over 3000 simulated rounds; needs a simulation harness.
// SKIP B_0064#75: counts breath mishaps over 3000 simulated rounds; needs a simulation harness.
// SKIP B_0064#76: checks mishap and firing totals over 3000 simulated rounds; needs a simulation harness.
// SKIP B_0064#77: measures spit landing percentage over 20000 runs with no living companions; needs a simulation harness.
