#include "test_support.h"
#include "mordor/clock.h"
#include "mordor/state_character.h"

// Minimised window is modelled by the `paused` argument of playClockTick.
// Assumes Vec<T> supports push_back and operator[].

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
