#include "test_support.h"
#include "mordor/companions2a.h"

using namespace mordor;

namespace {
// Tolerance comparison for f64 results.
bool nearlyEq(f64 a, f64 b, f64 tol = 1e-6) {
    f64 d = a - b;
    if (d < 0) d = -d;
    return d <= tol;
}
}  // namespace

// Binding state predicates and the joiner binding (R7, R11).
TEST(binding_predicates_and_joiner_value) {
    Companion c;
    c.bindingLevel = 1;
    CHECK(isBound(c));
    CHECK(!isContent(c));
    CHECK(!isForLife(c));

    c.bindingLevel = 0;
    CHECK(!isBound(c));
    CHECK(isContent(c));

    c.bindingLevel = kBindingMax;
    CHECK(isBound(c));
    CHECK(isForLife(c));

    c.bindingLevel = -10;
    CHECK(isContent(c));
    CHECK_EQ(joinerBinding(), -10);
}

// Starting binding = min(5 + monsterFloor, 10) (R8).
TEST(starting_binding_min_5_plus_floor_cap_10) {
    CHECK_EQ(startingBinding(0), 5);
    CHECK_EQ(startingBinding(3), 8);
    CHECK_EQ(startingBinding(4), 9);
    CHECK_EQ(startingBinding(5), 10);
    CHECK_EQ(startingBinding(30), 10);
}

// Bought binding = min(10, 5 + LevelFound) (R8).
TEST(bought_binding_min_10_of_5_plus_level) {
    CHECK_EQ(boughtBinding(0), 5);
    CHECK_EQ(boughtBinding(4), 9);
    CHECK_EQ(boughtBinding(5), 10);
    CHECK_EQ(boughtBinding(7), 10);
}

// Engine cost factor table (R54, engine preferred): f = a+4 while a+4 <= 12,
// else 12-a. So a=9 gives 3 and a=10 gives 2 (not the doc value 14).
TEST(binding_cost_factor_engine_table) {
    CHECK_EQ(bindingCostFactor(0), 4);
    CHECK_EQ(bindingCostFactor(1), 5);
    CHECK_EQ(bindingCostFactor(8), 12);
    CHECK_EQ(bindingCostFactor(9), 3);
    CHECK_EQ(bindingCostFactor(10), 2);
    CHECK_EQ(bindingCostFactor(11), 1);
    CHECK_EQ(bindingCostFactor(12), 0);
}

// Cost = GL * 1237 * f * current / 4 (R9). At current 12, f = 0 so cost is 0.
TEST(binding_cost_raw_formula_and_zero_at_12) {
    // GL 1, current 8, f = 12: 1 * 1237 * 12 * 8 / 4 = 29688
    CHECK(nearlyEq(bindingCostRaw(1, 8), 29688.0));
    // GL 5, current 12, f = 0: cost 0
    CHECK(nearlyEq(bindingCostRaw(5, 12), 0.0));
    CHECK_EQ(bindingCharge(5, 12), 0);
}

// Charge = round_half_even(Cost) (R9).
TEST(binding_charge_rounds_half_even) {
    // GL 1, current 1, f = 5: 1237 * 5 / 4 = 1546.25 -> 1546
    CHECK_EQ(bindingCharge(1, 1), 1546);
    // GL 2, current 9, f = 3: 2 * 1237 * 3 * 9 / 4 = 16699.5 -> 16700 (even)
    CHECK_EQ(bindingCharge(2, 9), 16700);
}

// Join bar = max(0, Cha*10 - (LevelFound-1)*10) (R10).
TEST(join_bar_cha_scale_minus_level_term_floor_0) {
    CHECK_EQ(joinBar(15, 3), 130);  // 150 - 20
    CHECK_EQ(joinBar(3, 1), 30);    // 30 - 0
    CHECK_EQ(joinBar(30, 1), 300);  // 300 - 0
    CHECK_EQ(joinBar(3, 10), 0);    // 30 - 90 = -60, floored to 0
}

// Empty slot is Ref == -1 (R2); the first free slot index is returned.
TEST(first_free_companion_slot_skips_occupied) {
    Arr<Companion, kCompanionSlots> slots{};
    CHECK_EQ(firstFreeCompanionSlot(slots), 0);

    slots[0].ref = 7;
    slots[0].occupied = true;
    CHECK_EQ(firstFreeCompanionSlot(slots), 1);

    slots[1].ref = 9;
    slots[1].occupied = true;
    CHECK_EQ(firstFreeCompanionSlot(slots), 2);
}

// Chance to leave is clamped to 0..90 (R35).
TEST(clamp_chance_to_leave_range_0_to_90) {
    CHECK_EQ(clampChanceToLeave(-5), 0);
    CHECK_EQ(clampChanceToLeave(0), 0);
    CHECK_EQ(clampChanceToLeave(45), 45);
    CHECK_EQ(clampChanceToLeave(90), 90);
    CHECK_EQ(clampChanceToLeave(91), 90);
    CHECK_EQ(clampChanceToLeave(500), 90);
}

// Raise chance = min(98, 85 + CON) (R42).
TEST(companion_raise_chance_min_98_of_85_plus_con) {
    CHECK_EQ(companionRaiseChance(0), 85);
    CHECK_EQ(companionRaiseChance(10), 95);
    CHECK_EQ(companionRaiseChance(13), 98);
    CHECK_EQ(companionRaiseChance(20), 98);
}

// Content drift = max(0.5, (MaxHits - Hits) / MaxHits * 15) (R34, quirk: full
// health gives the 0.5 floor under the engine).
TEST(drift_content_step_formula_and_half_floor) {
    CHECK(nearlyEq(driftContentStep(0, 10), 15.0));
    CHECK(nearlyEq(driftContentStep(5, 10), 7.5));
    CHECK(nearlyEq(driftContentStep(8, 10), 3.0));
    CHECK(nearlyEq(driftContentStep(9, 10), 1.5));
    CHECK(nearlyEq(driftContentStep(10, 10), 0.5));
}

// Bound drift is floored at 1 (R34). Both cases are checked where the
// log base does not matter: the raw value is 0 or negative.
TEST(drift_bound_step_never_below_one) {
    CHECK(nearlyEq(driftBoundStep(0, 0, 0), 1.0));
    CHECK(nearlyEq(driftBoundStep(1, 12, -100), 1.0));
}

// ForLife (binding 100) never drifts (R2 quick reference, term ForLife).
TEST(drift_companion_forlife_binding_unchanged) {
    tsup::Env env({0.5, 0.5});
    Companion c;
    c.bindingLevel = kBindingMax;
    c.hits = 2;
    c.maxHits = 10;
    driftCompanion(env.ctx, c, 12);
    CHECK_EQ(c.bindingLevel, kBindingMax);
}

// Stone chance = con * Log(Max(1,con)) / Log(2.5) - LevelFound, capped at 95
// (R30). The log base cancels in the ratio, so the result is base-independent.
TEST(stone_chance_con_log_ratio_minus_level_capped_95) {
    // 5 * ln5 / ln2.5 = 8.78235; minus 2 gives 6.78235
    CHECK(nearlyEq(companionStoneChance(5, 2), 6.78235, 1e-3));
    // con 1: log(1) = 0
    CHECK(nearlyEq(companionStoneChance(1, 0), 0.0));
    // con 100: raw value is about 502.6, capped at 95
    CHECK(nearlyEq(companionStoneChance(100, 0), 95.0));
}

// Stone roll: stoned when the roll is below the (capped) chance (R30).
TEST(stone_roll_uses_capped_chance) {
    {
        tsup::Env env({0.5, 0.5});
        CHECK(companionStoneRoll(env.ctx, 100, 0));   // 50 < 95
    }
    {
        tsup::Env env({0.97, 0.97});
        CHECK(!companionStoneRoll(env.ctx, 100, 0));  // 97 >= 95
    }
}

// Crit bonus +4 (bit 10) and +1 (bit 11); added to damMod (R28).
TEST(companion_crit_bonus_and_blow_mod) {
    CHECK_EQ(companionCritBonus(0u), 0);
    CHECK_EQ(companionCritBonus(kSpecialBitCrit10), 4);
    CHECK_EQ(companionCritBonus(kSpecialBitCrit11), 1);
    CHECK_EQ(companionCritBonus(kSpecialBitCrit10 | kSpecialBitCrit11), 5);

    CHECK_EQ(companionBlowDamageMod(3, 0u), 3);
    CHECK_EQ(companionBlowDamageMod(3, kSpecialBitCrit10), 7);
    CHECK_EQ(companionBlowDamageMod(3, kSpecialBitCrit10 | kSpecialBitCrit11), 8);
}

// Resistance: after = damage - damage * (resist / 2) / 100 (R29 worked examples).
TEST(resisted_companion_damage_worked_examples) {
    CHECK_EQ(resistedCompanionDamage(100, 90), 55);
    CHECK_EQ(resistedCompanionDamage(100, 40), 80);
    CHECK_EQ(resistedCompanionDamage(100, 0), 100);
    CHECK_EQ(resistedCompanionDamage(50, 20), 45);
}

// Breath fire: strong lead gives 80, weak lead gives 20 (R23, R28). Only the
// clearly strong and clearly weak cases are tested; the threshold is unknown.
TEST(breath_fire_percent_strong_and_weak_leads) {
    CHECK_EQ(breathFirePercent(20, 5), 80);
    CHECK_EQ(breathFirePercent(1, 20), 20);
}

// Breath sweep stops on a surviving stopper or after the fifth kill (R23, c28).
TEST(breath_sweep_stops_on_survivor_or_fifth_kill) {
    CHECK(breathSweepContinues(0, false));
    CHECK(breathSweepContinues(4, false));
    CHECK(!breathSweepContinues(5, false));
    CHECK(!breathSweepContinues(2, true));
}

// Mishap clamp table: Breath 2.0 (2 to 40), Spell 1.5 (1 to 40), Spit 1.0 (0.5 to 20).
TEST(mishap_range_table_matches_srd) {
    CHECK(nearlyEq(kMishapRanges[0].factor, 2.0));
    CHECK(nearlyEq(kMishapRanges[0].low, 2.0));
    CHECK(nearlyEq(kMishapRanges[0].high, 40.0));

    CHECK(nearlyEq(kMishapRanges[1].factor, 1.5));
    CHECK(nearlyEq(kMishapRanges[1].low, 1.0));
    CHECK(nearlyEq(kMishapRanges[1].high, 40.0));

    CHECK(nearlyEq(kMishapRanges[2].factor, 1.0));
    CHECK(nearlyEq(kMishapRanges[2].low, 0.5));
    CHECK(nearlyEq(kMishapRanges[2].high, 20.0));
}

// Victim draw = round_half_even(rng * (partySize - 1)) + 1 (c8).
TEST(mishap_victim_index_rounds_half_even_plus_one) {
    {
        tsup::Env env({0.0, 0.0});
        CHECK_EQ(mishapVictimIndex(env.ctx, 4), 1);   // round(0) = 0
    }
    {
        tsup::Env env({0.5, 0.5});
        CHECK_EQ(mishapVictimIndex(env.ctx, 4), 3);   // round_half_even(1.5) = 2
    }
    {
        tsup::Env env({0.99, 0.99});
        CHECK_EQ(mishapVictimIndex(env.ctx, 4), 4);   // round(2.97) = 3
    }
}

// Companion caster failure is 50 percent (R-quick reference, c64).
TEST(companion_cast_fails_at_50_percent) {
    {
        tsup::Env env({0.1, 0.1});
        CHECK(companionCastFails(env.ctx));    // 10 < 50
    }
    {
        tsup::Env env({0.9, 0.9});
        CHECK(!companionCastFails(env.ctx));   // 90 >= 50
    }
}

// Turn check: turns when |creature - owner| > 1 on the 0/1/2 scheme (R36).
// The measured Neutral rate in the docs is not tested here; it conflicts
// with this formula.
TEST(alignment_turns_when_scheme_gap_exceeds_one) {
    CHECK(alignmentTurns(CharAlign::Good, Align::Evil));
    CHECK(alignmentTurns(CharAlign::Evil, Align::Good));
    CHECK(!alignmentTurns(CharAlign::Good, Align::Neutral));
    CHECK(!alignmentTurns(CharAlign::Neutral, Align::Evil));
    CHECK(!alignmentTurns(CharAlign::Evil, Align::Evil));
}

// Good monsters join only Good characters; Neutral monsters join any (wiki table).
TEST(monster_joins_good_only_good_characters) {
    CHECK(monsterJoinsCharacter(CharAlign::Good, Align::Good));
    CHECK(!monsterJoinsCharacter(CharAlign::Good, Align::Neutral));
    CHECK(!monsterJoinsCharacter(CharAlign::Good, Align::Evil));
    CHECK(monsterJoinsCharacter(CharAlign::Neutral, Align::Evil));
}

// Id 307 split: Evil gives Shadow Ninja (389); Neutral, Good and Unaligned give Morgul (278).
TEST(id307_creature_ref_split_by_alignment) {
    CHECK_EQ(id307CreatureRef(ItemAlign::Evil), kId307ShadowNinjaRef);
    CHECK_EQ(id307CreatureRef(ItemAlign::Neutral), kId307MorgulRef);
    CHECK_EQ(id307CreatureRef(ItemAlign::Good), kId307MorgulRef);
    CHECK_EQ(id307CreatureRef(ItemAlign::Unaligned), kId307MorgulRef);
}
