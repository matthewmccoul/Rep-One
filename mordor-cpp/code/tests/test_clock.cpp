#include "test_support.h"
#include "mordor/clock.h"

using namespace mordor;

namespace {

i32 ik(CombatOrderKind k) {
    return static_cast<i32>(k);
}

}  // namespace

// ---------- Hostile base table (R19, Tables) ----------

// ---------- Hostile roll (R19): T = trunc(base - 7.5*ln(max(1,CHA))) ----------
// Draws are 1..100; aggression if draw < T. Only extreme RNG values are used
// so the expected result does not depend on the exact draw mapping.

// ---------- Free-attack threshold (R22/R26) ----------
// max(5, 50 + 5*(monsterDex - charDex)); cap at 98 only when cap is set.

TEST(free_attack_threshold_equal_dex_is_fifty) {
    CHECK_EQ(freeAttackThreshold(12, 12, false), 50);
    CHECK_EQ(freeAttackThreshold(12, 12, true), 50);
}

TEST(free_attack_threshold_floor_of_five) {
    // 50 + 5*(0-20) = -50, floored to 5 regardless of cap
    CHECK_EQ(freeAttackThreshold(0, 20, false), 5);
    CHECK_EQ(freeAttackThreshold(0, 20, true), 5);
}

TEST(free_attack_threshold_floor_boundary) {
    // 50 + 5*(0-9) = 5 exactly
    CHECK_EQ(freeAttackThreshold(0, 9, false), 5);
    // 50 + 5*(0-8) = 10
    CHECK_EQ(freeAttackThreshold(0, 8, false), 10);
    // 50 + 5*(0-10) = 0, floored to 5
    CHECK_EQ(freeAttackThreshold(0, 10, false), 5);
}

TEST(free_attack_threshold_small_gap) {
    // 50 + 5*(1-2) = 45
    CHECK_EQ(freeAttackThreshold(1, 2, false), 45);
    // 50 + 5*(9-0) = 95, under the cap
    CHECK_EQ(freeAttackThreshold(9, 0, true), 95);
}

TEST(free_attack_threshold_uncapped_can_exceed_98) {
    // 50 + 5*30 = 200 with no cap (Moved, argument 1)
    CHECK_EQ(freeAttackThreshold(30, 0, false), 200);
    // 50 + 5*10 = 100 with no cap
    CHECK_EQ(freeAttackThreshold(10, 0, false), 100);
}

TEST(free_attack_threshold_cap_at_98_on_level_change) {
    // 200 capped to 98 (ChangedLevel, argument 2)
    CHECK_EQ(freeAttackThreshold(30, 0, true), 98);
    // 100 capped to 98
    CHECK_EQ(freeAttackThreshold(10, 0, true), 98);
}

// ---------- Play clock (R7, R28-R30, P8) ----------

// ---------- Clock setup (P1) ----------

TEST(clock_setup_intervals) {
    tsup::Env env({0.5, 0.9});
    GameState game;
    clockSetup(env.ctx, game);
    CHECK_EQ(game.combatIntervalMs, 1000);
    CHECK_EQ(game.ailmentIntervalMs, 4000);
    CHECK_EQ(game.playIntervalMs, 250);
}

// ---------- Standing orders ----------

TEST(order_from_setting_fight_and_defend) {
    tsup::Env env({0.5, 0.9});
    Arr<i32, kSpellBuffers> buffers{};
    OrderChoice fight = orderFromSetting(env.ctx, 1, buffers);
    CHECK_EQ(ik(fight.kind), ik(CombatOrderKind::Fight));
    CHECK(!fight.useItem);
    OrderChoice defend = orderFromSetting(env.ctx, 2, buffers);
    CHECK_EQ(ik(defend.kind), ik(CombatOrderKind::Defend));
    CHECK(!defend.useItem);
}

TEST(order_from_setting_non_positive_is_none) {
    tsup::Env env({0.5, 0.9});
    Arr<i32, kSpellBuffers> buffers{};
    OrderChoice zero = orderFromSetting(env.ctx, 0, buffers);
    CHECK_EQ(ik(zero.kind), ik(CombatOrderKind::None));
    OrderChoice neg = orderFromSetting(env.ctx, -1, buffers);
    CHECK_EQ(ik(neg.kind), ik(CombatOrderKind::None));
}

TEST(end_fight_clears_standing_orders) {
    tsup::Env env({0.5, 0.9});
    GameState game;
    game.party.size = 2;
    game.party.members[0].order.kind = CombatOrderKind::Fight;
    game.party.members[1].order.kind = CombatOrderKind::Defend;
    endFight(env.ctx, game);
    CHECK_EQ(ik(game.party.members[0].order.kind), ik(CombatOrderKind::None));
    CHECK_EQ(ik(game.party.members[1].order.kind), ik(CombatOrderKind::None));
}
