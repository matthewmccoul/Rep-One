#include "test_support.h"
#include "mordor/status2.h"

using namespace mordor;
using tsup::Env;

namespace {

// Arr<> indices are unsigned in this codebase (see Character::activeLevel).
u32 ix(i32 v) {
    return static_cast<u32>(v);
}

i32 mag(i32 v) {
    return v < 0 ? -v : v;
}

}  // namespace

TEST(status2_constants_match_srd) {
    CHECK_EQ(kAilmentTickMs, 4000);
    CHECK_EQ(kPoisonCureRollAbove, 185);
    CHECK_EQ(kDiseaseCureRollAbove, 195);
    CHECK_EQ(kCureRollSides, 200);
    CHECK_EQ(kParalysisChanceCap, 95);
    CHECK_EQ(kStoneChanceCap, 95);
    CHECK_EQ(kTickSaveMin, 5);
    CHECK_EQ(kTickSaveMax, 90);
    CHECK_EQ(kMaxAgingDays, 1825);
    CHECK_EQ(kAgingDaysPerYear, 365);
    CHECK_EQ(kPoisonHitsPerFail, 2);
    CHECK_EQ(kDiseaseHitsPerFail, 1);
    CHECK_EQ(kBreathHighPercent, 80);
    CHECK_EQ(kSpitLowPercent, 10);
    CHECK(kBandHighRatio == 1.25);
    CHECK(kBandLowRatio == 0.75);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Poison), 0);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Disease), 1);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Paralysis), 2);
    CHECK_EQ(static_cast<i32>(AilmentSlot::Protected), 6);
}

