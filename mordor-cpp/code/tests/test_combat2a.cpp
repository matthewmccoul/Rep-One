#include "test_support.h"
#include "mordor/combat2a.h"

// Derived from the SRD combat chapter (c07p2), sections R1-R10.
// Not tested, because the excerpt does not state them: the rollChestTrap
// selection method (R1), the chest trap damage magnitudes for traps 1-4
// (only "traps 3 and 4 deal damage" is given), the critChance and
// backstabChance formulas and their clamps, and the printed text of trap arms.

// ---- constants ----

TEST(combat2a_constants_match_srd) {
    CHECK_EQ(mordor::kChestTrapMax, 10);
    CHECK_EQ(mordor::kChestTrapWholePartyMax, 4);
    CHECK_EQ(mordor::kCritModBonus, 5);
    CHECK_EQ(mordor::kBackstabModBonus, 2);
    CHECK_EQ(mordor::kCritChanceScale, 100);
    CHECK_EQ(static_cast<int>(mordor::ChestTrap::Trap10), mordor::kChestTrapMax);
    CHECK_EQ(static_cast<int>(mordor::ChestTrap::Fire), 4);
}

// ---- R2: trap scope ----

// ---- R1: roll stays within trap numbers 0..10 ----

// ---- R2/R3: no damage from trap 0 ----

// ---- R4: CurHits = round_half_even(CurHits - dmg) ----

// ---- R5: CurHits below 1 becomes 0 ----

// ---- Procedure P1 scope on party ----

// ---- R6-R7: proc roll is rng value (0..99) strictly less than chance ----

TEST(combat2a_roll_proc_strictly_less_than_chance) {
    // Draw 0.5 -> 50 out of 0..99.
    tsup::Env env({0.5, 0.5});
    CHECK(!mordor::rollProc(env.ctx, 50.0));  // 50 < 50 is false
    CHECK(mordor::rollProc(env.ctx, 50.5));   // 50 < 50.5 is true
}

TEST(combat2a_roll_proc_chance_extremes) {
    // Draw 0.9 -> 90.
    tsup::Env env({0.9, 0.9});
    CHECK(mordor::rollProc(env.ctx, 100.0));  // 90 < 100
    CHECK(!mordor::rollProc(env.ctx, 90.0));  // 90 < 90 is false
    CHECK(mordor::rollProc(env.ctx, 90.5));   // 90 < 90.5 is true
    CHECK(!mordor::rollProc(env.ctx, 0.0));   // nothing is below 0
}

// ---- R9: both procs rolled, crit first then backstab (R7 before R8) ----

TEST(combat2a_blow_procs_roll_crit_then_backstab) {
    // First draw 0.5 -> 50 used for crit, second draw 0.9 -> 90 for backstab.
    tsup::Env env({0.5, 0.9});
    mordor::BlowProcs p = mordor::rollBlowProcs(env.ctx, 60.0, 80.0);
    CHECK(p.crit);       // 50 < 60
    CHECK(!p.backstab);  // 90 < 80 is false
}

TEST(combat2a_blow_procs_both_succeed) {
    tsup::Env env({0.1, 0.2});  // draws 10 and 20
    mordor::BlowProcs p = mordor::rollBlowProcs(env.ctx, 50.0, 50.0);
    CHECK(p.crit);
    CHECK(p.backstab);
}

TEST(combat2a_blow_procs_none_succeed_at_zero_chance) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowProcs p = mordor::rollBlowProcs(env.ctx, 0.0, 0.0);
    CHECK(!p.crit);
    CHECK(!p.backstab);
}

// ---- R10: damage modifier bonuses and Severed ----

TEST(combat2a_modifier_bonus_values) {
    mordor::BlowProcs none;
    CHECK_EQ(mordor::blowModifierBonus(none), 0);

    mordor::BlowProcs crit;
    crit.crit = true;
    CHECK_EQ(mordor::blowModifierBonus(crit), 5);

    mordor::BlowProcs back;
    back.backstab = true;
    CHECK_EQ(mordor::blowModifierBonus(back), 2);

    mordor::BlowProcs both;
    both.crit = true;
    both.backstab = true;
    CHECK_EQ(mordor::blowModifierBonus(both), 5 + 2);
}

