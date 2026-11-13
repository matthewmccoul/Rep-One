#include <cmath>

#include "test_support.h"
#include "mordor/death3.h"

using namespace mordor;

namespace {

// Builds a body whose guild level and character level agree, so the
// morgue level check gives the same answer whichever field it reads.
Character makeBody(DeathState state, i32 level, i32 maxHits) {
    Character ch;
    ch.deathState = state;
    ch.level = level;
    ch.guilds[static_cast<u32>(Guild::Nomad)].level = level;
    ch.vitals.maxHits = maxHits;
    return ch;
}

}  // namespace

TEST(death3_constants_match_srd) {
    CHECK_EQ(kMorgueOwnBodyBonus, 40);
    CHECK_EQ(kMorgueCounterBonus, 10);
    CHECK_EQ(kRockStoredRaiseChance, 10);
    CHECK_EQ(kAgeQuietFailure, 54);
    CHECK_EQ(kAgeStoned, 84);
    CHECK_EQ(kAgeRock, 124);
    CHECK_EQ(kMorgueMinLevel, 1);
    CHECK_EQ(kMorgueMaxLevel, 15);
    CHECK_EQ(kGuildPaysMaxLevel, 5);
    CHECK_EQ(kOldAgePct, 98);
}

TEST(death3_raise_age_cost_by_cause) {
    CHECK_EQ(raiseAgeCost(DeathState::Dead), 54);
    CHECK_EQ(raiseAgeCost(DeathState::Stoned), 84);
    CHECK_EQ(raiseAgeCost(DeathState::Rock), 124);
}

