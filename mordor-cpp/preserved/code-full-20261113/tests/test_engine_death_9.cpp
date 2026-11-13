#include "test_support.h"
#include "mordor/death1a.h"

// SKIP B_0066#139: killing by a brute is a monster combat simulation; no exposed function performs that kill path
// SKIP B_0068#3: monster attack resolution against a poisoned, diseased body needs a combat simulation
// SKIP B_0068#4: monster group attack resolution on a picked-up body needs a combat simulation
// SKIP B_0068#5: morgue raise outcome depends on the reference's seeded RNG stream, which cannot be reproduced here
// SKIP B_0068#6: join-based morgue raise outcome depends on the reference's seeded RNG stream, which cannot be reproduced here
// SKIP B_0068#7: no exposed function computes days-until-found from the RNG; rescuersReturn returns only a bool

namespace {

mordor::i32 resistSum(const mordor::Character& c) {
    mordor::i32 sum = 0;
    for (mordor::u32 i = 0; i < static_cast<mordor::u32>(mordor::kResistKinds); ++i) {
        sum += c.resistances.resist[i];
    }
    return sum;
}

mordor::Character makeAilingStonedCarried() {
    mordor::Character ch;
    ch.deathState = mordor::DeathState::Stoned;
    ch.ailments.poisonLevel = 7;
    ch.ailments.diseaseLevel = 4;
    ch.ailments.paralysisLevel = 3;
    ch.ailments.paralysed = true;
    ch.resistances.resist[0] = 30;
    ch.carryingCharacterName = "Somebody";
    return ch;
}

}  // namespace

// B_0068#1: death cleanup clears poison, disease, paralysis, resistances and carrier.
TEST(engine_B_0068_1_cleanup_clears_ailments_and_carrier) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch = makeAilingStonedCarried();

    mordor::applyPruneDeath(env.ctx, ch);

    CHECK(ch.ailments.poisonLevel == 0);
    CHECK_EQ(ch.ailments.poisonLevel, 0);
    CHECK(ch.ailments.diseaseLevel == 0);
    CHECK_EQ(ch.ailments.diseaseLevel, 0);
    CHECK(!ch.ailments.paralysed);
    CHECK_EQ(resistSum(ch), 0);
    CHECK(ch.carryingCharacterName.empty());
}

// B_0068#2: death cleanup leaves the stoned state set.
TEST(engine_B_0068_2_cleanup_keeps_stoned) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch = makeAilingStonedCarried();

    mordor::applyPruneDeath(env.ctx, ch);

    CHECK(ch.deathState == mordor::DeathState::Stoned);
}

// B_0068#8: rescue carries the dead character home to town and ages them by the days waited.
TEST(engine_B_0068_8_rescue_carries_home_and_ages) {
    tsup::Env env(42);
    mordor::Character ch;
    ch.deathState = mordor::DeathState::Dead;
    ch.location.dungeonLevel = 3;
    ch.location.area = 12;
    ch.location.x = 5;
    ch.location.y = 7;
    ch.ageDays = 1000;

    const mordor::i32 days = 3;
    mordor::applyRescueFound(env.ctx, ch, days);

    CHECK_EQ(ch.location.dungeonLevel, 0);          // dungeon level 0
    CHECK_EQ(ch.location.area, -1);                 // area -1
    CHECK_EQ(ch.ageDays - 1000, days);              // age rose by exactly those days
    CHECK(days > 0);                                // days more than zero
    CHECK(ch.isDead());                             // still dead
}

// B_0068#9: a dead character below ground gets the Wait for Rescue button.
TEST(engine_B_0068_9_wait_for_rescue_offered_below_ground) {
    mordor::Party party;
    party.size = 1;
    party.dungeonLevel = 1;
    party.members[0].deathState = mordor::DeathState::Dead;
    party.members[0].location.dungeonLevel = 1;

    CHECK(mordor::waitButtonEnabled(party));
}
