#include "test_support.h"
#include "mordor/spells1b.h"

using namespace mordor;

// Enum-to-int helper so CHECK_EQ never needs operator<< on scoped enums.
template <typename E>
static i32 ev(E e) {
    return static_cast<i32>(e);
}

// R60: OpensLockUpTo(effect) = effect - 29
TEST(opens_lock_up_to_maps_effects_30_to_32) {
    CHECK_EQ(kLockOpenBaseEffect, 29);
    CHECK_EQ(opensLockUpTo(30), 1);
    CHECK_EQ(opensLockUpTo(31), 2);
    CHECK_EQ(opensLockUpTo(32), 3);
}

// Module constants stated in the SRD and header.
TEST(module_constants_match_srd) {
    CHECK_EQ(kSanctuaryDays, 365);
    CHECK_EQ(kLocateLevelRange, 15);
    CHECK_EQ(kCityArea, 0);
    CHECK_EQ(kPortalPartyWipeSlotLimit, 2);
    CHECK_EQ(kFateUnusedDraw, 3);
}

// R64 / Table A: utility effect ids 11, 12, 13.
TEST(utility_effect_ids_match_table_a) {
    CHECK_EQ(ev(UtilityEffect::DetectRock), 11);
    CHECK_EQ(ev(UtilityEffect::DepthPerception), 12);
    CHECK_EQ(ev(UtilityEffect::FindDirection), 13);
}

// R63: zero stored point means never cast. Only x and y are checked.
TEST(default_sanctuary_point_is_never) {
    SanctuaryPoint none;
    CHECK(none.isNever());

    SanctuaryPoint levelOnly;
    levelOnly.dungeonLevel = 3;
    CHECK(levelOnly.isNever());

    SanctuaryPoint xSet;
    xSet.x = 1;
    CHECK(!xSet.isNever());
}

