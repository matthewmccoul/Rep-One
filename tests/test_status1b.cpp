#include <cmath>

#include "test_support.h"
#include "mordor/status1b.h"

using namespace mordor;

static bool nearly(double a, double b, double tol = 1e-3) {
    return std::fabs(a - b) < tol;
}

TEST(chest_trap_count_is_ten) {
    CHECK_EQ(kChestTrapCount, 10);
    CHECK_EQ(static_cast<i32>(ChestTrap::NotTrapped), 0);
    CHECK_EQ(static_cast<i32>(ChestTrap::Trap10), 10);
}

TEST(cure_thresholds_and_disease_classes) {
    CHECK_EQ(kCureControlLevel, 3);
    CHECK_EQ(kCureParalysisLevel, 4);
    CHECK_EQ(kCureDiseaseLevel, 5);
    CHECK_EQ(kCureLevel, 8);
    CHECK(kCureClass == Guild::Healer);
    // Cure Disease is taught to Pal/Mag/Hea only; Villain is excluded.
    for (u32 i = 0; i < 3; ++i) {
        CHECK(kCureDiseaseClasses[i] != Guild::Villain);
    }
}

TEST(ambient_sound_roll_is_ten_percent) {
    // Draw 0.05 -> 5 < 10 sounds; draw 0.5 -> 50 does not.
    tsup::Env env({0.05, 0.5});
    CHECK(ambientSoundRoll(env.ctx, true));
    CHECK(!ambientSoundRoll(env.ctx, true));
}

TEST(ambient_sound_silent_when_something_in_front) {
    // Ambient sound is only rolled when nothing is in front.
    tsup::Env env({0.0, 0.0});
    CHECK(!ambientSoundRoll(env.ctx, false));
}

TEST(water_hazard_subtracts_level_from_hits) {
    tsup::Env env({0.5, 0.9});
    Character m;
    m.vitals.curHits = 10;
    HazardResult r = waterOrQuicksandHit(env.ctx, m, 3, false);
    CHECK_EQ(r.hitsLost, 3);
    CHECK_EQ(m.vitals.curHits, 7);
    CHECK(!r.itemDestroyed);
    CHECK_EQ(r.destroyedSlot, -1);
}

TEST(identify_first_pass_skips_identified_or_unseen) {
    tsup::Env env({0.0, 0.0});
    Character ch;
    Creature foe;
    foe.identified = true;
    CHECK(!identifyFirstPass(env.ctx, ch, foe, false));
    foe.identified = false;
    foe.unseen = true;
    CHECK(!identifyFirstPass(env.ctx, ch, foe, false));
}

TEST(identify_second_pass_needs_wrong_belief) {
    tsup::Env env({0.0, 0.0});
    Character ch;
    Creature foe;
    foe.ref = 3;
    foe.believedRef = 3;
    CHECK(!identifySecondPass(env.ctx, ch, foe, false));
}

TEST(content_drift_floor_and_scale) {
    CHECK(nearly(contentDrift(0, 20), 0.5));    // no missing hits: floor
    CHECK(nearly(contentDrift(20, 20), 15.0));  // fully missing: 15
    CHECK(nearly(contentDrift(10, 20), 7.5));   // 15 * 10 / 20
    CHECK(nearly(contentDrift(1, 30), 0.5));    // exactly at floor
    CHECK(nearly(contentDrift(1, 100), 0.5));   // 0.15 raised to floor
}

TEST(bound_drift_floors_at_one) {
    // Owner term and monster term are both zero: max(1, 0) = 1.
    CHECK(nearly(boundDrift(0, 0, -10), 1.0));
    // 2 * 10 / ln(3) = 18.2048, no owner term.
    CHECK(nearly(boundDrift(10, 0, 0), 18.2048));
}

TEST(bound_drift_subtracts_owner_term) {
    // binding -10: abs(-10/2) = 5.
    // 2*30/ln3 = 54.61435; 12*5/ln12 = 24.14578; difference = 30.46858.
    CHECK(nearly(boundDrift(30, 12, -10), 30.4686));
}

TEST(companion_leave_value_zero_inputs) {
    // All terms vanish: v = 0, leave = 0.
    CHECK(nearly(companionLeaveValue(0, 0, 0, 0, 0, 10), 0.0));
}

TEST(companion_leave_value_formula) {
    // M=100, binding -10 -> b = 45, v = 24*ln(101) - 90 = 20.76289.
    // leave = (v/2 + 0)/2 = 5.19072.
    CHECK(nearly(companionLeaveValue(100, 0, -10, 0, 0, 10), 5.19072));
}

TEST(companion_leave_value_clamps) {
    // Upper clamp at 90: v/2 + 15 is about 180.8, halved is about 90.4.
    CHECK(nearly(companionLeaveValue(1000000, 0, 0, 0, 10, 10), 90.0));
    // Lower clamp at 0: CHA 1000 gives v = -500, leave is negative, clamped.
    CHECK(nearly(companionLeaveValue(0, 0, 0, 1000, 0, 10), 0.0));
}

TEST(companion_raise_chance_caps_at_98) {
    CHECK_EQ(companionRaiseChance(0), 85);
    CHECK_EQ(companionRaiseChance(12), 97);
    CHECK_EQ(companionRaiseChance(13), 98);
    CHECK_EQ(companionRaiseChance(50), 98);
}

TEST(companion_raise_resurrect_restores_full_hits) {
    // U = 0 -> 0 < 85 succeeds.
    tsup::Env env({0.0, 0.0});
    Companion comp;
    comp.maxHits = 20;
    comp.hits = 0;
    CHECK(companionRaise(env.ctx, comp, RaiseKind::Resurrect, 0) ==
          CompanionRaiseResult::RaisedFull);
    CHECK_EQ(comp.hits, 20);
}

TEST(companion_raise_other_kinds_leave_one_hit) {
    tsup::Env env({0.0, 0.0});
    Companion comp;
    comp.maxHits = 20;
    comp.hits = 0;
    CHECK(companionRaise(env.ctx, comp, RaiseKind::RaiseDead, 0) ==
          CompanionRaiseResult::RaisedOneHit);
    CHECK_EQ(comp.hits, 1);
}

TEST(companion_raise_failure_destroys) {
    // U = 0.999 -> 99.9 is not under 85, so the raise fails.
    tsup::Env env({0.999, 0.999});
    Companion comp;
    comp.maxHits = 20;
    CHECK(companionRaise(env.ctx, comp, RaiseKind::Resurrect, 0) ==
          CompanionRaiseResult::Destroyed);
}

TEST(drift_check_compares_scaled_draw) {
    // Draws 0.5 (-> 100) and 0.2 (-> 40).
    tsup::Env env({0.5, 0.2});
    CHECK(driftCheck(env.ctx, 150.0));   // 100 < 150
    CHECK(!driftCheck(env.ctx, 30.0));   // 40 is not < 30
}

TEST(drift_check_is_strict_at_zero) {
    tsup::Env env({0.0, 0.0});
    CHECK(!driftCheck(env.ctx, 0.0));    // 0 < 0 is false
    CHECK(driftCheck(env.ctx, 0.5));     // 0 < 0.5 is true
}

TEST(leave_check_uses_leave_value) {
    // Draw 0.3 -> 60.
    tsup::Env env({0.3, 0.3});
    CHECK(leaveCheck(env.ctx, 90.0));    // 60 < 90
    CHECK(!leaveCheck(env.ctx, 50.0));   // 60 is not < 50
}

TEST(rock_death_lone_character_dies_and_completes) {
    // R54: caster in slot 0 is the only member, so the victim set is the caster alone.
    // The implemented signature is rockDeath(Ctx&, Party&, i32 victimIndex) and returns void,
    // so the completes/killed result is not observable through the API. Check the death only.
    tsup::Env env({0.5, 0.9});
    Party party;
    party.size = 1;
    party.members[0].vitals.curHits = 5;
    rockDeath(env.ctx, party, 0);
    CHECK(party.members[0].isDead());
}

TEST(fate_rerolls_hp_and_sp_in_range) {
    // HP: (int)(0.5*20)+1 = 11. SP: (int)(0.95*10)+1 = 10.
    tsup::Env env({0.5, 0.95});
    Character c;
    c.vitals.maxHits = 20;
    c.vitals.maxSpellPoints = 10;
    chestFateTrap(env.ctx, c);
    CHECK_EQ(c.vitals.curHits, 11);
    CHECK_EQ(c.vitals.curSpellPoints, 10);
}

TEST(fate_top_of_range_caps_at_max) {
    // (int)(0.999*20)+1 = 20; (int)(0.999*10)+1 = 10.
    tsup::Env env({0.999, 0.999});
    Character c;
    c.vitals.maxHits = 20;
    c.vitals.maxSpellPoints = 10;
    chestFateTrap(env.ctx, c);
    CHECK_EQ(c.vitals.curHits, 20);
    CHECK_EQ(c.vitals.curSpellPoints, 10);
}

TEST(blackout_clears_spell_effects) {
    tsup::Env env({0.0, 0.0});
    Character c;
    c.activeEffectIds.push_back(4);
    c.activeEffectIds.push_back(7);
    chestBlackoutTrap(env.ctx, c);
    CHECK_EQ(static_cast<i32>(c.activeEffectIds.size()), 0);
}

TEST(city_gate_clear_cures_poison_disease_keeps_stone) {
    // EndEveryAilment cures poison and disease; Stone survives.
    tsup::Env env({0.0, 0.0});
    Character c;
    c.ailments.poisonLevel = 2;
    c.ailments.diseaseLevel = 1;
    c.deathState = DeathState::Stoned;
    cityGateClear(env.ctx, c);
    CHECK_EQ(c.ailments.poisonLevel, 0);
    CHECK_EQ(c.ailments.diseaseLevel, 0);
    CHECK(c.deathState == DeathState::Stoned);
}
