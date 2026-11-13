#include "test_support.h"

#include "mordor/companions2b.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

using namespace mordor;
using tsup::Env;

// Binding step cost. Engine formula preferred per SRD conflict note 1:
// f = (a + 4 > 12) ? 12 - a : a + 4. Cost is linear in current.

TEST(bind_cost_zero_at_max_binding) {
    // a = 12 gives f = 0, so the step is free at max.
    CHECK_EQ(bindingStepCost(10000, 12), 0);
    CHECK_EQ(bindingStepCost(987654, 12), 0);
}

TEST(bind_cost_cheaper_near_top_and_linear_in_current) {
    const Currency c = 10000;
    // f(8) = 12, f(9) = 3, f(10) = 2, f(11) = 1.
    CHECK_EQ(bindingStepCost(c, 8), 4 * bindingStepCost(c, 9));
    CHECK_EQ(bindingStepCost(c, 10), 2 * bindingStepCost(c, 11));
    CHECK(bindingStepCost(c, 9) < bindingStepCost(c, 8));
    // "Cost multiplies current": doubling current doubles cost.
    CHECK_EQ(bindingStepCost(2 * c, 8), 2 * bindingStepCost(c, 8));
}

// Chance to leave.

TEST(chance_to_leave_clamped_to_ninety) {
    CHECK_EQ(clampChanceToLeave(95), kChanceToLeaveMax);
    CHECK_EQ(clampChanceToLeave(90), 90);
    CHECK_EQ(clampChanceToLeave(42), 42);
}

// Raise chance is min(98, 85 + CON).

TEST(companion_raise_chance_is_85_plus_con_capped_at_98) {
    CHECK_EQ(companionRaiseChance(0), 85);
    CHECK_EQ(companionRaiseChance(12), 97);
    CHECK_EQ(companionRaiseChance(13), 98);  // exact cap boundary
    CHECK_EQ(companionRaiseChance(40), 98);
}

// Turning rules.

TEST(good_creature_under_good_master_never_turns) {
    CHECK_EQ(companionTurnPercent(CharAlign::Good, Align::Good), 0);
    // FIXED: roll values are u64 in the Env helper; 0.0 -> 0 (scale 1000 = 100%).
    Env env({0});  // even a roll at the very bottom does not turn it
    CHECK(!companionTurnRoll(env.ctx, CharAlign::Good, Align::Good));
}

TEST(evil_creature_under_good_master_always_turns) {
    // Two-step gap always turns (docs attribute the 100% to this gap).
    CHECK_EQ(companionTurnPercent(CharAlign::Evil, Align::Good), 100);
    // FIXED: 0.999 -> 999 (scale 1000 = 100%).
    Env env({999});
    CHECK(companionTurnRoll(env.ctx, CharAlign::Evil, Align::Good));
}

TEST(kick_out_leaves_silently_when_companion_does_not_turn) {
    // Good creature under Good master does not turn, so it leaves with no print.
    // FIXED: 0.0 -> 0 (scale 1000 = 100%).
    Env env({0});
    CHECK(kickOutLeavesSilently(env.ctx, CharAlign::Good, Align::Good));
}

// Breath stop rule: fire is never stopped by an earlier breath; cold is.

TEST(fire_breath_not_stopped_by_earlier_breath) {
    CHECK(!breathStopped(BreathType::Fire, true));
}

TEST(cold_breath_stopped_by_earlier_breath) {
    CHECK(breathStopped(BreathType::Cold, true));
}

// Breath roll fires only on a roll below the chance (chance is 20, 50 or 80).

TEST(breath_roll_fires_only_below_chance) {
    // FIXED: 0.0 -> 0 (scale 1000 = 100%).
    Env low({0});
    CHECK(breathRollFires(low.ctx, 0));    // roll 0 is below any chance
    // FIXED: 0.999 -> 999 (scale 1000 = 100%).
    Env high({999});
    CHECK(!breathRollFires(high.ctx, 0));  // roll 99.9 is never below 80
}

// Special range extras: breath a quarter of range; spit and electrocution a half.

TEST(special_range_extra_quarter_for_breath_half_for_spit_and_shock) {
    CHECK_EQ(specialRangeExtra(SpecialAttackKind::Breath, 40), 10);
    CHECK_EQ(specialRangeExtra(SpecialAttackKind::AcidSpit, 40), 20);
    CHECK_EQ(specialRangeExtra(SpecialAttackKind::Electrocute, 40), 20);
}

// Stone follow-up can land on the next monster in the group that has hits left.

TEST(next_stone_foe_skips_dead_monster) {
    Vec<Creature> foes;
    Creature a;
    a.hits = 5;
    Creature b;
    b.hits = 0;
    Creature c;
    c.hits = 7;
    foes.push_back(a);
    foes.push_back(b);
    foes.push_back(c);
    CHECK_EQ(nextStoneFoeIndex(foes, 1), 2);
}

// Release keeps the last occupant's record.

TEST(released_slot_keeps_last_occupant_record) {
    // FIXED: 0.5 -> 500 (scale 1000 = 100%).
    Env env({500});
    Companion comp;
    comp.monsterId = 42;
    comp.name = "Ghoul";
    Str before = comp.name;
    releaseCompanionSlot(env.ctx, comp);
    CHECK_EQ(comp.monsterId, 42);
    CHECK(comp.name == before);
}

// Drag swap is refused if either end is dead.

TEST(drag_swap_refused_when_either_end_is_dead) {
    Companion a;
    Companion b;
    CHECK(!dragSwapBlockedByDeath(a, b));
    a.isDead = true;
    CHECK(dragSwapBlockedByDeath(a, b));
    a.isDead = false;
    b.isDead = true;
    CHECK(dragSwapBlockedByDeath(a, b));
}

// Purchase hits are capped at the definition's max.

TEST(purchase_hits_capped_at_definition_hits) {
    CHECK_EQ(clampPurchaseHits(5, 10), 5);
    CHECK_EQ(clampPurchaseHits(10, 10), 10);
    CHECK_EQ(clampPurchaseHits(12, 10), 10);
}

// Joiners from a fight take the front monster's hits and maximum.

TEST(joiner_takes_front_monster_hits_and_maximum) {
    // FIXED: 0.5 -> 500 (scale 1000 = 100%).
    Env env({500});
    Companion joiner;
    joiner.hits = 3;
    joiner.maxHits = 4;
    Creature front;
    front.hits = 7;
    front.maxHits = 12;
    applyJoinerHitsFromFront(env.ctx, joiner, front);
    CHECK_EQ(joiner.hits, 7);
    CHECK_EQ(joiner.maxHits, 12);
}

// Only group 0's join offer is ever shown.

TEST(join_offer_never_visible_for_group_one) {
    Encounter enc;
    enc.groups[0].joinOffer = true;
    enc.groups[1].joinOffer = true;
    CHECK(!joinOfferVisibleFor(enc, 1));
}

// Join offer admits a group whose alignment matches the viewer.
// (Alignment-1 admission deliberately not tested: SRD conflict 19.)

TEST(join_offer_admits_matching_alignment_only_here) {
    CHECK(joinOfferAdmits(CharAlign::Good, Align::Good));
    CHECK(joinOfferAdmits(CharAlign::Evil, Align::Evil));
    CHECK(!joinOfferAdmits(CharAlign::Good, Align::Evil));
}

// Bind spells do not work on companions that joined of their own free will.

TEST(bind_spell_skips_companions_that_joined_freely) {
    Companion freeWill;
    freeWill.joinedFreely = true;
    CHECK(!bindSpellApplies(freeWill));
    Companion bought;
    bought.joinedFreely = false;
    CHECK(bindSpellApplies(bought));
}

// Only Id 307 is shared with the Morgul.

TEST(only_monster_307_is_shared_morgul_id) {
    CHECK(isSharedMorgulId(307));
    CHECK(!isSharedMorgulId(306));
    CHECK(!isSharedMorgulId(308));
}

// No monster in MDATA5 has spawn mode 5, 6 or 7.

TEST(no_monster_data_spawn_mode_five_six_or_seven) {
    const GameData& d = tsup::data();
    CHECK(!spawnModeInData(d, 5));
    CHECK(!spawnModeInData(d, 6));
    CHECK(!spawnModeInData(d, 7));
}

// Companion casts at guild level / 6.

TEST(companion_cast_level_is_guild_level_over_six) {
    Companion c;
    c.guildLevel = 5;
    CHECK_EQ(companionCastLevel(c), 0);
    c.guildLevel = 6;
    CHECK_EQ(companionCastLevel(c), 1);
    c.guildLevel = 11;
    CHECK_EQ(companionCastLevel(c), 1);
    c.guildLevel = 12;
    CHECK_EQ(companionCastLevel(c), 2);
}

// Heals with M5B1D = 0 reach nobody.

TEST(heal_with_zero_flag_reaches_nobody) {
    CHECK(spellHealsNoOne(0));
    CHECK(!spellHealsNoOne(1));
}

// While a morgue body is carried, the carry choice is skipped.

TEST(carry_choice_skipped_while_morgue_body_carried) {
    Party party;
    party.carryingSlot = CarryingSlot::DeadCharacter;
    CHECK(!carryChoiceApplies(party));
}
