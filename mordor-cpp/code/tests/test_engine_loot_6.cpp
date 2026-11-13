#include "test_support.h"
#include "mordor/loot1a.h"
#include "mordor/loot1b.h"
#include "mordor/state_character.h"

using namespace mordor;

// SKIP B_0065#17: needs a PayGold fight simulation (gold on hand vs. amount owed); no single-call equivalent in headers
// SKIP B_0065#18: needs AwardSpoils fight flow; no single-call equivalent in headers
// SKIP B_0065#26: expects a "none" result for a logarithm error; openingRoundsForDisarm returns a plain i32 with no error channel
// SKIP B_0065#27: needs a seeded wall-monster encounter simulation to decide encounter gold; no single-call equivalent

TEST(engine_B_0065_21_rounds_disarm11) {
    CHECK_EQ(openingRoundsForDisarm(11), 10);
}

TEST(engine_B_0065_22_rounds_disarm12) {
    CHECK_EQ(openingRoundsForDisarm(12), 9);
}

TEST(engine_B_0065_23_rounds_disarm20) {
    CHECK_EQ(openingRoundsForDisarm(20), 5);
}

TEST(engine_B_0065_24_rounds_disarm60) {
    CHECK_EQ(openingRoundsForDisarm(60), 2);
}

TEST(engine_B_0065_25_rounds_disarm99) {
    CHECK_EQ(openingRoundsForDisarm(99), 2);
}

TEST(engine_B_0065_28_m1_chest_count_after_round) {
    tsup::Env env({0.5, 0.9});
    Character m1{};
    m1.chestCountdown = 2;
    tickChestCountdown(env.ctx, m1);
    CHECK_EQ(m1.chestCountdown, 1);
}
