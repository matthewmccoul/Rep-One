#include "test_support.h"
#include "mordor/clock.h"
#include "mordor/state_character.h"

// Minimised window is modelled by the `paused` argument of playClockTick.
// Assumes Vec<T> supports push_back and operator[].

TEST(engine_B_0059_39_play_seconds_minimised) {
    tsup::Env env({0.5, 0.9});
    mordor::Vec<mordor::Character> members;
    mordor::Character c;
    c.playSeconds = 0;
    c.playStart = 100;
    members.push_back(c);

    // Game running: ticks at 100 s and 110 s.
    mordor::playClockTick(env.ctx, members, false, 100);
    mordor::playClockTick(env.ctx, members, false, 110);

    // Window minimised: ticks at 500 s and 900 s.
    mordor::playClockTick(env.ctx, members, true, 500);
    mordor::playClockTick(env.ctx, members, true, 900);

    CHECK_EQ(members[0].playSeconds, 10);
}

TEST(engine_B_0059_40_play_seconds_after_restore) {
    tsup::Env env({0.5, 0.9});
    mordor::Vec<mordor::Character> members;
    mordor::Character c;
    c.playSeconds = 0;
    c.playStart = 100;
    members.push_back(c);

    // Same history as B_0059#39.
    mordor::playClockTick(env.ctx, members, false, 100);
    mordor::playClockTick(env.ctx, members, false, 110);
    mordor::playClockTick(env.ctx, members, true, 500);
    mordor::playClockTick(env.ctx, members, true, 900);

    // Window restored: tick at 905 s counts only from the last tick (900 s).
    mordor::playClockTick(env.ctx, members, false, 905);

    CHECK_EQ(members[0].playSeconds, 15);
}

TEST(engine_B_0061_108_free_attack_moved_dex_gap_plus20) {
    // Moved attack: uncapped. 50 + 5*(30-10) = 150.
    CHECK_EQ(mordor::freeAttackThreshold(30, 10, false), 150);
}

TEST(engine_B_0061_109_free_attack_changed_level_dex_gap_plus20) {
    // ChangedLevel attack: capped at 98.
    CHECK_EQ(mordor::freeAttackThreshold(30, 10, true), 98);
}

TEST(engine_B_0061_110_free_attack_moved_dex_gap_minus20) {
    // Moved attack: max(5, 50 + 5*(10-30)) = max(5, -50) = 5.
    CHECK_EQ(mordor::freeAttackThreshold(10, 30, false), 5);
}

TEST(engine_B_0061_111_free_attack_changed_level_dex_gap_minus20) {
    // ChangedLevel attack: floor of 5 applies, cap does not matter here.
    CHECK_EQ(mordor::freeAttackThreshold(10, 30, true), 5);
}

// SKIP B_0065#19: needs seeded multi-round combat simulation (combatTick/playerRound) to count companion kills over 200 rounds
// SKIP B_0065#20: needs seeded multi-round combat simulation to observe whether m1 acts after m0's companion kills the monster
// SKIP B_0061 AttackChance: CombatEngine.AttackChance is not in the headers; only freeAttackThreshold (same formula) is mapped above
