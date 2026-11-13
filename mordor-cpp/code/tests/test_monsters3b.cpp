#include "test_support.h"
#include "mordor/monsters3b.h"

using namespace mordor;

// Not tested: groupSizeCInt (CInt is not defined in the SRD part) and
// newbieGuardExcludes (the meaning of "newbie" is not given in the SRD part).

TEST(constants_match_srd) {
    CHECK_EQ(kAttackAttemptCap, 5);
    CHECK_EQ(kSwingBoundFactor, 0.5);
    CHECK_EQ(kSwingBoundDivisor, 2.0);
    CHECK_EQ(kSwingBoundBonus, 1);
    CHECK_EQ(kMemberBaseHitsFloor, 1);
    CHECK_EQ(kRespawnSecondsPerArea, static_cast<i64>(5));
    CHECK_EQ(kJoinChancePerCha, 10);
    CHECK_EQ(kJoinChancePerFloor, 10);
    CHECK_EQ(kJoinChanceRollMax, 3000);
    CHECK_EQ(kLairPlacePercent, 75);
    CHECK_EQ(kNewbieMonsterGuildCap, 2);
}

