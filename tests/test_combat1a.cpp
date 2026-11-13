#include <cmath>

#include "test_support.h"
#include "mordor/combat1a.h"

using namespace mordor;

// Tolerance comparison for the floating-point helpers.
static bool nearly(double a, double b) { return std::fabs(a - b) < 1e-6; }

// R-bits: monster property flags are tested by bit number (T5).
TEST(monster_bit_flags_are_tested_by_number) {
    Creature m;
    m.specialPropertyFlags = 1u << 4;
    CHECK(monsterHasBit(m, 4));
    CHECK(!monsterHasBit(m, 5));
    CHECK(!monsterHasBit(m, 0));
}

// R1: arrival needs a hostile first monster and a living member.
TEST(arrival_needs_hostile_living_front) {
    MonsterGroup g;
    g.count = 1;
    g.livingCount = 1;
    g.front.hostile = true;
    g.front.alive = true;
    CHECK(arrivalCombatStarts(g));

    g.front.hostile = false;
    CHECK(!arrivalCombatStarts(g));

    g.front.hostile = true;
    g.front.alive = false;
    g.livingCount = 0;
    CHECK(!arrivalCombatStarts(g));
}

// R4: combat setting applies only with no order. R6: Defend suppresses attack.
TEST(setting_applies_only_without_order_and_defend_suppresses_attack) {
    CombatOrder o;  // kind None
    CHECK(combatSettingApplies(o));
    CHECK(!defendSuppressesAttack(o));

    o.kind = CombatOrderKind::Fight;
    CHECK(!combatSettingApplies(o));
    CHECK(!defendSuppressesAttack(o));

    o.kind = CombatOrderKind::Defend;
    CHECK(!combatSettingApplies(o));
    CHECK(defendSuppressesAttack(o));
}

// R7: swings = actor.Swings + w.Swings.
TEST(swings_per_weapon_is_actor_plus_weapon) {
    CHECK_EQ(swingsPerWeapon(2, 3), 5);
    CHECK_EQ(swingsPerWeapon(1, 0), 1);
    CHECK_EQ(swingsPerWeapon(0, 2), 2);
}

// R10: m <= 0 grants no extra swings.
TEST(multiple_swings_coefficient_at_or_below_zero_grants_nothing) {
    CHECK_EQ(extraSwingsFromMultiple(0.0, 500), 0);
    CHECK_EQ(extraSwingsFromMultiple(-2.0, 500), 0);
}

// R10 ladder with m = 5:
//   step = ln(6) = 1.7918
//   T1 = B(50 * step) = B(89.59) = 90   -> level 91 is the first extra swing
//   T2 = B(2 * 90 * step) = B(322.52) = 323 -> level 324 is the second
TEST(multiple_swings_ladder_thresholds_m5) {
    CHECK_EQ(extraSwingsFromMultiple(5.0, 90), 0);
    CHECK_EQ(extraSwingsFromMultiple(5.0, 91), 1);
    CHECK_EQ(extraSwingsFromMultiple(5.0, 323), 1);
    CHECK_EQ(extraSwingsFromMultiple(5.0, 324), 2);
}

// R11: swings never fall; each guild only raises the figure if it beats it.
TEST(raise_swings_never_lowers) {
    CHECK_EQ(raiseSwingsIfHigher(3, 2), 3);
    CHECK_EQ(raiseSwingsIfHigher(1, 4), 4);
    CHECK_EQ(raiseSwingsIfHigher(2, 2), 2);
}

// T1/T2: maximum extra swings per guild and community milestones.
TEST(extra_swing_tables_match_t1_and_t2) {
    CHECK_EQ(kMaxExtraSwings[0], 0);   // Nomad
    CHECK_EQ(kMaxExtraSwings[1], 2);   // Warrior
    CHECK_EQ(kMaxExtraSwings[3], 4);   // Ninja
    CHECK_EQ(kMaxExtraSwings[4], 2);   // Villain
    CHECK_EQ(kMaxExtraSwings[7], 2);   // Scavenger

    // Twelve milestones in total (2+2+4+2+2) per T1.
    i32 milestones = 0;
    for (i32 g = 0; g < kGuildCount; ++g) {
        for (i32 k = 0; k < kMilestonesPerGuild; ++k) {
            if (kCommunityMilestones[static_cast<u32>(g)][static_cast<u32>(k)] != -1) {
                ++milestones;
            }
        }
    }
    CHECK_EQ(milestones, 12);
    CHECK_EQ(kCommunityMilestones[3][0], 56);
    CHECK_EQ(kCommunityMilestones[3][3], 585);
    CHECK_EQ(kCommunityMilestones[1][1], 324);
}

// R13 / T5: bit 4 needs weapon floor >= (int)(LevelFound / 2).
// LevelFound = 9 gives half = 4.
TEST(needs_good_weapon_floor_is_compared_to_half_level) {
    Creature m;
    m.levelFound = 9;
    m.specialPropertyFlags = 1u << 4;
    CHECK(weaponBlockedByNeedsGoodWeapon(m, 3));
    CHECK(!weaponBlockedByNeedsGoodWeapon(m, 4));

    m.specialPropertyFlags = 0;
    CHECK(!weaponBlockedByNeedsGoodWeapon(m, 0));
}

// R13-R15: gate outcomes for bits 4 and 5.
TEST(weapon_gate_outcomes_for_bits_four_and_five) {
    Creature m;
    m.levelFound = 9;
    CHECK(checkWeaponGate(m, 0) == WeaponGateOutcome::Pass);

    m.specialPropertyFlags = 1u << 4;
    CHECK(checkWeaponGate(m, 3) == WeaponGateOutcome::NeedsGoodWeaponBlocked);
    CHECK(checkWeaponGate(m, 4) == WeaponGateOutcome::Pass);

    m.specialPropertyFlags = 1u << 5;
    CHECK(weaponNullifiedByImmunity(m));
    CHECK(checkWeaponGate(m, 0) == WeaponGateOutcome::ImmuneNoEffect);
}

// R13: blocked hand switches the actor to Defend and clears its item order.
TEST(blocked_hand_sets_defend_and_clears_item_order) {
    tsup::Env env({0.5, 0.9});
    Party p;
    // The party must contain slot 1 for the actor to be valid. Party::size
    // defaults to 0, so without this the function's bounds guard returns early.
    p.size = 2;
    p.combatOrders[1].kind = CombatOrderKind::Fight;
    p.itemOrders[1].item = 5;
    p.itemOrders[1].cast = true;

    applyBlockedHandDefend(env.ctx, p, 1);

    CHECK(p.combatOrders[1].kind == CombatOrderKind::Defend);
    CHECK_EQ(p.itemOrders[1].item, -1);
    CHECK(!p.itemOrders[1].cast);
    CHECK(p.combatOrders[0].kind == CombatOrderKind::None);
}

// R18 / R20: community test points, plus the def==0 and clamp cases.
TEST(hit_threshold_points_from_community_table) {
    CHECK_EQ(hitThresholdPercent(10, 0), 98);
    CHECK_EQ(hitThresholdPercent(0, 0), 50);
    CHECK_EQ(hitThresholdPercent(1, 1000), 2);
    CHECK_EQ(hitThresholdPercent(20, 20), 50);
    CHECK_EQ(hitThresholdPercent(1000, 0), 98);
}

// R19: term = Int(Int((Def - Att) / 2) / 4).
TEST(attack_defence_term_halves_then_quarters) {
    CHECK_EQ(attackDefenceTerm(0, 40), 5);   // 40/2 = 20, 20/4 = 5
    CHECK_EQ(attackDefenceTerm(0, 8), 1);    // 8/2 = 4, 4/4 = 1
    CHECK_EQ(attackDefenceTerm(8, 0), -1);   // -8/2 = -4, -4/4 = -1
    CHECK_EQ(attackDefenceTerm(0, 0), 0);
}

// R18 hit gate: roll = Int(Rnd*100)+1; miss when roll >= threshold.
// Rnd = 0.5 gives roll 51; Rnd = 0.0 gives roll 1; Rnd = 0.975 gives roll 98.
TEST(hit_gate_misses_when_roll_reaches_threshold) {
    {
        tsup::Env env({0.5, 0.9});
        CHECK(!rollHitGate(env.ctx, 51));
    }
    {
        tsup::Env env({0.5, 0.9});
        CHECK(rollHitGate(env.ctx, 52));
    }
    {
        tsup::Env env({0.0, 0.9});
        CHECK(rollHitGate(env.ctx, 98));   // roll 1 hits
    }
    {
        tsup::Env env({0.975, 0.9});
        CHECK(!rollHitGate(env.ctx, 98));  // roll 98 misses at threshold 98
    }
    {
        tsup::Env env({0.965, 0.9});
        CHECK(rollHitGate(env.ctx, 98));   // roll 97 hits: 97% rate
    }
}

// R17 / R16: a swing whose gate roll fails deals 0 damage.
// atk 0 vs def 0 gives threshold 50; every draw of 0.99 gives roll 100.
TEST(melee_swing_misses_when_gate_roll_fails) {
    tsup::Env env({0.99, 0.99, 0.99, 0.99});
    CHECK_EQ(meleeHitAndDamage(env.ctx, 0, 0, 10, 10, 1.0), 0);
}

// R17: adjustment a = Int(((def - atk)/2)/4) = 125 for def 1000, atk 0.
// With U2 = 0 the term is 125 - 0 + 1 = 126, far above any raw damage
// at level 10 (about 4), so the 999-in-1000 adjustment wipes the swing.
// U3 = 0.5 makes Int(Rnd*1000)+1 = 501 > 1, so the adjustment applies.
// Threshold is 2 and U4 = 0.0 gives roll 1, so the gate passes.
TEST(defence_adjustment_wipes_small_damage_in_999_of_1000) {
    tsup::Env env({0.0, 0.0, 0.5, 0.0});
    CHECK_EQ(meleeHitAndDamage(env.ctx, 0, 1000, 10, 10, 1.0), 0);
}

// R16 / R22 / R27 / R31: miss, chain and record helpers.
TEST(miss_chain_and_record_helpers) {
    CHECK(isMissedSwing(0));
    CHECK(!isMissedSwing(1));

    CHECK(weaponChainContinues(5, true));    // a kill continues the chain
    CHECK(!weaponChainContinues(5, false));  // nonlethal blow ends it
    CHECK(!weaponChainContinues(0, false));  // miss ends it

    CHECK_EQ(recordedHitsFor(3, 10), 3);
    CHECK_EQ(recordedHitsFor(5, 2), 2);      // overkill is not counted
}

// R23 order: base*weapon (weapon<=0 treated as 1), +5 crit, +2 backstab,
// halve if invisible target, then size term (sizeDiff/10).
TEST(character_damage_mod_order_crit_then_invisible_halving) {
    // 0.6 * 2 = 1.2; +5 = 6.2; invisible halves to 3.1
    CHECK(nearly(characterDamageMod(0, 0.0, 2.0, true, false, true, 0), 3.1));
    // 0.6 * 1 = 0.6; +2 backstab = 2.6; size +10/10 = +1 gives 3.6
    CHECK(nearly(characterDamageMod(0, 0.0, 1.0, false, true, false, 10), 3.6));
    // weapon 0 is treated as 1 gives 0.6
    CHECK(nearly(characterDamageMod(0, 0.0, 0.0, false, false, false, 0), 0.6));
}

// F5 base: 0.6 + ((ln(100+L)/1.75 - 2.3) * fm^2) / 2.
// With fm = 0 the base is exactly 0.6. With L = 0, fm = 1:
// ln(100) = 4.6051702; /1.75 = 2.6315258; -2.3 = 0.3315258; /2 = 0.1657629; +0.6 = 0.7657629.
TEST(melee_base_factor_formula) {
    CHECK(nearly(meleeBaseFactor(0, 0.0), 0.6));
    CHECK(nearly(meleeBaseFactor(0, 1.0), 0.76576291));
}

// R25: companion defence adds +4 for crit and +1 for backstab.
// R25: companion damage halves against an unseen target.
TEST(companion_defence_and_damage_adjustments) {
    CHECK_EQ(companionGateDefence(10, false, false), 10);
    CHECK_EQ(companionGateDefence(10, true, false), 14);
    CHECK_EQ(companionGateDefence(10, false, true), 11);
    CHECK_EQ(companionGateDefence(10, true, true), 15);
    CHECK(nearly(companionDamageMod(3.0, false), 3.0));
    CHECK(nearly(companionDamageMod(3.0, true), 1.5));
}

// R29: the round loop ends only when one member is left.
// R28: a non-Alive DeathState leaves at once; Hits at nought or below need processing.
TEST(round_loop_and_death_flags) {
    CHECK(roundLoopEnds(1));
    CHECK(!roundLoopEnds(2));
    CHECK(!roundLoopEnds(3));

    Character c;
    CHECK(!leavesRoundAtOnce(c));
    c.deathState = DeathState::Dead;
    CHECK(leavesRoundAtOnce(c));

    Character h;
    h.vitals.curHits = 5;
    CHECK(!needsDeathProcessing(h));
    h.vitals.curHits = 0;
    CHECK(needsDeathProcessing(h));
}

// R8: no weapon, or total swings below 1, puts the actor on Defend.
// With a weapon and at least 1 swing the order is left alone.
TEST(no_usable_weapon_puts_actor_on_defend) {
    tsup::Env env({0.5, 0.9});

    Character a;
    (void)applyNoWeaponDefend(env.ctx, a, false, 2);
    CHECK(a.order.kind == CombatOrderKind::Defend);

    Character b;
    (void)applyNoWeaponDefend(env.ctx, b, true, 0);
    CHECK(b.order.kind == CombatOrderKind::Defend);

    Character c;
    (void)applyNoWeaponDefend(env.ctx, c, true, 2);
    CHECK(c.order.kind == CombatOrderKind::None);
}

// R16 / R19: unseen flag needs an invisible target and an actor without See Invisible.
TEST(unseen_needs_invisible_target_and_no_see_invisible) {
    Character actor;
    Creature target;

    target.unseen = false;
    CHECK(!swingUnseen(actor, target));

    target.unseen = true;
    CHECK(swingUnseen(actor, target));

    actor.ailments.seeInvisible = true;
    CHECK(!swingUnseen(actor, target));
}
