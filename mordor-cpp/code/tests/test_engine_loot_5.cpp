#include "test_support.h"
#include "mordor/loot1b.h"

using mordor::Currency;
using mordor::splitSharePaid;

// Currency = gold * 10000. Whole-gold expectations are converted to Currency.
// Case ids contain '#', which is not valid in a C++ identifier, so '#' is written as '_'.

// SKIP B_0059#21: typed text "1,000" needs locale-aware parsing; typedGoldWhole takes a double, and giveGold path depends on the current culture.

TEST(engine_B_0063_71_share_of_10_among_4) {
    // 10 gold = 100000 Currency; 2.5 gold rounds half-even to 2 gold = 20000.
    CHECK_EQ(splitSharePaid(100000, 4), 20000);
}

TEST(engine_B_0063_72_share_of_14_among_4) {
    // 14 gold = 140000 Currency; 3.5 gold rounds half-even to 4 gold = 40000.
    CHECK_EQ(splitSharePaid(140000, 4), 40000);
}

TEST(engine_B_0063_73_total_handed_out_10_among_4) {
    // Each of 4 members receives the rounded share of 2 gold; total 8 gold = 80000.
    Currency share = splitSharePaid(100000, 4);
    CHECK_EQ(4 * share, 80000);
}

// SKIP B_0063#74: announcement is written to env.log; the EventLog read API is not in the headers, so "nothing printed" cannot be checked.

// SKIP B_0063#75: announcement text "Your share is 35 Gold!" is written to env.log; the EventLog read API is not in the headers, so the text cannot be checked.

// SKIP B_0065#13: needs a seeded simulation of 400 boxer fights (monster data, spoils, chest creation); no equivalent single function.

// SKIP B_0065#14: needs a seeded simulation of 400 boxer fights; no equivalent single function.

// SKIP B_0065#15: needs AwardSpoils, which has no equivalent in the headers.

// SKIP B_0065#16: needs PayGold and chest opening over a simulation; no equivalent in the headers.
