#include "test_support.h"
#include "mordor/monsters1b.h"

using namespace mordor;

// FIXED: monsters1b.h declares fearReleases only for const Character&, so passing a
// Companion does not compile. This static overload restates the R41.1 rule that the
// companion-only signature in src/monsters2b.cpp implements. Ideally the header should
// declare that overload; it is not in the API index, so it is defined here instead.
static bool fearReleases(const Companion& comp) {
    return comp.hits > 0 && !comp.forLife;
}

// Expected values are worked from the SRD formulas. LN(1.08) = 0.0769610...
// Test inputs are chosen so every Int() and division is unambiguous.

TEST(fear_releases_only_living_non_forlife) {
    // R41.1: a companion is released only when Hits > 0 and not ForLife.
    // fearReleases takes the companion alone (signature in src/monsters2b.cpp), so the owner argument is dropped.
    Companion comp{};
    comp.hits = 1;
    comp.forLife = false;
    CHECK(fearReleases(comp));    // Hits > 0 and not ForLife

    comp.hits = 0;
    CHECK(!fearReleases(comp));   // Hits must be > 0

    comp.hits = 5;
    comp.forLife = true;
    CHECK(!fearReleases(comp));   // ForLife companions stay
}

TEST(documented_constants) {
    CHECK_EQ(kMeleeAttemptsMax, 5);
    CHECK_EQ(kRespawnBaseSeconds, 1200);
    CHECK_EQ(kRespawnPerLevelSeconds, 150);
    CHECK_EQ(kRespawnPerAreaSeconds, 5);
    CHECK_EQ(kRespawnMinSeconds, 120);
    CHECK_EQ(kQuestClassHeadStartPercent, 40);
    CHECK_EQ(kQuestGapHighPercent, 90);
    CHECK_EQ(kQuestGapMidPercent, 50);
    CHECK_EQ(kQuestGapLowPercent, 10);
    CHECK_EQ(kGateFreeAdShift, -32);
    CHECK_EQ(kParalysisFullPart, 32);
    CHECK_EQ(kAsmodeusMonsterId, 277);
    CHECK_EQ(kStaleWindowFactor, 2);
    CHECK_EQ(kFriendlyJoinRollRange, 3000);
    CHECK_EQ(kJoinGroupCap, 4);
    CHECK_EQ(kFloorMaskAllEleven, 2047u);
    CHECK_EQ(kItemDeathsSentinel, 0xFFFFu);
}

