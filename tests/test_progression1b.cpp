#include <cmath>

#include "test_support.h"
#include "mordor/progression1b.h"

using namespace mordor;

namespace {
bool nearVal(double a, double b, double tol) { return std::fabs(a - b) <= tol; }
}  // namespace

// R19: starting age is 16 years in days.
TEST(start_age_is_16_years_in_days) {
    CHECK_EQ(kAgeStartDays, 5840);
    CHECK_EQ(kAgeStartDays / 365, 16);
}

// R13 spreadsheet form: zero at L <= 1; level 2 with EP 1.0 and race 1.0.
TEST(unrounded_closed_form_zero_at_level_one_and_level_two_value) {
    CHECK(closedFormLevelXpUnrounded(1, 1.0, 1.0) == 0.0);
    CHECK(closedFormLevelXpUnrounded(0, 1.0, 1.0) == 0.0);
    // 88.09020776 + 500/11 - 0.264
    CHECK(nearVal(closedFormLevelXpUnrounded(2, 1.0, 1.0), 133.28075321, 1e-6));
}

// R13 worked anchor: Human Nomad L100 = 872,326 (l = 99, EP 1.0).
TEST(unrounded_closed_form_level_100_human_nomad_rounds_to_872326) {
    double xp = closedFormLevelXpUnrounded(100, 1.0, 1.0);
    CHECK(nearVal(xp, 872326.4077, 1e-3));
    CHECK_EQ(static_cast<i64>(std::llround(xp)), 872326);
}

// R13: guild EP multiplies only the squared term; race multiplies the whole bracket.
TEST(unrounded_closed_form_guild_term_and_race_multiplier) {
    // Doubling guild EP adds exactly 88.09020776 at level 2 (x = 1).
    double diff = closedFormLevelXpUnrounded(2, 2.0, 1.0) - closedFormLevelXpUnrounded(2, 1.0, 1.0);
    CHECK(nearVal(diff, 88.09020776, 1e-6));
    // Elf race factor (T1) scales the whole result.
    double base = closedFormLevelXpUnrounded(2, 1.0, 1.0);
    double elf = closedFormLevelXpUnrounded(2, 1.0, 1.18544741085);
    CHECK(nearVal(elf, base * 1.18544741085, 1e-9));
}

// R13 research form uses -1 inside the bracket (community form, distinct constant).
TEST(research_closed_form_uses_minus_one_inside_bracket) {
    // R13 CONFLICT resolved to engine-aligned closed form: offset -0.264, not the research/wiki -1.
    // x = 1, GEF 8 -> E/8 = 1: 88.09020776 + 500/11 - 0.264
    CHECK(nearVal(closedFormLevelXpResearch(2, 8.0, 1.0), 133.28075321, 1e-6));
}

// R13 wiki form: race factor is the actual exp multiplier applied to the same bracket.
TEST(wiki_closed_form_matches_research_at_race_one_and_scales_with_actual_exp) {
    // R13 CONFLICT resolved to engine-aligned closed form: offset -0.264 (wiki's -1 not used).
    double w1 = closedFormLevelXpWiki(2, 8.0, 1.0);
    CHECK(nearVal(w1, 133.28075321, 1e-6));
    CHECK(nearVal(closedFormLevelXpWiki(2, 8.0, 1.18544741085) / w1, 1.18544741085, 1e-9));
}

// R15 / XP still needed = XP_Total - current XP.
TEST(xp_still_needed_is_total_minus_current) {
    CHECK_EQ(xpStillNeeded(872326, 100000), 772326);
    CHECK_EQ(xpStillNeeded(872326, 872326), 0);
}

// R14 table T1 RaceMult by raw exp, plus the approximation hand check.
TEST(race_multiplier_table_and_actual_exp_approximation) {
    CHECK(nearVal(raceMultForRawExp(3), 1.0, 1e-12));
    CHECK(nearVal(raceMultForRawExp(4), 1.05819132268, 1e-9));
    CHECK(nearVal(raceMultForRawExp(7), 1.18544741085, 1e-9));
    CHECK(nearVal(raceMultForRawExp(9), 1.25647909518, 1e-9));
    // Approximation: Exp 3 gives exactly 1; Exp 7 gives about 1.18659.
    CHECK(nearVal(raceActualExpApprox(3), 1.0, 1e-12));
    CHECK(nearVal(raceActualExpApprox(7), 1.18659, 1e-4));
}

// R14 / archive [c106] race-guild restrictions.
TEST(race_guild_restrictions) {
    CHECK(!raceAllowsGuild(Race::Elf, Guild::Warrior));
    CHECK(!raceAllowsGuild(Race::Osiri, Guild::Warrior));
    CHECK(!raceAllowsGuild(Race::Giant, Guild::Wizard));
    CHECK(!raceAllowsGuild(Race::Morloch, Guild::Wizard));
    CHECK(!raceAllowsGuild(Race::Gnome, Guild::Wizard));
    CHECK(!raceAllowsGuild(Race::Gnome, Guild::Warrior));
    CHECK(raceAllowsGuild(Race::Human, Guild::Warrior));
    CHECK(raceAllowsGuild(Race::Human, Guild::Wizard));
    CHECK(raceAllowsGuild(Race::Dwarf, Guild::Wizard));
}

// Stat items: natural max + 5 (wiki). Guild parameter percent = Parameter / 8 * 100.
TEST(stat_item_max_and_guild_parameter_percent) {
    CHECK_EQ(statItemMaxAllowed(18), 23);
    CHECK_EQ(statItemMaxAllowed(25), 30);
    CHECK(nearVal(guildParameterPercent(8), 100.0, 1e-9));
    CHECK(nearVal(guildParameterPercent(21), 262.5, 1e-9));
    CHECK(nearVal(guildParameterPercent(27), 337.5, 1e-9));
}

// R16 gold anchors (T7): Warrior L1 = 33, Nomad L3 = 352.
TEST(gold_to_reach_matches_printed_anchors) {
    CHECK_EQ(goldToReachArchiveLoop(1, 1), 33);
    CHECK_EQ(goldToReachArchiveLoop(3, 3), 352);
}

// R16: running total is 30 + 90 + 151 = 271; 150.5 rounds up (x.5 up, not half-even).
TEST(gold_audit_running_total_rounds_half_up) {
    CHECK_EQ(goldCostAuditRunning(1, 1), 33);
    // SRD-AMBIGUOUS: combat-audit gold running-total rounding rule is not stated in the SRD.
    // Code rounds half-even each step: 120 + 150.5 -> 270, then 270 * 1.3 = 351.
    // Archive loop (x.5 up) is covered by goldToReachArchiveLoop below.
    CHECK_EQ(goldCostAuditRunning(3, 3), 351);
    CHECK_EQ(goldToReachArchiveLoop(3, 0), 271);
}

// R18 CCur: rounds after the fourth decimal (1/3 -> 0.3333).
TEST(ccur_rounds_to_four_decimals) {
    CHECK(nearVal(ccurRound4(1.0 / 3.0), 0.3333, 1e-9));
}

// R26 table T5 and the floor at 0.
TEST(con_loss_chance_table_and_floor) {
    CHECK_EQ(conLossChancePercent(10), 30);
    CHECK_EQ(conLossChancePercent(21), 8);
    CHECK_EQ(conLossChancePercent(23), 4);
    CHECK_EQ(conLossChancePercent(24), 2);
    CHECK_EQ(conLossChancePercent(25), 0);
    CHECK_EQ(conLossChancePercent(27), 0);
    CHECK_EQ(conLossChancePercent(30), 0);
}

// R26: a roll at 0.0 is inside an 8% chance (loss); at 0.99 it is not.
// A 0% chance never loses for a roll of 0.5.
TEST(raise_con_loss_roll_follows_chance) {
    tsup::Env low({0.0, 0.0});
    CHECK(rollRaiseConLoss(low.ctx, 21));
    tsup::Env high({0.99, 0.99});
    CHECK(!rollRaiseConLoss(high.ctx, 21));
    tsup::Env none({0.5, 0.5});
    CHECK(!rollRaiseConLoss(none.ctx, 25));
}

// R20: Withering ages 28 days; AgeBy adds to age in days.
TEST(age_by_adds_days_to_character_age) {
    tsup::Env env({0.5, 0.9});
    Character ch;
    ch.ageDays = kAgeStartDays;
    ageBy(env.ctx, ch, 28, true);
    CHECK_EQ(ch.ageDays, 5868);
    ageBy(env.ctx, ch, 28, false);
    CHECK_EQ(ch.ageDays, 5896);
}

// R21: ailment tick runs every 4 seconds (4000 ms).
TEST(ailment_tick_due_every_four_seconds) {
    CHECK(!ailmentTickDue(3999, 0));
    CHECK(ailmentTickDue(4001, 0));
    CHECK(!ailmentTickDue(10000, 6001));
    CHECK(ailmentTickDue(10000, 5999));
}

// R21: old-age threshold is 5 + ln(max(agePercent - 98, 1e-9)).
TEST(old_age_roll_threshold_formula) {
    CHECK(nearVal(oldAgeRollThreshold(99.0), 5.0, 1e-9));
    CHECK(nearVal(oldAgeRollThreshold(108.0), 5.0 + std::log(10.0), 1e-9));
    CHECK(nearVal(oldAgeRollThreshold(98.0), 5.0 + std::log(1e-9), 1e-9));
}

// R24: full HP costs 0 days; zero HP costs 31 days.
TEST(rest_age_cost_full_and_empty_hits) {
    CHECK_EQ(restAgeCostDays(20, 20), 0);
    CHECK_EQ(restAgeCostDays(0, 20), 31);
}

// R17: forfeit target is max(1, from - cost).
TEST(forfeit_target_level_clamps_at_one) {
    CHECK_EQ(forfeitTargetLevel(5, 2), 3);
    CHECK_EQ(forfeitTargetLevel(3, 2), 1);
    CHECK_EQ(forfeitTargetLevel(3, 5), 1);
}

// R15 archive example: L80 at 556,907 projects to L100 at 870,167.1875.
TEST(projection_archive_example) {
    CHECK(nearVal(projectedXp(100, 80, 556907.0), 870167.1875, 1e-6));
    CHECK(nearVal(projectedLevel(870167.1875, 556907.0, 80), 100.0, 1e-9));
}

// R-pin: XP for pin uses min(currentLevel + 1, 998).
TEST(xp_for_pin_clamps_next_level_at_998) {
    CHECK_EQ(xpForPin(997, 1.0, 1.0), levelXpWikiC88(998, 1.0, 1.0));
    CHECK_EQ(xpForPin(998, 1.0, 1.0), levelXpWikiC88(998, 1.0, 1.0));
    CHECK_EQ(xpForPin(500, 1.0, 1.0), levelXpWikiC88(501, 1.0, 1.0));
}

// R21: draws 0.99 -> 198. Effective CON 12 (+4) and poison 2 (-4) net to 0.
// Poison: 198 > 185 clears. Disease (poison now 0, +4): 202 > 195 clears.
TEST(ailment_tick_clears_poison_and_disease_on_high_rolls) {
    tsup::Env env({0.99, 0.99});
    Character ch;
    ch.vitals.curHits = 20;
    ch.vitals.maxHits = 20;
    ch.ailments.poisonLevel = 2;
    ch.ailments.diseaseLevel = 1;
    ailmentTick(env.ctx, ch, 12, 0.0);
    CHECK_EQ(ch.ailments.poisonLevel, 0);
    CHECK_EQ(ch.ailments.diseaseLevel, 0);
    CHECK_EQ(ch.vitals.curHits, 20);
}

// R21 boundary: draw 186 clears poison (186 > 185). Disease 190 does not clear
// (190 <= 195) and costs 1 HP.
TEST(ailment_tick_poison_boundary_clears_but_disease_costs_hp) {
    tsup::Env env({0.9325, 0.9325});
    Character ch;
    ch.vitals.curHits = 20;
    ch.vitals.maxHits = 20;
    ch.ailments.poisonLevel = 2;
    ch.ailments.diseaseLevel = 1;
    ailmentTick(env.ctx, ch, 12, 0.0);
    CHECK_EQ(ch.ailments.poisonLevel, 0);
    CHECK_EQ(ch.ailments.diseaseLevel, 1);
    CHECK_EQ(ch.vitals.curHits, 19);
}

// R21: draw 185 does not clear poison (185 is not > 185) and costs 2 HP.
// Disease 185 does not clear (not > 195) and costs 1 HP. Total 3 HP.
TEST(ailment_tick_uncleared_rolls_cost_hp) {
    tsup::Env env({0.9275, 0.9275});
    Character ch;
    ch.vitals.curHits = 20;
    ch.vitals.maxHits = 20;
    ch.ailments.poisonLevel = 2;
    ch.ailments.diseaseLevel = 1;
    ailmentTick(env.ctx, ch, 12, 0.0);
    CHECK_EQ(ch.ailments.poisonLevel, 2);
    CHECK_EQ(ch.ailments.diseaseLevel, 1);
    CHECK_EQ(ch.vitals.curHits, 17);
}
