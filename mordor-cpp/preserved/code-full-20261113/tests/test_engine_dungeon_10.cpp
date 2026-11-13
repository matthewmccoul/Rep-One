#include "test_support.h"
#include "mordor/dungeon2a.h"

// Case ids contain '#', which is not valid in identifiers, so '#' is written as '_'.
// SKIP A_01_2: needs the shipped MDATA5.MDR/MDATA11.MDR files and EncounterLoopsProbe, which are not in the headers.
// SKIP A_02_8: needs a chute record on level 3 in the shipped data and a full party/floor setup for takeChute; not isolable with the pure helpers.
// SKIP A_02_9: needs a chute record on level 2 and level 3 in the shipped data and a full party/floor setup for takeChute; not isolable with the pure helpers.

TEST(engine_A_02_1_full_level_cap) {
    CHECK_EQ(mordor::kFullBuildLevelLimit, 15);
}

TEST(engine_A_02_2_limited_last_level) {
    CHECK_EQ(mordor::kLimitedBuildLevelLimit, 3);
}

TEST(engine_A_02_3_limited_clamp_level4) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Limited, 4), 3);
    CHECK(mordor::isDestinationDenied(4, mordor::ReleaseKind::Limited));
    // PARTIAL A_02_3 message: 'LIMITED VERSION: Request to go to Level 4 is Denied.' is not exposed
    // as text by the headers (reportLimitedMessage returns a LimitedMessageKind only), so it is not asserted.
}

TEST(engine_A_02_4_limited_clamp_level15) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Limited, 15), 3);
}

TEST(engine_A_02_5_limited_levels_1_and_3_not_denied) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Limited, 1), 1);
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Limited, 3), 3);
    CHECK(!mordor::isDestinationDenied(1, mordor::ReleaseKind::Limited));
    CHECK(!mordor::isDestinationDenied(3, mordor::ReleaseKind::Limited));
    // PARTIAL A_02_5 message: null message is not observable through the headers, so it is not asserted.
}

TEST(engine_A_02_6_limited_levels_16_and_minus1_unclamped) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Limited, 16), 16);
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Limited, -1), -1);
    // PARTIAL A_02_6 message: null message is not observable through the headers, so it is not asserted.
}

TEST(engine_A_02_7_full_levels_4_and_15_not_denied) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Full, 4), 4);
    CHECK_EQ(mordor::clampLevelRequest(env.ctx, mordor::ReleaseKind::Full, 15), 15);
    CHECK(!mordor::isDestinationDenied(4, mordor::ReleaseKind::Full));
    CHECK(!mordor::isDestinationDenied(15, mordor::ReleaseKind::Full));
    // PARTIAL A_02_7 message: null message is not observable through the headers, so it is not asserted.
}
