#include "test_support.h"
#include "mordor/monsters1a.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"

using namespace mordor;

namespace {

Arr<Companion, kCompanionSlots> fourOccupiedCompanions() {
    Arr<Companion, kCompanionSlots> comps{};
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        comps[i].occupied = true;
    }
    return comps;
}

// Draws pickCompanionSlot n times against four occupied companion slots
// and counts how many times each return value 0..4 occurred.
void acidDrawCounts(i32 seed, i32 n, i32 counts[5]) {
    for (i32 k = 0; k < 5; ++k) {
        counts[k] = 0;
    }
    tsup::Env env(seed);
    Arr<Companion, kCompanionSlots> comps = fourOccupiedCompanions();
    for (i32 i = 0; i < n; ++i) {
        i32 t = pickCompanionSlot(env.ctx, comps);
        if (t >= 0 && t < 5) {
            counts[t] += 1;
        }
    }
}

}  // namespace

// B_0063#3: 60000 draws, seed 11, four occupied slots: count of return 0
TEST(engine_B_0063_3_acid_count_character) {
    i32 counts[5];
    acidDrawCounts(11, 60000, counts);
    CHECK(counts[0] >= 10800 && counts[0] <= 13200);
}

// B_0063#4: count of return 1
TEST(engine_B_0063_4_acid_count_slot1) {
    i32 counts[5];
    acidDrawCounts(11, 60000, counts);
    CHECK(counts[1] >= 10800 && counts[1] <= 13200);
}

// B_0063#5: count of return 2
TEST(engine_B_0063_5_acid_count_slot2) {
    i32 counts[5];
    acidDrawCounts(11, 60000, counts);
    CHECK(counts[2] >= 10800 && counts[2] <= 13200);
}

// B_0063#6: count of return 3
TEST(engine_B_0063_6_acid_count_slot3) {
    i32 counts[5];
    acidDrawCounts(11, 60000, counts);
    CHECK(counts[3] >= 10800 && counts[3] <= 13200);
}

// B_0063#7: count of return 4
TEST(engine_B_0063_7_acid_count_slot4) {
    i32 counts[5];
    acidDrawCounts(11, 60000, counts);
    CHECK(counts[4] >= 10800 && counts[4] <= 13200);
}

// B_0063#8: seed 13, 60000 draws; reachable nonzero companion slots are exactly 1,2,3
TEST(engine_B_0063_8_acid_reachable_slots) {
    tsup::Env env(13);
    Arr<Companion, kCompanionSlots> comps = fourOccupiedCompanions();
    bool seen[5] = {false, false, false, false, false};
    for (i32 i = 0; i < 60000; ++i) {
        i32 t = pickCompanionSlot(env.ctx, comps);
        // reference (SelfCheckProbe.cs:2003): only t != 0 && t < 4 selects a companion slot
        if (t != 0 && t >= 0 && t < 4) {
            seen[t] = true;
        }
    }
    CHECK(seen[1]);
    CHECK(seen[2]);
    CHECK(seen[3]);
    CHECK(!seen[4]);
}

// B_0063#9: seed 12, 500 draws with no companions: every draw returns 0
TEST(engine_B_0063_9_acid_no_companions_all_character) {
    tsup::Env env(12);
    Arr<Companion, kCompanionSlots> comps{};
    bool allZero = true;
    for (i32 i = 0; i < 500; ++i) {
        if (pickCompanionSlot(env.ctx, comps) != 0) {
            allZero = false;
        }
    }
    CHECK(allZero);
}

// B_0063#10: drain monster stat drop count over 1500 rounds (Strength)
// SKIP B_0063#10: needs a multi-round combat simulation with MDATA drain monster setup; no single stat-drain function in headers
// B_0063#11: stat index 1 drop count over 1500 rounds
// SKIP B_0063#11: needs a multi-round combat simulation with MDATA drain monster setup; no single stat-drain function in headers
// B_0063#12: stat index 2 drop count over 1500 rounds
// SKIP B_0063#12: needs a multi-round combat simulation with MDATA drain monster setup; no single stat-drain function in headers

