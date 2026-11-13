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

TEST(combat2a_trap0_scope_is_none) {
    CHECK_EQ(static_cast<int>(mordor::chestTrapScope(mordor::ChestTrap::NotTrapped)),
             static_cast<int>(mordor::TrapScope::None));
}

TEST(combat2a_traps_1_to_4_scope_whole_party) {
    for (int t = 1; t <= 4; ++t) {
        auto scope = mordor::chestTrapScope(static_cast<mordor::ChestTrap>(t));
        CHECK_EQ(static_cast<int>(scope), static_cast<int>(mordor::TrapScope::WholeParty));
    }
    CHECK_EQ(static_cast<int>(mordor::chestTrapScope(mordor::ChestTrap::PoisonGas)),
             static_cast<int>(mordor::TrapScope::WholeParty));
    CHECK_EQ(static_cast<int>(mordor::chestTrapScope(mordor::ChestTrap::Fire)),
             static_cast<int>(mordor::TrapScope::WholeParty));
}

TEST(combat2a_traps_5_to_10_scope_opener_only) {
    for (int t = 5; t <= 10; ++t) {
        auto scope = mordor::chestTrapScope(static_cast<mordor::ChestTrap>(t));
        CHECK_EQ(static_cast<int>(scope), static_cast<int>(mordor::TrapScope::OpenerOnly));
    }
    CHECK_EQ(static_cast<int>(mordor::chestTrapScope(mordor::ChestTrap::ItemDestruction)),
             static_cast<int>(mordor::TrapScope::OpenerOnly));
}

TEST(combat2a_scope_boundary_between_4_and_5) {
    CHECK_EQ(static_cast<int>(mordor::chestTrapScope(static_cast<mordor::ChestTrap>(4))),
             static_cast<int>(mordor::TrapScope::WholeParty));
    CHECK_EQ(static_cast<int>(mordor::chestTrapScope(static_cast<mordor::ChestTrap>(5))),
             static_cast<int>(mordor::TrapScope::OpenerOnly));
}

// ---- R1: roll stays within trap numbers 0..10 ----

TEST(combat2a_roll_chest_trap_stays_in_range) {
    tsup::Env env(12345);
    for (int i = 0; i < 60; ++i) {
        mordor::ChestTrap t = mordor::rollChestTrap(env.ctx);
        int v = static_cast<int>(t);
        CHECK(v >= 0);
        CHECK(v <= mordor::kChestTrapMax);
    }
}

// ---- R2/R3: no damage from trap 0 ----

TEST(combat2a_trap0_damage_is_zero) {
    tsup::Env env({0.5, 0.9});
    mordor::Character target;
    target.vitals.curHits = 10;
    mordor::f64 dmg = mordor::chestTrapDamage(env.ctx, mordor::ChestTrap::NotTrapped, target);
    CHECK_EQ(dmg, 0.0);
    CHECK_EQ(target.vitals.curHits, 10);
}

// ---- R4: CurHits = round_half_even(CurHits - dmg) ----

TEST(combat2a_hits_half_even_rounds_2_5_up_to_even) {
    // 10 - 2.5 = 7.5 -> 8 (even)
    CHECK_EQ(mordor::hitsAfterChestTrap(10, 2.5), 8);
}

TEST(combat2a_hits_half_even_rounds_3_5_down_to_even) {
    // 10 - 3.5 = 6.5 -> 6 (even)
    CHECK_EQ(mordor::hitsAfterChestTrap(10, 3.5), 6);
    // 11 - 2.5 = 8.5 -> 8 (even)
    CHECK_EQ(mordor::hitsAfterChestTrap(11, 2.5), 8);
}

TEST(combat2a_hits_nearest_rounding_off_half) {
    // 10 - 2.4 = 7.6 -> 8
    CHECK_EQ(mordor::hitsAfterChestTrap(10, 2.4), 8);
    // 10 - 2.6 = 7.4 -> 7
    CHECK_EQ(mordor::hitsAfterChestTrap(10, 2.6), 7);
}

TEST(combat2a_hits_equivalent_to_rounding_damage) {
    // Quirk: because CurHits is an integer, rounding (CurHits - dmg) equals
    // CurHits - round_half_even(dmg).
    // dmg 2.5 -> round to 2, so 10 - 2 = 8.
    CHECK_EQ(mordor::hitsAfterChestTrap(10, 2.5), 10 - 2);
    // dmg 3.5 -> round to 4, so 10 - 4 = 6.
    CHECK_EQ(mordor::hitsAfterChestTrap(10, 3.5), 10 - 4);
}

// ---- R5: CurHits below 1 becomes 0 ----

TEST(combat2a_hits_clamped_to_zero_when_damage_exceeds_hits) {
    CHECK_EQ(mordor::hitsAfterChestTrap(5, 9.0), 0);
    CHECK_EQ(mordor::hitsAfterChestTrap(3, 10.0), 0);
}

TEST(combat2a_hits_unchanged_with_zero_damage) {
    CHECK_EQ(mordor::hitsAfterChestTrap(7, 0.0), 7);
    CHECK_EQ(mordor::hitsAfterChestTrap(1, 0.0), 1);
}

TEST(combat2a_apply_chest_damage_writes_cur_hits) {
    mordor::Character c;
    c.vitals.curHits = 10;
    c.vitals.maxHits = 15;
    mordor::applyChestTrapDamage(c, 3.5);
    CHECK_EQ(c.vitals.curHits, 6);
    CHECK_EQ(c.vitals.maxHits, 15);
}

TEST(combat2a_apply_chest_damage_clamps_at_zero) {
    mordor::Character c;
    c.vitals.curHits = 4;
    mordor::applyChestTrapDamage(c, 20.0);
    CHECK_EQ(c.vitals.curHits, 0);
}

// ---- Procedure P1 scope on party ----

TEST(combat2a_trap_no_effect_leaves_party_unchanged) {
    tsup::Env env({0.5, 0.9});
    mordor::Party party;
    party.size = 2;
    party.opener = 0;
    party.members[0].vitals.curHits = 10;
    party.members[1].vitals.curHits = 12;
    mordor::resolveChestTrap(env.ctx, party, mordor::ChestTrap::NotTrapped);
    CHECK_EQ(party.members[0].vitals.curHits, 10);
    CHECK_EQ(party.members[1].vitals.curHits, 12);
}

TEST(combat2a_non_damage_trap_leaves_party_unchanged) {
    // R3: only traps 3 and 4 deal damage, so trap 5 (opener-only) does not.
    tsup::Env env({0.5, 0.9});
    mordor::Party party;
    party.size = 2;
    party.opener = 0;
    party.members[0].vitals.curHits = 10;
    party.members[1].vitals.curHits = 12;
    mordor::resolveChestTrap(env.ctx, party, mordor::ChestTrap::ItemDestruction);
    CHECK_EQ(party.members[0].vitals.curHits, 10);
    CHECK_EQ(party.members[1].vitals.curHits, 12);
}

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

TEST(combat2a_severed_only_when_both_procs) {
    mordor::BlowProcs none;
    CHECK(!mordor::blowIsSevered(none));

    mordor::BlowProcs crit;
    crit.crit = true;
    CHECK(!mordor::blowIsSevered(crit));

    mordor::BlowProcs back;
    back.backstab = true;
    CHECK(!mordor::blowIsSevered(back));

    mordor::BlowProcs both;
    both.crit = true;
    both.backstab = true;
    CHECK(mordor::blowIsSevered(both));
}

TEST(combat2a_severed_blow_end_to_end) {
    // Both procs succeed on the same blow: the modifier is +5 and +2 (total +7),
    // and the blow counts as Severed.
    tsup::Env env({0.1, 0.2});
    mordor::BlowProcs p = mordor::rollBlowProcs(env.ctx, 50.0, 50.0);
    CHECK(mordor::blowIsSevered(p));
    CHECK_EQ(mordor::blowModifierBonus(p), 7);
}
