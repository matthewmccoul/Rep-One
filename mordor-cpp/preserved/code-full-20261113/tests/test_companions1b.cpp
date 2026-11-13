#include <cmath>

#include "test_support.h"
#include "mordor/companions1b.h"

namespace mo = mordor;

namespace {
bool approx(double a, double b, double tol) { return std::fabs(a - b) <= tol; }
}  // namespace

// R27.1: companion casts only if no cast this round and no anti-magic.
// R27.3: order goes to Defend for Att 0 or 1, Fight for Att 2.
TEST(cast_gate_and_order_after_cast) {
    CHECK(mo::companionMayCast(false, false));
    CHECK(!mo::companionMayCast(true, false));
    CHECK(!mo::companionMayCast(false, true));
    CHECK(mo::orderAfterCompanionCast(0) == mo::CombatOrderKind::Defend);
    CHECK(mo::orderAfterCompanionCast(1) == mo::CombatOrderKind::Defend);
    CHECK(mo::orderAfterCompanionCast(2) == mo::CombatOrderKind::Fight);
}

// R27.6: companion casts at guild level / 6. Heals with M5B1D = 0 reach nobody.
TEST(cast_level_and_heal_route) {
    CHECK_EQ(mo::companionCastLevel(5), 0);
    CHECK_EQ(mo::companionCastLevel(6), 1);
    CHECK_EQ(mo::companionCastLevel(12), 2);
    CHECK(mo::companionHealRoute(false, 0) == mo::HealRoute::Nobody);
    CHECK(mo::companionHealRoute(true, 0) == mo::HealRoute::Nobody);
}

// R27.9: spell odds x 1.5, clamped to 1..40.
TEST(spell_mishap_chance_clamps_1_to_40) {
    CHECK(approx(mo::companionSpellMishapChance(0.5), 1.0, 1e-9));   // 0.75 -> floor 1
    CHECK(approx(mo::companionSpellMishapChance(10.0), 15.0, 1e-9));
    CHECK(approx(mo::companionSpellMishapChance(30.0), 40.0, 1e-9)); // 45 -> ceiling 40
}

// R27.5: non-Mage caster gives up half the time; Mage never gives up.
// Companion failure is 50% for non-Mage casters.
TEST(caster_fizzle_and_fail_rules) {
    tsup::Env mage({0.0, 0.0});
    CHECK(!mo::companionCastFizzles(mage.ctx, true));

    tsup::Env gives({0.0, 0.0});
    CHECK(mo::companionCastFizzles(gives.ctx, false));
    tsup::Env keeps({0.99, 0.99});
    CHECK(!mo::companionCastFizzles(keeps.ctx, false));

    tsup::Env failsLow({0.0, 0.0});
    CHECK(mo::companionCastFails(failsLow.ctx));
    tsup::Env failsHigh({0.99, 0.99});
    CHECK(!mo::companionCastFails(failsHigh.ctx));
}

// R27.2: the charge comes from the owner's item in the given pack slot.
// With no charges left the cast is refused and nothing changes.
TEST(cast_charge_comes_from_pack_item) {
    mo::Inventory inv;
    inv.pack[2].charges = 3;
    tsup::Env env({0.5, 0.5});
    CHECK(mo::companionTakeCastCharge(env.ctx, inv, 2));
    CHECK_EQ(inv.pack[2].charges, 2);
    inv.pack[2].charges = 0;
    CHECK(!mo::companionTakeCastCharge(env.ctx, inv, 2));
    CHECK_EQ(inv.pack[2].charges, 0);
}

// R47: Good monsters join Good characters only and are never hostile to Good.
// Good monsters are always hostile to Neutral and Evil. Neutral monsters join anyone.
// Evil monsters are always hostile.
TEST(monster_join_and_hostility_rules) {
    CHECK(mo::monsterMayJoin(mo::CharAlign::Good, mo::Align::Good));
    CHECK(!mo::monsterMayJoin(mo::CharAlign::Good, mo::Align::Neutral));
    CHECK(!mo::monsterMayJoin(mo::CharAlign::Good, mo::Align::Evil));
    CHECK(mo::monsterMayJoin(mo::CharAlign::Neutral, mo::Align::Good));
    CHECK(mo::monsterMayJoin(mo::CharAlign::Neutral, mo::Align::Neutral));
    CHECK(mo::monsterMayJoin(mo::CharAlign::Neutral, mo::Align::Evil));

    tsup::Env env({0.0, 0.0});
    CHECK(!mo::monsterHostileToCharacter(env.ctx, mo::CharAlign::Good, mo::Align::Good));
    CHECK(mo::monsterHostileToCharacter(env.ctx, mo::CharAlign::Good, mo::Align::Neutral));
    CHECK(mo::monsterHostileToCharacter(env.ctx, mo::CharAlign::Good, mo::Align::Evil));
    CHECK(mo::monsterHostileToCharacter(env.ctx, mo::CharAlign::Evil, mo::Align::Good));
}

// R28: crit flag is bit 10. Unseen targets lose damMod / 2 (integer halving).
// With no crit or backstab flags no crit roll is involved.
TEST(blow_flag_bits_and_unseen_damage_mod) {
    CHECK(mo::specialFlagBit(1u << mo::kBlowFlagCrit, mo::kBlowFlagCrit));
    CHECK(!mo::specialFlagBit(1u << mo::kBlowFlagCrit, mo::kBlowFlagBackstab));
    CHECK(!mo::specialFlagBit(0u, mo::kBlowFlagStone));

    tsup::Env env({0.5, 0.5});
    mo::BlowParams p;
    p.damageMod = 10;
    CHECK_EQ(mo::blowDamageMod(env.ctx, p, false, 0), 10);
    CHECK_EQ(mo::blowDamageMod(env.ctx, p, true, 0), 5);
    p.damageMod = 9;
    CHECK_EQ(mo::blowDamageMod(env.ctx, p, true, 0), 5); // 9 - 4
}

// R29: after = damage - damage * (resist / 2) / 100.
// Worked examples from the SRD: 100 at 90% -> 55, 100 at 40% -> 80.
TEST(after_resistance_removes_half_the_percentage) {
    CHECK_EQ(mo::afterResistance(100, 90), 55);
    CHECK_EQ(mo::afterResistance(100, 40), 80);
    CHECK_EQ(mo::afterResistance(100, 0), 100);
}

// R30: chance = con * (Log(Max(1,con)) / Log(2.5)) - LevelFound, capped at 95.
// CON 10 at LevelFound 0: 10 * ln10 / ln2.5 = 25.129 (stone chance 74.87 per docs).
TEST(stone_chance_con_log_formula_capped_95) {
    CHECK(approx(mo::stoneChance(10, 0), 25.1294, 0.001));
    CHECK(approx(mo::stoneChance(10, 5), 20.1294, 0.001));
    CHECK(approx(mo::stoneChance(40, 0), 95.0, 1e-9));
}

// R30: stone roll is rnd(100)+1 > res, then rnd()*100 > chance means stoned.
// Resistance 100 can never be stoned. Chance is capped at 95.
TEST(stone_lands_resist_100_and_cap_95) {
    tsup::Env never({0.99, 0.0});
    CHECK(!mo::stoneLands(never.ctx, 100, 10, 0));

    tsup::Env below({0.9, 0.94});  // 94 > 95 is false
    CHECK(!mo::stoneLands(below.ctx, 0, 40, 0));

    tsup::Env above({0.9, 0.96});  // 96 > 95 is true
    CHECK(mo::stoneLands(above.ctx, 0, 40, 0));
}

// R31: electrocution is AfterResistance(raw, res). With zero resistance it is raw.
TEST(electrocution_zero_resist_is_raw_damage) {
    int raw = mo::rawElectrocuteDamage(10, 5);
    CHECK_EQ(mo::electrocutionDamage(10, 5, 0), raw);
}

// R30/R32: stone and electrocute follow-ups test Foes(...).Hits > 0,
// so the next live foe in the group is picked.
TEST(next_live_foe_skips_dead_foes) {
    mo::Vec<mo::Creature> foes;
    mo::Creature a;
    a.hits = 0;
    foes.push_back(a);
    mo::Creature b;
    b.hits = 5;
    foes.push_back(b);
    mo::Creature d;
    d.hits = 0;
    foes.push_back(d);
    mo::Creature e;
    e.hits = 3;
    foes.push_back(e);
    CHECK_EQ(mo::nextLiveFoe(foes, 0), 1);
    CHECK_EQ(mo::nextLiveFoe(foes, 2), 3);
}

// R34: drift content = max(0.5, (MaxHits - Hits) / MaxHits * 15), 0 if MaxHits <= 0.
// drift bound = max(1, GuildLevel / ln3 * 2 - TopGuildLevel / ln12 * |Binding / 2|).
TEST(drift_content_and_bound_formulas) {
    CHECK(approx(mo::companionDriftContent(0, 10), 15.0, 1e-9));
    CHECK(approx(mo::companionDriftContent(9, 10), 1.5, 1e-9));
    CHECK(approx(mo::companionDriftContent(10, 10), 0.5, 1e-9));
    CHECK(approx(mo::companionDriftContent(3, 0), 0.0, 1e-9));

    CHECK(approx(mo::companionDriftBound(10, 0, 5), 18.2048, 0.001));
    CHECK(approx(mo::companionDriftBound(0, 5, 4), 1.0, 1e-9)); // negative term floors at 1
}

// R34: if rng()*200 < Drift then Binding moves one notch toward 0.
// Negative binding drifts up by 1 on a low roll. Positive binding drifts down.
// A high roll (198 is not below 15) leaves binding unchanged.
TEST(mood_round_drifts_binding_on_low_roll) {
    mo::Companion neg;
    neg.bindingLevel = -10;
    neg.hits = 0;
    neg.maxHits = 10;
    tsup::Env low({0.0, 0.0});
    mo::companionMoodRound(low.ctx, neg, 0);
    CHECK_EQ(neg.bindingLevel, -9);

    mo::Companion quiet;
    quiet.bindingLevel = -10;
    quiet.hits = 0;
    quiet.maxHits = 10;
    tsup::Env high({0.99, 0.99});
    mo::companionMoodRound(high.ctx, quiet, 0);
    CHECK_EQ(quiet.bindingLevel, -10);

    mo::Companion pos;
    pos.bindingLevel = 10;
    tsup::Env posLow({0.0, 0.0});
    mo::companionMoodRound(posLow.ctx, pos, 0);
    CHECK_EQ(pos.bindingLevel, 9);
}

// R35: bound = round_half_even(|binding| * 4.5). Halves round to even:
// 4.5 -> 4, 13.5 -> 14, 22.5 -> 22.
TEST(bound_from_binding_rounds_half_even) {
    CHECK_EQ(mo::boundFromBinding(10), 45);
    CHECK_EQ(mo::boundFromBinding(-1), 4);
    CHECK_EQ(mo::boundFromBinding(3), 14);
    CHECK_EQ(mo::boundFromBinding(5), 22);
    CHECK_EQ(mo::boundFromBinding(-100), 450);
}

// R35: binding 100 gives chance 0. Base case with zero inputs and 0 of 10 hits
// gives x = 15, halved to 7.5. Charisma 20 gives 5. Extremes clamp to 0..90.
TEST(chance_to_leave_base_and_clamps) {
    mo::Companion c;
    c.bindingLevel = 0;
    c.hits = 0;
    c.maxHits = 10;
    CHECK(approx(mo::companionChanceToLeave(c, 0, 0), 7.5, 1e-9));
    CHECK(approx(mo::companionChanceToLeave(c, 0, 20), 5.0, 1e-9));
    CHECK(approx(mo::companionChanceToLeave(c, 0, -1000), 90.0, 1e-9));
    CHECK(approx(mo::companionChanceToLeave(c, 0, 1000), 0.0, 1e-9));
    c.bindingLevel = 100;
    CHECK(approx(mo::companionChanceToLeave(c, 0, 0), 0.0, 1e-9));
}

// R35 describe table: 0 Completely, <=3 Excellently, <=10 Fairly,
// <=15 Marginally, <=20 Weakly, else Very Weakly.
// R34 leave test: rng()*200 < ChanceToLeave (strict).
TEST(leave_describe_thresholds_and_strict_roll) {
    CHECK(mo::companionLeaveDescribe(0.0) == "Completely");
    CHECK(mo::companionLeaveDescribe(0.5) == "Excellently");
    CHECK(mo::companionLeaveDescribe(3.0) == "Excellently");
    CHECK(mo::companionLeaveDescribe(3.5) == "Fairly");
    CHECK(mo::companionLeaveDescribe(10.0) == "Fairly");
    CHECK(mo::companionLeaveDescribe(15.0) == "Marginally");
    CHECK(mo::companionLeaveDescribe(20.0) == "Weakly");
    CHECK(mo::companionLeaveDescribe(20.5) == "Very Weakly");

    mo::Companion c;
    tsup::Env zero({0.0, 0.0});
    CHECK(!mo::companionLeaveRoll(zero.ctx, c, 0.0)); // 0 < 0 is false
    tsup::Env low({0.0, 0.0});
    CHECK(mo::companionLeaveRoll(low.ctx, c, 7.5));
    tsup::Env high({0.5, 0.5});
    CHECK(!mo::companionLeaveRoll(high.ctx, c, 7.5)); // 100 < 7.5 is false
}

// R37.1: carry choice open only with nothing carried (-1) or a companion (0 to 3).
// R37.3: pickup if Monsters(Ref).Size - Race.Size <= 1.
TEST(carry_choice_open_and_pick_test) {
    CHECK(mo::carryChoiceOpen(mo::kCarryNone));
    CHECK(mo::carryChoiceOpen(0));
    CHECK(mo::carryChoiceOpen(3));
    CHECK(!mo::carryChoiceOpen(4));
    CHECK(!mo::carryChoiceOpen(7));

    CHECK(mo::carryPickTest(3, 2));
    CHECK(!mo::carryPickTest(4, 2));
    CHECK(mo::carryPickTest(2, 1));
    CHECK(!mo::carryPickTest(3, 1));
}

// R37.2: candidates are occupied slots at or below 0 Hits. The first is taken.
// A later one replaces it only with strictly greater MaxHits. Ties keep the earlier slot.
TEST(carry_candidate_first_then_strictly_greater_max_hits) {
    mo::Arr<mo::Companion, mo::kCompanionSlots> slots{};
    mo::Arr<int, mo::kCompanionSlots> sizes{};
    slots[0].occupied = true;
    slots[0].hits = 0;
    slots[0].maxHits = 10;
    slots[1].occupied = true;
    slots[1].hits = 0;
    slots[1].maxHits = 10;
    slots[2].occupied = true;
    slots[2].hits = 3;
    slots[2].maxHits = 50;
    slots[3].occupied = true;
    slots[3].hits = 0;
    slots[3].maxHits = 12;
    CHECK_EQ(mo::pickCarryCandidate(slots, sizes), 3);

    slots[3].maxHits = 10;
    CHECK_EQ(mo::pickCarryCandidate(slots, sizes), 0);
}

// R44: a new companion fills the first hole and keeps the name left in that slot.
TEST(new_companion_fills_first_hole_and_keeps_old_name) {
    mo::Arr<mo::Companion, mo::kCompanionSlots> slots{};
    slots[0].occupied = true;
    slots[1].occupied = true;
    slots[3].occupied = true;
    slots[2].occupied = false;
    slots[2].name = "Old";
    CHECK_EQ(mo::firstCompanionHole(slots), 2);

    tsup::Env env({0.5, 0.5});
    mo::Companion incoming;
    incoming.occupied = true;
    incoming.name = "New";
    CHECK_EQ(mo::placeNewCompanion(env.ctx, slots, incoming), 2);
    CHECK(slots[2].occupied);
    CHECK(slots[2].name == "Old");

    mo::Companion prev;
    prev.name = "Ulf";
    CHECK(mo::keepSlotName(prev) == "Ulf");
}

// R42: raise chance = min(98, 85 + CON). Offer the first slot with Hits == 0.
// Roll Next(100) > chance fails. 85 is not above 85, so 0.855 succeeds and
// 0.865 (86) fails. Failure releases the slot. Carry is always cleared.
TEST(morgue_chance_offer_and_raise_outcomes) {
    CHECK_EQ(mo::morgueRaiseChance(0), 85);
    CHECK_EQ(mo::morgueRaiseChance(10), 95);
    CHECK_EQ(mo::morgueRaiseChance(13), 98);
    CHECK_EQ(mo::morgueRaiseChance(40), 98);

    mo::Arr<mo::Companion, mo::kCompanionSlots> slots{};
    slots[0].occupied = true;
    slots[0].hits = 5;
    slots[0].maxHits = 9;
    slots[1].occupied = true;
    slots[1].hits = 0;
    slots[1].maxHits = 12;
    slots[2].occupied = true;
    slots[2].hits = 0;
    slots[2].maxHits = 9;
    CHECK_EQ(mo::morgueOfferSlot(slots), 1);

    mo::Character owner;
    mo::Inventory inv;
    inv.carryIndex = 2;
    tsup::Env ok({0.855, 0.855});
    CHECK(mo::raiseCompanionAtMorgue(ok.ctx, slots, owner, inv, 0) == mo::MorgueRaiseOutcome::Raised);
    CHECK_EQ(slots[1].hits, 12);
    CHECK_EQ(inv.carryIndex, -1);

    slots[1].hits = 0;
    slots[1].occupied = true;
    inv.carryIndex = 2;
    tsup::Env bad({0.865, 0.865});
    CHECK(mo::raiseCompanionAtMorgue(bad.ctx, slots, owner, inv, 0) == mo::MorgueRaiseOutcome::Failed);
    CHECK(!slots[1].occupied);
    CHECK_EQ(inv.carryIndex, -1);
}

// R46: drag swap is refused if either end is dead. Otherwise the two companions swap.
TEST(drag_swap_refuses_dead_and_swaps_live) {
    mo::Arr<mo::Companion, mo::kCompanionSlots> slots{};
    slots[0].occupied = true;
    slots[0].hits = 0;
    slots[0].isDead = true;
    slots[0].name = "A";
    slots[1].occupied = true;
    slots[1].hits = 5;
    slots[1].name = "B";

    tsup::Env env({0.5, 0.5});
    CHECK(mo::dragSwapCompanions(env.ctx, slots, 0, 1) == mo::DragSwapOutcome::RefusedDead);
    CHECK(slots[0].name == "A");

    slots[0].hits = 4;
    slots[0].isDead = false;
    CHECK(mo::dragSwapCompanions(env.ctx, slots, 0, 1) == mo::DragSwapOutcome::Swapped);
    CHECK(slots[0].name == "B");
    CHECK(slots[1].name == "A");
}

// R39: Kick Out is refused if dead or if the party is in the city.
// Binding > 0 earns a warning. R40: right-click is ignored at Hits <= 0.
// Refused if both Good. Otherwise the companion is let go.
TEST(kick_out_and_right_click_rules) {
    mo::Companion c;
    c.hits = 10;
    c.bindingLevel = -10;
    mo::KickOutCheck ok = mo::kickOutCheck(c, false);
    CHECK(ok.allowed);
    CHECK(ok.refusal == mo::KickOutRefusal::None);
    CHECK(!ok.warnBound);

    mo::KickOutCheck city = mo::kickOutCheck(c, true);
    CHECK(!city.allowed);
    CHECK(city.refusal == mo::KickOutRefusal::City);

    mo::Companion dead = c;
    dead.hits = 0;
    dead.isDead = true;
    CHECK(mo::kickOutCheck(dead, false).refusal == mo::KickOutRefusal::Dead);

    mo::Companion bound = c;
    bound.bindingLevel = 1;
    mo::KickOutCheck warn = mo::kickOutCheck(bound, false);
    CHECK(warn.allowed);
    CHECK(warn.warnBound);

    mo::Companion r;
    r.hits = 0;
    r.alignment = mo::CompanionAlignment::Good;
    CHECK(mo::rightClickAttackCheck(r, mo::Align::Good) == mo::RightClickOutcome::Ignored);
    r.hits = 5;
    CHECK(mo::rightClickAttackCheck(r, mo::Align::Good) == mo::RightClickOutcome::Refused);
    CHECK(mo::rightClickAttackCheck(r, mo::Align::Evil) == mo::RightClickOutcome::LetGo);
    r.alignment = mo::CompanionAlignment::Evil;
    CHECK(mo::rightClickAttackCheck(r, mo::Align::Good) == mo::RightClickOutcome::LetGo);
}

// R49: Control needs Int 12, Wis 12, Cha 10. Bind needs 14, 14, 16.
// Domination needs 18 in Int, Wis, Con and Cha. Bind spells skip free joiners.
TEST(bind_requirements_and_free_joiners) {
    mo::Character c;
    c.abilities.stats[1] = 12;  // Int
    c.abilities.stats[2] = 12;  // Wis
    c.abilities.stats[4] = 10;  // Cha
    CHECK(mo::bindSpellMeetsRequirements(c, mo::BindSpell::Control));
    c.abilities.stats[4] = 9;
    CHECK(!mo::bindSpellMeetsRequirements(c, mo::BindSpell::Control));

    c.abilities.stats[1] = 14;
    c.abilities.stats[2] = 14;
    c.abilities.stats[4] = 16;
    CHECK(mo::bindSpellMeetsRequirements(c, mo::BindSpell::Bind));
    c.abilities.stats[4] = 15;
    CHECK(!mo::bindSpellMeetsRequirements(c, mo::BindSpell::Bind));

    c.abilities.stats[1] = 18;
    c.abilities.stats[2] = 18;
    c.abilities.stats[3] = 18;  // Con
    c.abilities.stats[4] = 18;
    CHECK(mo::bindSpellMeetsRequirements(c, mo::BindSpell::Domination));
    c.abilities.stats[3] = 17;
    CHECK(!mo::bindSpellMeetsRequirements(c, mo::BindSpell::Domination));

    mo::Companion free;
    free.joinedFreely = true;
    CHECK(!mo::bindSpellApplies(free));
    free.joinedFreely = false;
    CHECK(mo::bindSpellApplies(free));
}

// R36: turn check. Gap of 2 always turns. Binding <= 0 never turns unless forced.
// A free group slot is needed. A fight refuses a creature with a gap of 2 from Foes[0].
// Neutral creature under Neutral master turns on rnd2*100 < 15.
TEST(turn_check_gap_binding_roll_and_refusals) {
    mo::Companion c;
    c.bindingLevel = 5;
    mo::Vec<mo::Creature> none;
    tsup::Env env({0.9, 0.9});

    CHECK(mo::companionTurnCheck(env.ctx, c, mo::Align::Good, mo::CharAlign::Evil, false, none, true)
          == mo::TurnOutcome::Joins);
    CHECK(mo::companionTurnCheck(env.ctx, c, mo::Align::Good, mo::CharAlign::Good, false, none, true)
          == mo::TurnOutcome::Holds);
    CHECK(mo::companionTurnCheck(env.ctx, c, mo::Align::Good, mo::CharAlign::Evil, false, none, false)
          == mo::TurnOutcome::NoGroupSlot);

    mo::Creature foe;
    foe.align = mo::CharAlign::Good;
    foe.hits = 5;
    mo::Vec<mo::Creature> foes;
    foes.push_back(foe);
    CHECK(mo::companionTurnCheck(env.ctx, c, mo::Align::Good, mo::CharAlign::Evil, false, foes, true)
          == mo::TurnOutcome::RefusedByFoe);

    c.bindingLevel = -10;
    CHECK(mo::companionTurnCheck(env.ctx, c, mo::Align::Good, mo::CharAlign::Evil, false, none, true)
          == mo::TurnOutcome::Holds);
    CHECK(mo::companionTurnCheck(env.ctx, c, mo::Align::Good, mo::CharAlign::Good, true, none, true)
          == mo::TurnOutcome::Joins);

    c.bindingLevel = 5;
    tsup::Env neutralHit({0.9, 0.1});  // rnd2*100 = 10 < 15
    CHECK(mo::companionTurnCheck(neutralHit.ctx, c, mo::Align::Neutral, mo::CharAlign::Neutral,
                                 false, none, true) == mo::TurnOutcome::Joins);
    tsup::Env neutralMiss({0.9, 0.2});  // rnd2*100 = 20 is not below 15
    CHECK(mo::companionTurnCheck(neutralMiss.ctx, c, mo::Align::Neutral, mo::CharAlign::Neutral,
                                 false, none, true) == mo::TurnOutcome::Holds);
}
