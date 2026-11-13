#include "test_support.h"
#include "mordor/progression1b.h"
#include "mordor/progression2b.h"

using mordor::i32;
using mordor::i64;

// SKIP B_0061#4: Fn13ED has no clear equivalent in the headers (levelXpWikiC88 and xpForPin are not confirmed to match its semantics).
// SKIP B_0061#5: Requires looping over every guild and race table and comparing against an approximation (simulation over full tables).

TEST(engine_B_0061_1_level99_total_xp) {
    CHECK_EQ(mordor::levelXpWikiC88(99, 8.0, 3.0), 872326);
}

TEST(engine_B_0061_2_level79_total_xp) {
    CHECK_EQ(mordor::levelXpWikiC88(79, 8.0, 3.0), 556907);
}

TEST(engine_B_0061_3_level65_total_xp_i64) {
    i64 xp = mordor::levelXpWikiC88(65, 8.0, 3.0);
    CHECK_EQ(xp, 378045);
}

TEST(engine_B_0061_21_thief_level755_gold_range) {
    i64 gold = mordor::levelUpGoldCost(755, 9.0);
    CHECK(gold >= 32000000LL * 10000LL);
    CHECK(gold <= 33000000LL * 10000LL);
}

TEST(engine_B_0061_22_thief_levels1_754_gold_total) {
    i64 total = 0;
    for (i32 l = 1; l <= 754; ++l) {
        total += mordor::levelUpGoldCost(l, 9.0);
    }
    CHECK(total >= 8000000000LL * 10000LL);
    CHECK(total <= 9000000000LL * 10000LL);
}

TEST(engine_B_0061_23_thief_levels1_998_gold_total) {
    i64 total = 0;
    for (i32 l = 1; l <= 998; ++l) {
        total += mordor::levelUpGoldCost(l, 9.0);
    }
    CHECK(total >= 18000000000LL * 10000LL);
    CHECK(total <= 19000000000LL * 10000LL);
}

TEST(engine_B_0061_24_nomad_level3_gold) {
    CHECK_EQ(mordor::levelUpGoldCost(3, 3.0), 352LL * 10000LL);
}

TEST(engine_B_0061_25_warrior_level1_gold) {
    CHECK_EQ(mordor::levelUpGoldCost(1, 1.0), 33LL * 10000LL);
}
