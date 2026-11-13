#include "test_support.h"
#include "mordor/casting1.h"

#include <cmath>

// Tests derived from SRD c09p1 (R1-R38). Only header-declared functions are called.
// Functions whose rules are not in this SRD part (breath, monster spells, map wrap,
// effect ids, item-only levels) are intentionally not tested.

TEST(guild_spell_level_follows_formula_and_clamps) {
    // R21: SL = clamp(Int(guildLevel/2 + 0.5), 1, 255)
    CHECK_EQ(mordor::guildSpellLevel(0), 1);
    CHECK_EQ(mordor::guildSpellLevel(1), 1);
    CHECK_EQ(mordor::guildSpellLevel(2), 1);
    CHECK_EQ(mordor::guildSpellLevel(3), 2);
    CHECK_EQ(mordor::guildSpellLevel(4), 2);
    CHECK_EQ(mordor::guildSpellLevel(19), 10);
    CHECK_EQ(mordor::guildSpellLevel(20), 10);
    CHECK_EQ(mordor::guildSpellLevel(600), 255);
}

TEST(reach_teleport_example_needs_guild_level_23) {
    // R27 / R23: Teleport level 12, Wizard rate 1. GL >= 2*12 - 1 = 23.
    CHECK(!mordor::guildReachesSpell(12, 1.0f, 22));
    CHECK(mordor::guildReachesSpell(12, 1.0f, 23));
}

TEST(reach_requires_positive_rate_and_inclusive_boundary) {
    // R23: rate must be above 0. Charm of Opening (level 1) at rate 3:
    // GL 4 gives half = 2.5 < 3 (no); GL 5 gives half = 3.0 and 3 <= 3 (yes).
    CHECK(!mordor::guildReachesSpell(1, 3.0f, 4));
    CHECK(mordor::guildReachesSpell(1, 3.0f, 5));
    CHECK(!mordor::guildReachesSpell(1, 0.0f, 50));
}

TEST(shown_spell_level_floors_exact_product) {
    // R25: spellLevelShown = floor(level * rate)
    CHECK_EQ(mordor::shownSpellLevel(1, 12.0f), 12);  // Nomad rate 12
    CHECK_EQ(mordor::shownSpellLevel(1, 3.0f), 3);    // Thief rate 3
    CHECK_EQ(mordor::shownSpellLevel(5, 2.5f), 12);   // 12.5 floors to 12
}

TEST(rated_spell_level_rounds_half_to_even) {
    // R28: rated = Round_half_even(level * rate)
    CHECK_EQ(mordor::ratedSpellLevel(1, 2.5f), 2);  // 2.5 -> 2 (even)
    CHECK_EQ(mordor::ratedSpellLevel(3, 2.5f), 8);  // 7.5 -> 8 (even)
    CHECK_EQ(mordor::ratedSpellLevel(4, 1.0f), 4);
}

TEST(published_cost_floors_fractional_product) {
    // R29: cost = Int(rated/SL * ratio * rate * 1.9 - eps)
    // GuildLevel 1 -> SL 1; rated 1; 1 * 33 * 1.0 * 1.9 = 62.7 -> 62
    CHECK_EQ(mordor::spellCostPublished(1, 33, 1.0f, 1), 62);
    // GuildLevel 5 -> SL 3; rated 3; 3/3 * 33 * 1.9 = 62.7 -> 62
    CHECK_EQ(mordor::spellCostPublished(3, 33, 1.0f, 5), 62);
}

TEST(published_cost_whole_product_is_reduced_by_one) {
    // R28/R29: whole exact product gives one less.
    // GuildLevel 5 -> SL 3; level 6 rated 6; 6/3 = 2; 2 * 10 * 1.0 * 1.9 = 38 (whole) -> 37
    CHECK_EQ(mordor::spellCostPublished(6, 10, 1.0f, 5), 37);
}

TEST(archive_minimum_and_initial_cost) {
    // R31: minimum cost = StartSpellLevel * BaseMod
    CHECK_EQ(mordor::archiveMinimumCost(4, 2.5f), 10);
    // R31: Initial = floor(ratio * baseMod * 1.9 - 0.0001); 10 * 1.0 * 1.9 = 19 -> 18
    CHECK_EQ(mordor::archiveInitialCost(10.0f, 1.0f), 18);
}

TEST(archive_spell_cost_desired_level_and_guild_equivalent) {
    // R31: Spell Cost = floor(start * ratio * mod^2 * 1.9 / SL - eps)
    // 2 * 10 * 1 * 1.9 / 4 = 9.5 -> 9
    CHECK_EQ(mordor::archiveSpellCost(2, 10.0f, 1.0f, 4), 9);
    // R31: level for desired cost = ceil(start * ratio * mod^2 * 1.9 / (desired + 1))
    // 38 / 10 = 3.8 -> 4
    CHECK_EQ(mordor::archiveLevelForDesiredCost(2, 10.0f, 1.0f, 9), 4);
    // R31: guild level equivalent = SL * 2 - 1
    CHECK_EQ(mordor::archiveGuildLevelEquivalent(4), 7);
}

TEST(sheet_common_initial_and_minimum_cost) {
    // R33 D: ROUNDDOWN(Q*R*1.9 - 0.0001, 0); Q 1.0, R 10 -> 18
    CHECK_EQ(mordor::sheetCommonInitialCost(1.0f, 10.0f), 18);
    // R33 K: ROUNDUP(C*Q*R*1.9/(C+1), 0)*2 - 1; C 2: 38/3 = 12.67 -> 13 -> 25
    CHECK_EQ(mordor::sheetCommonMinCost(2, 1.0f, 10.0f), 25);
}

TEST(sheet_common_cost_floors_at_initial_level) {
    // R33 E: MAX(ROUNDDOWN(C*Q*R*1.9/SL - eps, 0), C)
    // C 5, SL 20: 5*10*1.9/20 = 4.75 -> 4, below C -> 5
    CHECK_EQ(mordor::sheetCommonCost(5, 1.0f, 10.0f, 20), 5);
    // C 2, SL 4: 38/4 = 9.5 -> 9, above C -> 9
    CHECK_EQ(mordor::sheetCommonCost(2, 1.0f, 10.0f, 4), 9);
}

TEST(sorcerer_formula_j_uses_log_base_ten) {
    // R34: J = G + G*LN(C2)/LN(10) + INT((C2/10)*(H-1))
    // G 10, H 3, C2 10: 10 + 10*1 + INT(1*2) = 22
    double j = mordor::sorcererFormulaJ(10, 3, 10.0);
    CHECK(std::fabs(j - 22.0) < 1e-9);
    // R34: J grows more slowly than I for C2 > 1 (log base 5 gives larger term)
    double i = mordor::sorcererFormulaI(10, 3, 10.0);
    CHECK(i > j);
}

TEST(charm_opening_initial_and_minimum_cost) {
    // R35 SC_init: floor(E4 * SL_init * 1.9 - eps); 2 * 10.5 * 1.9 = 39.9 -> 39
    CHECK_EQ(mordor::charmOpeningInitialCost(2, 10.5f), 39);
    // R35 Min Cost: ceil(E4 * SL^2 * 1.9 / (SL+1)) * 2 - 1
    // 10.5 * 4 * 1.9 = 79.8; /3 = 26.6 -> ceil 27 -> 53
    CHECK_EQ(mordor::charmOpeningMinCost(2, 10.5f), 53);
}

TEST(charm_opening_sheet_cost_clamps_and_uses_squared_level) {
    // R35: raw = floor(E4 * SL_init^2 * 1.9 / C2 - eps); Spell Cost = SL_init if raw < SL_init
    // E4 1.0, SL 5, C2 29: 25 * 1.9 / 29 = 1.64 -> raw 1 < 5 -> 5
    CHECK_EQ(mordor::charmOpeningSheetCost(5, 1.0f, 29.0), 5);
    // E4 10.5, SL 5, C2 29: 10.5 * 25 * 1.9 / 29 = 17.20 -> 17
    CHECK_EQ(mordor::charmOpeningSheetCost(5, 10.5f, 29.0), 17);
}

TEST(cast_gate_spell_points_checked_first) {
    // R5 row 1 / R6 step 1: cost greater than SP refuses; equal proceeds.
    mordor::Character caster;
    caster.vitals.curSpellPoints = 5;
    mordor::CastSituation none;
    CHECK(mordor::checkCastGate(caster, 6, mordor::SpellUse::NonCombat, 1, none)
          == mordor::CastRefusal::SpellPoints);
    CHECK(mordor::checkCastGate(caster, 5, mordor::SpellUse::NonCombat, 1, none)
          == mordor::CastRefusal::None);
    // Even with an engaged NonCombat fight, the SP check comes first.
    mordor::CastSituation fight;
    fight.monstersPresent = true;
    fight.engaged = true;
    CHECK(mordor::checkCastGate(caster, 6, mordor::SpellUse::NonCombat, 1, fight)
          == mordor::CastRefusal::SpellPoints);
}

TEST(cast_gate_noncombat_refused_only_when_engaged) {
    // R6 step 2 / R14: monsters + engaged + NonCombat refuses.
    mordor::Character caster;
    caster.vitals.curSpellPoints = 10;
    mordor::CastSituation fight;
    fight.monstersPresent = true;
    fight.engaged = true;
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::NonCombat, 1, fight)
          == mordor::CastRefusal::NonCombat);
    // Either and Combat are not refused by row 2.
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Either, 1, fight)
          == mordor::CastRefusal::None);
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Combat, 1, fight)
          == mordor::CastRefusal::None);
}

TEST(cast_gate_antimagic_precedes_combat_only) {
    // R6 step 3 comes before step 4a.
    mordor::Character caster;
    caster.vitals.curSpellPoints = 10;
    mordor::CastSituation anti;
    anti.antiMagicSquare = true;
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Combat, 1, anti)
          == mordor::CastRefusal::AntiMagic);
    // Anti-magic with an engaged NonCombat fight: row 2 comes first.
    mordor::CastSituation antiFight = anti;
    antiFight.monstersPresent = true;
    antiFight.engaged = true;
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::NonCombat, 1, antiFight)
          == mordor::CastRefusal::NonCombat);
}

TEST(cast_gate_combat_only_without_monsters) {
    // R6 step 4a: no monsters, Combat spell refused; Either is not refused here.
    mordor::Character caster;
    caster.vitals.curSpellPoints = 10;
    mordor::CastSituation empty;
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Combat, 1, empty)
          == mordor::CastRefusal::CombatOnly);
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Either, 1, empty)
          == mordor::CastRefusal::None);
}

TEST(cast_gate_too_long_peaced_boundary) {
    // R6 step 4b: monsters present, not engaged, Level > 7 refuses.
    mordor::Character caster;
    caster.vitals.curSpellPoints = 10;
    mordor::CastSituation peaced;
    peaced.monstersPresent = true;
    peaced.engaged = false;
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Combat, 8, peaced)
          == mordor::CastRefusal::TooLongPeaced);
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Combat, 7, peaced)
          == mordor::CastRefusal::None);
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Either, 8, peaced)
          == mordor::CastRefusal::TooLongPeaced);
    // NonCombat is not in step 4 at all.
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::NonCombat, 8, peaced)
          == mordor::CastRefusal::None);
    // Engaged exempts the spell from TooLongPeaced.
    mordor::CastSituation engaged = peaced;
    engaged.engaged = true;
    CHECK(mordor::checkCastGate(caster, 1, mordor::SpellUse::Combat, 8, engaged)
          == mordor::CastRefusal::None);
}

TEST(peaced_state_and_item_slot_clearing) {
    // R7: peaced = monsters present, not engaged.
    mordor::CastSituation s;
    s.monstersPresent = true;
    s.engaged = false;
    CHECK(mordor::isPeaced(s));
    s.engaged = true;
    CHECK(!mordor::isPeaced(s));
    mordor::CastSituation none;
    CHECK(!mordor::isPeaced(none));
    // R5: the four refusals clear the item slot; SpellPoints does not.
    CHECK(mordor::refusalClearsItemSlot(mordor::CastRefusal::NonCombat));
    CHECK(mordor::refusalClearsItemSlot(mordor::CastRefusal::AntiMagic));
    CHECK(mordor::refusalClearsItemSlot(mordor::CastRefusal::CombatOnly));
    CHECK(mordor::refusalClearsItemSlot(mordor::CastRefusal::TooLongPeaced));
    CHECK(!mordor::refusalClearsItemSlot(mordor::CastRefusal::SpellPoints));
    CHECK(!mordor::refusalClearsItemSlot(mordor::CastRefusal::None));
}

TEST(unfit_gate_applies_only_to_level_zero_buffer_or_item) {
    // R3: runs for buffers/items with item level 0; any non-zero level skips it.
    CHECK(mordor::unfitGateApplies(true, 0));
    CHECK(!mordor::unfitGateApplies(true, 3));
    CHECK(!mordor::unfitGateApplies(true, -1));
    CHECK(!mordor::unfitGateApplies(false, 0));
}

TEST(cast_disposition_becomes_order_rules) {
    // R13: BecomesOrder if monsters present and (fight on or not NonCombat).
    using D = mordor::CastDisposition;
    using U = mordor::SpellUse;
    CHECK(mordor::castDisposition(false, false, U::Combat) == D::CastNow);
    CHECK(mordor::castDisposition(false, true, U::Combat) == D::CastNow);
    CHECK(mordor::castDisposition(true, false, U::NonCombat) == D::CastNow);
    CHECK(mordor::castDisposition(true, false, U::Combat) == D::BecomesOrder);
    CHECK(mordor::castDisposition(true, false, U::Either) == D::BecomesOrder);
    CHECK(mordor::castDisposition(true, true, U::NonCombat) == D::BecomesOrder);
}

TEST(standing_spell_order_falls_to_fight_keeping_target) {
    // R15: a standing spell order whose caster lacks SP becomes Fight, keeping target.
    mordor::CombatOrder order;
    order.kind = mordor::CombatOrderKind::Buffer;
    order.target = 3;
    order.spellId = 9;
    mordor::enforceStandingOrderSp(order, 2, 5);
    CHECK(order.kind == mordor::CombatOrderKind::Fight);
    CHECK_EQ(order.target, 3);

    // Enough SP (equal counts as affordable): order unchanged.
    mordor::CombatOrder kept;
    kept.kind = mordor::CombatOrderKind::Buffer;
    kept.target = 4;
    kept.spellId = 9;
    mordor::enforceStandingOrderSp(kept, 5, 5);
    CHECK(kept.kind == mordor::CombatOrderKind::Buffer);
    CHECK_EQ(kept.target, 4);

    // Cost-0 spell is always affordable, even with zero SP.
    mordor::CombatOrder free;
    free.kind = mordor::CombatOrderKind::Buffer;
    free.target = 6;
    free.spellId = 9;
    mordor::enforceStandingOrderSp(free, 0, 0);
    CHECK(free.kind == mordor::CombatOrderKind::Buffer);
    CHECK_EQ(free.target, 6);
}

TEST(points_spent_only_after_target_and_not_on_teleport_death) {
    // R16: spend only if target chosen and not killed by teleport.
    CHECK(!mordor::spendsCastPoints(false, false));
    CHECK(mordor::spendsCastPoints(true, false));
    CHECK(!mordor::spendsCastPoints(true, true));

    // payCastCost subtracts the cost from current SP.
    mordor::Character caster;
    caster.vitals.curSpellPoints = 10;
    mordor::payCastCost(caster, 4);
    CHECK_EQ(caster.vitals.curSpellPoints, 6);
}

TEST(item_charge_spent_only_on_completed_cast) {
    // R19: a charge is spent only on a completed cast; one charge per cast.
    mordor::Item item;
    item.charges = 3;
    mordor::spendItemCharge(item, false);
    CHECK_EQ(item.charges, 3);
    mordor::spendItemCharge(item, true);
    CHECK_EQ(item.charges, 2);
}

TEST(cast_roll_is_int_of_rnd_times_100) {
    // R12: l01D0 = Int(Rnd * 100), drawn once per passing cast.
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(mordor::drawCastRoll(env.ctx), 50);
    CHECK_EQ(mordor::drawCastRoll(env.ctx), 90);
}
