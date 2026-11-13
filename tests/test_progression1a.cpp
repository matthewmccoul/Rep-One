// Tests for progression1a: experience from blows and chests, XP thresholds,
// pin cap and toolbar, level-up hit and A/D helpers.
// Expected values are derived by hand from SRD chapter 5 (c05p1).
// Deliberately NOT tested (SRD marks Unknown): charm XP, rounding of B(.),
// abilityChance scale, acc/bonus meaning in the award step, guild hit/A-D
// rate values, gold deduction, drain mechanism, MaxLevel value.

#include <cmath>
#include "mordor/guilds1.h"

#include "test_support.h"
#include "mordor/progression1a.h"

using mordor::i64;

namespace {

mordor::GuildMembership& guildOf(mordor::Character& c, mordor::Guild g) {
    return c.guilds[static_cast<unsigned>(g)];
}

}  // namespace

// R1: inflicted = min(dmg, max(0, target.Hits))
TEST(inflicted_hits_is_min_of_damage_and_target_hits) {
    CHECK_EQ(mordor::inflictedHits(10, 4), 4);   // target has fewer hits
    CHECK_EQ(mordor::inflictedHits(3, 10), 3);   // blow does less than target hits
    CHECK_EQ(mordor::inflictedHits(7, 7), 7);    // exact kill
    CHECK_EQ(mordor::inflictedHits(0, 7), 0);    // no damage dealt
}

// R1: max(0, target.Hits) floors the target at zero
TEST(inflicted_hits_zero_when_target_has_no_hits) {
    CHECK_EQ(mordor::inflictedHits(5, 0), 0);
    CHECK_EQ(mordor::inflictedHits(5, -3), 0);
}

// R2 engine formula, h=1, monGL=0, curGL=0, party 1, no companion:
// xp = 1*(9-ln2)*(20-ln1)*(0+50)/1000 = 8.3069 -> 8
TEST(blow_xp_single_hit_low_monster) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowAward a = mordor::blowExperience(1, 0, 0, false, 1, -1);
    CHECK_EQ(a.xp, 8);
    CHECK(!a.creditOwner);
    CHECK_EQ(a.ownerIndex, -1);
}

// R2 engine formula, h=10, monGL=10, curGL=0, party 1:
// 10*(9-ln11)*(20-ln10)*60/1000 = 70.104 -> 70
TEST(blow_xp_ten_hits_monster_gl10) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowAward a = mordor::blowExperience(10, 10, 0, false, 1, -1);
    CHECK_EQ(a.xp, 70);
}

// Party factor 1 - (4-1)*0.04 = 0.88; 0.88 * 70 = 61.6 -> 62
TEST(blow_xp_party_of_four_applies_party_factor) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowAward a = mordor::blowExperience(10, 10, 0, false, 4, -1);
    CHECK_EQ(a.xp, 62);
}

// Party factor 1 - 1*0.04 = 0.96; 0.96 * 70 = 67.2 -> 67
TEST(blow_xp_party_of_two_applies_party_factor) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowAward a = mordor::blowExperience(10, 10, 0, false, 2, -1);
    CHECK_EQ(a.xp, 67);
}

// GuildLevelFactor = 1/(1 + 333/333) = 0.5; 70.104 * 0.5 = 35.052 -> 35
TEST(blow_xp_halved_at_guild_level_333) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowAward a = mordor::blowExperience(10, 10, 333, false, 1, -1);
    CHECK_EQ(a.xp, 35);
}

// CompanionFactor 0.25 and XP credited to owner: 70.104 * 0.25 = 17.526 -> 18
TEST(blow_xp_companion_hit_credits_owner_at_quarter_rate) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowAward a = mordor::blowExperience(10, 10, 0, true, 1, 2);
    CHECK_EQ(a.xp, 18);
    CHECK(a.creditOwner);
    CHECK_EQ(a.ownerIndex, 2);
}

// R2 step 3: award is zero when h = 0
TEST(blow_xp_zero_when_no_hits_inflicted) {
    tsup::Env env({0.5, 0.9});
    mordor::BlowAward a = mordor::blowExperience(0, 10, 0, false, 1, -1);
    CHECK_EQ(a.xp, 0);
}

// R4: chest XP is paid only when Fight.ChestTrap > 0
TEST(chest_xp_zero_when_no_trap_was_fought) {
    CHECK_EQ(mordor::chestTrapExperience(0, 50.0, 10, 0, 1), 0);
}

// R4: a zero disarm chance pays nothing (formula is linear in disarmChance)
TEST(chest_xp_zero_when_disarm_chance_zero) {
    CHECK_EQ(mordor::chestTrapExperience(1, 0.0, 10, 0, 1), 0);
}

// R4: monster guild level 0 gives monGL/2 = 0, so nothing is paid
TEST(chest_xp_zero_for_monster_guild_level_zero) {
    CHECK_EQ(mordor::chestTrapExperience(1, 50.0, 0, 0, 1), 0);
}

// curFixed(x) = Round_half_even(x * 10000); inputs chosen away from .5 ties
TEST(cur_fixed_scales_to_four_decimal_places) {
    CHECK_EQ(mordor::curFixed(1.0), 10000);
    CHECK_EQ(mordor::curFixed(3.0), 30000);
    CHECK_EQ(mordor::curFixed(1.23456), 12346);
}

// raceExpMultiplier(r) with r = 1: Ln(1)/Ln(2) + 1 - 0.7 + 1/25 = 0.34
TEST(race_multiplier_unit_race_is_0_34) {
    CHECK(std::fabs(mordor::raceExpMultiplier(1.0) - 0.34) < 1e-9);
}

// R12 with guildExp=1.0, raceExp=0 (so r=1, f=0.34), level 1:
// E=Cur(2)=2; base=((2/12)*500*1 + 344*1 - 2)/10 = 42.5333; *0.34 = 14.4613 -> 14
TEST(level_threshold_level_one_hand_computed) {
    CHECK_EQ(mordor::levelThresholdXp(1, 1.0, 0.0), 14);
}

// R12 with guildExp=1.0, raceExp=0, level 2:
// base=((4/12)*500*2 + 344*3 - 2)/10 = 136.3333; *0.34 = 46.3533 -> 46
TEST(level_threshold_level_two_hand_computed) {
    CHECK_EQ(mordor::levelThresholdXp(2, 1.0, 0.0), 46);
}

// capMaxHits: min(maxHits, 999)
TEST(cap_max_hits_clamps_to_999) {
    CHECK_EQ(mordor::capMaxHits(1000), 999);
    CHECK_EQ(mordor::capMaxHits(999), 999);
    CHECK_EQ(mordor::capMaxHits(998), 998);
    CHECK_EQ(mordor::capMaxHits(0), 0);
}

// R9: spell points refill on each level (refill to maximum)
TEST(refill_spell_points_restores_current_to_max) {
    mordor::Character ch;
    ch.vitals.maxSpellPoints = 12;
    ch.vitals.curSpellPoints = 3;
    mordor::refillSpellPoints(ch);
    CHECK_EQ(ch.vitals.curSpellPoints, 12);
    CHECK_EQ(ch.vitals.maxSpellPoints, 12);
}

// R10 award step with acc=0, bonus=0: ((1000/2) + 0) / (2 + 0) = 250
TEST(ad_award_step_formula_interval_two) {
    CHECK_EQ(mordor::adAwardStep(1000, 0, 2, 0), 250);
}

// ((1000/2) + 0) / (4 + 0) = 125
TEST(ad_award_step_formula_interval_four) {
    CHECK_EQ(mordor::adAwardStep(1000, 0, 4, 0), 125);
}

// R10: award on level Mod step = 0, and never above AdMaxLevel
TEST(ad_award_level_interval_and_ceiling) {
    CHECK(mordor::isAdAwardLevel(4, 2, 10));    // on interval, under ceiling
    CHECK(mordor::isAdAwardLevel(2, 2, 10));    // first award level
    CHECK(mordor::isAdAwardLevel(10, 2, 10));   // exactly at ceiling is allowed
    CHECK(!mordor::isAdAwardLevel(5, 2, 10));   // off interval
    CHECK(!mordor::isAdAwardLevel(1, 2, 10));   // level 1 is not a multiple of 2
    CHECK(!mordor::isAdAwardLevel(12, 2, 10));  // above AdMaxLevel
}

// R8 hit grant: no hits unless strictly ahead of every other guild.
// Archive example: level 26 with a level 30 guild present gains nothing.
// Default GuildHitsRef is used; with no hits granted the result is 0 regardless.
TEST(level_hit_gain_zero_when_not_ahead_of_other_guilds) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    mordor::GuildHitsRef rates{};
    for (auto& m : ch.guilds) {
        m.level = 30;
    }
    ch.guilds[static_cast<std::size_t>(mordor::Guild::Healer)].level = 25;
    auto before = ch.vitals.maxHits;
    mordor::levelUp(env.ctx, ch, mordor::Guild::Healer, rates);
    CHECK_EQ(ch.vitals.maxHits - before, 0);
}

// R8 tie rule: strict '>' means equal level to the highest other guild gains nothing
TEST(level_hit_gain_zero_on_tie_with_highest_other_guild) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    mordor::GuildHitsRef rates{};
    for (auto& m : ch.guilds) {
        m.level = 30;
    }
    ch.guilds[static_cast<std::size_t>(mordor::Guild::Healer)].level = 29;
    auto before = ch.vitals.maxHits;
    mordor::levelUp(env.ctx, ch, mordor::Guild::Healer, rates);
    CHECK_EQ(ch.vitals.maxHits - before, 0);
}

// R5: toolbar Pinned when exp == cap; amount is 0
TEST(toolbar_pinned_when_experience_equals_cap) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    guildOf(ch, mordor::Guild::Warrior).level = 1;
    i64 cap = mordor::experienceCap(env.ctx, ch, mordor::Guild::Warrior);
    guildOf(ch, mordor::Guild::Warrior).experience = cap;
    mordor::ToolbarNeed t = mordor::progressToolbar(env.ctx, ch, mordor::Guild::Warrior);
    CHECK_EQ(static_cast<int>(t.kind), static_cast<int>(mordor::ToolbarKind::Pinned));
    CHECK_EQ(t.amount, 0);
}

// Toolbar: exp below the next-level threshold shows Short with a positive amount
TEST(toolbar_short_with_no_experience) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    guildOf(ch, mordor::Guild::Warrior).level = 1;
    guildOf(ch, mordor::Guild::Warrior).experience = 0;
    mordor::ToolbarNeed t = mordor::progressToolbar(env.ctx, ch, mordor::Guild::Warrior);
    CHECK_EQ(static_cast<int>(t.kind), static_cast<int>(mordor::ToolbarKind::Short));
    CHECK(t.amount > 0);
}

// Toolbar: one XP below cap shows Ready with amount = cap - exp = 1
TEST(toolbar_ready_one_below_cap_shows_one_left) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    guildOf(ch, mordor::Guild::Warrior).level = 1;
    i64 cap = mordor::experienceCap(env.ctx, ch, mordor::Guild::Warrior);
    guildOf(ch, mordor::Guild::Warrior).experience = cap - 1;
    mordor::ToolbarNeed t = mordor::progressToolbar(env.ctx, ch, mordor::Guild::Warrior);
    CHECK_EQ(static_cast<int>(t.kind), static_cast<int>(mordor::ToolbarKind::Ready));
    CHECK_EQ(t.amount, 1);
}

// R3 step 3: total clamps to cap; counted gain is only the room left (3 of 10)
TEST(give_experience_clamps_gain_at_cap) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    guildOf(ch, mordor::Guild::Warrior).level = 1;
    i64 cap = mordor::experienceCap(env.ctx, ch, mordor::Guild::Warrior);
    guildOf(ch, mordor::Guild::Warrior).experience = cap - 3;
    i64 gained = mordor::giveExperience(env.ctx, ch, mordor::Guild::Warrior, 10);
    CHECK_EQ(gained, 3);
    CHECK(guildOf(ch, mordor::Guild::Warrior).experience == cap);
}

// R3 boundary: reaching the cap exactly counts the full award and pins
TEST(give_experience_exact_landing_on_cap_counts_full_award) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    guildOf(ch, mordor::Guild::Warrior).level = 1;
    i64 cap = mordor::experienceCap(env.ctx, ch, mordor::Guild::Warrior);
    guildOf(ch, mordor::Guild::Warrior).experience = cap - 10;
    i64 gained = mordor::giveExperience(env.ctx, ch, mordor::Guild::Warrior, 10);
    CHECK_EQ(gained, 10);
    CHECK(guildOf(ch, mordor::Guild::Warrior).experience == cap);
}

// R3 step 6 / R6: an already-pinned character gains nothing
TEST(give_experience_pinned_character_gains_nothing) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    guildOf(ch, mordor::Guild::Warrior).level = 1;
    i64 cap = mordor::experienceCap(env.ctx, ch, mordor::Guild::Warrior);
    guildOf(ch, mordor::Guild::Warrior).experience = cap;
    i64 gained = mordor::giveExperience(env.ctx, ch, mordor::Guild::Warrior, 10);
    CHECK_EQ(gained, 0);
    CHECK(guildOf(ch, mordor::Guild::Warrior).experience == cap);
}

// Payout path [c37] / port note: a total above a lowered pin is pulled back
// to the pin; the pull-back is not a counted loss and Amount stays at zero.
TEST(give_experience_pulls_total_back_to_pin_without_negative_amount) {
    tsup::Env env({0.5, 0.9});
    mordor::Character ch;
    guildOf(ch, mordor::Guild::Warrior).level = 1;
    i64 cap = mordor::experienceCap(env.ctx, ch, mordor::Guild::Warrior);
    guildOf(ch, mordor::Guild::Warrior).experience = cap + 50;
    i64 gained = mordor::giveExperience(env.ctx, ch, mordor::Guild::Warrior, 10);
    CHECK_EQ(gained, 0);
    CHECK(guildOf(ch, mordor::Guild::Warrior).experience == cap);
}

// Enum sanity from header: ToolbarKind values are Short=0, Ready=1, Pinned=2
TEST(toolbar_kind_enum_values_match_header) {
    CHECK_EQ(static_cast<int>(mordor::ToolbarKind::Short), 0);
    CHECK_EQ(static_cast<int>(mordor::ToolbarKind::Ready), 1);
    CHECK_EQ(static_cast<int>(mordor::ToolbarKind::Pinned), 2);
}
