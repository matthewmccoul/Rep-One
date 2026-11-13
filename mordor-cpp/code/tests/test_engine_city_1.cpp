#include "test_support.h"
#include "mordor/city1.h"

// Confinement cases: pen counts are passed as {Good, Neutral, Evil, unused}.
// Result indices are 0-based Good/Neutral/Evil, matching the reference's
// AlignmentChoice output.
// Store cases: counts are indexed by ItemAlign (0 Unaligned, 1 Good, 2 Neutral, 3 Evil).

TEST(engine_B_0055_1_buttons_and_counts) {
    auto r = mordor::confinementRowClick({0, 2, 0, 0}, -1);
    CHECK(!r.enabled[0]);
    CHECK(r.enabled[1]);
    CHECK(!r.enabled[2]);
    CHECK_EQ(r.stocked, 1);
    CHECK_EQ(r.last, 1);
}

TEST(engine_B_0055_2_asks_false) {
    auto r = mordor::confinementRowClick({0, 2, 0, 0}, -1);
    CHECK(!r.asks);
}

TEST(engine_B_0055_3_chosen_index) {
    // Offer.Chosen(2): the argument is the alignment chosen before.
    auto r = mordor::confinementRowClick({0, 2, 0, 0}, 2);
    CHECK_EQ(r.chosen, 1);
}

TEST(engine_B_0055_4_buttons_and_counts) {
    auto r = mordor::confinementRowClick({1, 0, 3, 0}, -1);
    CHECK(r.enabled[0]);
    CHECK(!r.enabled[1]);
    CHECK(r.enabled[2]);
    CHECK_EQ(r.stocked, 2);
    CHECK_EQ(r.last, 2);
}

TEST(engine_B_0055_5_asks_and_chosen) {
    auto r = mordor::confinementRowClick({1, 0, 3, 0}, 0);
    CHECK(r.asks);
    CHECK_EQ(r.chosen, -1);
}

TEST(engine_B_0055_6_empty_row) {
    auto r = mordor::confinementRowClick({0, 0, 0, 0}, 2);
    CHECK(!r.enabled[0]);
    CHECK(!r.enabled[1]);
    CHECK(!r.enabled[2]);
    CHECK_EQ(r.stocked, 0);
    CHECK_EQ(r.last, -1);
    CHECK(!r.asks);
    CHECK_EQ(r.chosen, 2);
}

TEST(engine_B_0055_7_store_evil_only) {
    auto r = mordor::alignmentChoice({0, 0, 0, 5}, 0);
    CHECK(!r.asks);
    CHECK_EQ(r.chosen, 3);
}

TEST(engine_B_0055_8_store_mixed) {
    auto r = mordor::alignmentChoice({4, 1, 0, 0}, -1);
    CHECK(r.enabled[0]);
    CHECK(r.enabled[1]);
    CHECK(!r.enabled[2]);
    CHECK(!r.enabled[3]);
    CHECK_EQ(r.stocked, 2);
    CHECK_EQ(r.last, 1);
    CHECK(r.asks);
}

// SKIP B_0055#9: needs MdrLoader.LoadMonsters over MDATA5.MDR to pick a creature; no loader in headers.
// SKIP B_0055#10: needs Confinement.Add to build pen state and PriceToBuy per alignment; the headers only expose PenBook.byRef and penBuyPrice(base, count) with no monster base value available.
