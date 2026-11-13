#include "test_support.h"
#include "mordor/spells1b.h"
#include "mordor/state_party.h"

namespace {

struct FateStats {
    mordor::i32 hitsMin = 1000;
    mordor::i32 hitsMax = -1000;
    double hitsSum = 0.0;
    mordor::i32 spMin = 1000;
    mordor::i32 spMax = -1000;
    double spSum = 0.0;
    mordor::i32 draws = 0;
};

// Fate applied to a fresh character with 40 max hits and 40 max spell points,
// 8000 draws from Random seeded 20260922.
FateStats runFateDraws(mordor::i32 count) {
    tsup::Env env(20260922);
    mordor::Character who;
    who.vitals.maxHits = 40;
    who.vitals.maxSpellPoints = 40;
    FateStats s;
    for (mordor::i32 i = 0; i < count; ++i) {
        mordor::rerollFateVitals(env.ctx, who);
        mordor::i32 h = who.vitals.curHits;
        mordor::i32 p = who.vitals.curSpellPoints;
        if (h < s.hitsMin) s.hitsMin = h;
        if (h > s.hitsMax) s.hitsMax = h;
        s.hitsSum += static_cast<double>(h);
        if (p < s.spMin) s.spMin = p;
        if (p > s.spMax) s.spMax = p;
        s.spSum += static_cast<double>(p);
        ++s.draws;
    }
    return s;
}

}  // namespace

// B_0063#18-#21: SKIP. applyCastState takes a numeric effect id, and the
// mapping from the MDATA2.MDR effect names (Levitate, Invisible, SeeInvisible,
// Protect) to those ids is not given in the headers, so the inputs cannot be
// set up faithfully.
// SKIP B_0063#18: effect-name to effect-id mapping not in headers; applyCastState is id-based
// SKIP B_0063#19: effect-name to effect-id mapping not in headers; applyCastState is id-based
// SKIP B_0063#20: effect-name to effect-id mapping not in headers; applyCastState is id-based
// SKIP B_0063#21: effect-name to effect-id mapping not in headers; applyCastState is id-based

TEST(engine_B_0063_26_fate_lowest_hits) {
    FateStats s = runFateDraws(8000);
    CHECK_EQ(s.draws, 8000);
    CHECK_EQ(s.hitsMin, 1);
}

TEST(engine_B_0063_27_fate_highest_hits) {
    FateStats s = runFateDraws(8000);
    CHECK_EQ(s.draws, 8000);
    CHECK_EQ(s.hitsMax, 40);
}

TEST(engine_B_0063_28_fate_mean_hits) {
    FateStats s = runFateDraws(8000);
    double mean = s.hitsSum / static_cast<double>(s.draws);
    CHECK(mean >= 20.0 && mean <= 21.0);
}

TEST(engine_B_0063_29_fate_lowest_spell_points) {
    FateStats s = runFateDraws(8000);
    CHECK_EQ(s.draws, 8000);
    CHECK_EQ(s.spMin, 1);
}

TEST(engine_B_0063_30_fate_highest_spell_points) {
    FateStats s = runFateDraws(8000);
    CHECK_EQ(s.draws, 8000);
    CHECK_EQ(s.spMax, 40);
}

TEST(engine_B_0063_31_fate_mean_spell_points) {
    FateStats s = runFateDraws(8000);
    double mean = s.spSum / static_cast<double>(s.draws);
    CHECK(mean >= 20.0 && mean <= 21.0);
}
