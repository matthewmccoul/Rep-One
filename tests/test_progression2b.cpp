#include "test_support.h"
#include "mordor/progression2b.h"

// Expectations are derived from the SRD tables and rules for progression2b.
// Currency is fixed point at 10000 units per gold piece.
// Not covered, because the SRD leaves the behaviour UNKNOWN or the API is not indexed:
//   announceLevelThresholdCrossed (EventLog query API not in the index),
//   hitsGainForLevel (guild hit values UNKNOWN),
//   naturalDeathChancePercent / rollNaturalDeath (comparison direction UNKNOWN),
//   applySanctuary (target and amount UNKNOWN for the port),
//   guildJoinCost (ability and spell inputs UNKNOWN; table is archive-sourced).

using namespace mordor;

namespace {

void setGuildLevel(Character& ch, Guild g, i32 lvl) {
    ch.guilds[static_cast<u32>(g)].level = lvl;
}

} // namespace

// ---- Hit cap (SRD T10: MaxHits = min(MaxHits, 999)) ----

TEST(clamp_max_hits_caps_at_999) {
    CHECK_EQ(clampMaxHits(1000), 999);
    CHECK_EQ(clampMaxHits(5000), 999);
}

TEST(clamp_max_hits_leaves_values_at_or_below_cap) {
    CHECK_EQ(clampMaxHits(999), 999);
    CHECK_EQ(clampMaxHits(998), 998);
    CHECK_EQ(clampMaxHits(15), 15);
}

// ---- Rest age cost (SRD T9, rule B((100 - 100*cur/max)/3.25)) ----

TEST(rest_age_cost_is_zero_at_full_hp) {
    CHECK_EQ(restAgeCostDays(15, 15), 0);
    CHECK_EQ(restAgeCostDays(40, 40), 0);
}

TEST(rest_age_cost_at_zero_hp_is_31_days) {
    // (100 - 0) / 3.25 = 30.77, documented as 31 days.
    CHECK_EQ(restAgeCostDays(0, 15), 31);
}

TEST(rest_age_cost_rises_as_hp_falls) {
    i32 at80pct = restAgeCostDays(12, 15); // (100 - 80) / 3.25 = 6.15 scale, about 24.6
    i32 at47pct = restAgeCostDays(7, 15);  // (100 - 46.7) / 3.25 = about 16.4
    CHECK(at47pct > at80pct);
    CHECK(at47pct < 31);
}

// ---- Poison and disease clear thresholds (SRD T9) ----

TEST(poison_clear_requires_result_strictly_above_185) {
    CHECK(!poisonClearSucceeds(185));
    CHECK(poisonClearSucceeds(186));
    CHECK(poisonClearSucceeds(200));
}

TEST(disease_clear_requires_result_strictly_above_195) {
    CHECK(!diseaseClearSucceeds(195));
    CHECK(diseaseClearSucceeds(196));
    CHECK(diseaseClearSucceeds(200));
}

// ---- Old-age onset at 98% of racial lifespan (SRD T9) ----

TEST(old_age_onset_at_98_percent_of_lifespan) {
    // Values well away from the 9800 boundary, to avoid the >= / > ambiguity.
    CHECK(!isOldAgeOnset(9000, 10000));
    CHECK(isOldAgeOnset(9900, 10000));
    CHECK(isOldAgeOnset(10000, 10000));
}

// ---- Highest guild (strict comparison against every other guild) ----

TEST(ahead_of_every_guild_false_when_all_levels_zero) {
    Character ch;
    CHECK(!isAheadOfEveryGuild(ch, Guild::Warrior));
}

TEST(ahead_of_every_guild_true_for_sole_leader) {
    Character ch;
    setGuildLevel(ch, Guild::Warrior, 5);
    CHECK(isAheadOfEveryGuild(ch, Guild::Warrior));
    CHECK(!isAheadOfEveryGuild(ch, Guild::Paladin));
}

TEST(ahead_of_every_guild_false_on_tie) {
    Character ch;
    setGuildLevel(ch, Guild::Warrior, 5);
    setGuildLevel(ch, Guild::Paladin, 5);
    CHECK(!isAheadOfEveryGuild(ch, Guild::Warrior));
    CHECK(!isAheadOfEveryGuild(ch, Guild::Paladin));
}

TEST(ahead_of_every_guild_only_the_top_guild_qualifies) {
    Character ch;
    setGuildLevel(ch, Guild::Warrior, 5);
    setGuildLevel(ch, Guild::Paladin, 6);
    CHECK(!isAheadOfEveryGuild(ch, Guild::Warrior));
    CHECK(isAheadOfEveryGuild(ch, Guild::Paladin));
}

// ---- Level-up gold: 30.1 * L^2 * (1 + ggf/10), cumulative at current level (SRD T7, T10) ----

TEST(level_up_gold_at_zero_guild_factor_is_30_1_per_square) {
    // L = 10: 30.1 * 100 = 3010 gold = 30,100,000 units.
    CHECK_EQ(levelUpGoldCost(10, 0.0), 30100000);
}

TEST(level_up_gold_guild_factor_scales_by_one_plus_tenth) {
    // ggf = 10 gives multiplier 2: 3010 * 2 = 6020 gold = 60,200,000 units.
    CHECK_EQ(levelUpGoldCost(10, 10.0), 60200000);
}

// ---- Experience cap: rises with level and with the factors (SRD T7, T10, conflicts 13-14) ----

TEST(experience_cap_rises_with_level) {
    CHECK(experienceCap(10, 8.0, 1.0) < experienceCap(11, 8.0, 1.0));
    CHECK(experienceCap(50, 8.0, 1.0) < experienceCap(51, 8.0, 1.0));
}

TEST(experience_cap_rises_with_guild_factor) {
    // Guild experience factor multiplies the threshold (Giant GEF 16 > Nomad GEF 8).
    CHECK(experienceCap(20, 8.0, 1.0) < experienceCap(20, 16.0, 1.0));
}

TEST(experience_cap_rises_with_race_factor) {
    // Dwarf race factor 1.105 is above a base race factor of 1.0.
    CHECK(experienceCap(20, 8.0, 1.0) < experienceCap(20, 8.0, 1.105));
}

// ---- Rest aging applied to the character (SRD T9) ----

TEST(rest_aging_adds_cost_days_at_zero_hp) {
    Character ch;
    ch.ageDays = kAgeStartDays;          // 5840
    ch.vitals.maxHits = 15;
    ch.vitals.curHits = 0;
    tsup::Env env({0.5, 0.9});
    applyRestAging(env.ctx, ch);
    CHECK_EQ(ch.ageDays, kAgeStartDays + 31);
}

TEST(rest_aging_at_full_hp_adds_nothing) {
    Character ch;
    ch.ageDays = kAgeStartDays;
    ch.vitals.maxHits = 15;
    ch.vitals.curHits = 15;
    tsup::Env env({0.5, 0.9});
    applyRestAging(env.ctx, ch);
    CHECK_EQ(ch.ageDays, kAgeStartDays);
}

// ---- Poison cure: success clears and costs 2 HP; failure leaves poison (SRD T9) ----

TEST(poison_cure_success_clears_poison_and_costs_2_hp) {
    Character ch;
    ch.ailments.poisonLevel = 3;
    ch.vitals.curHits = 10;
    tsup::Env env({0.5, 0.9});
    applyPoisonCure(env.ctx, ch, 186);
    CHECK_EQ(ch.ailments.poisonLevel, 0);
    CHECK_EQ(ch.vitals.curHits, 8);
}

TEST(poison_cure_failure_keeps_poison) {
    Character ch;
    ch.ailments.poisonLevel = 3;
    ch.vitals.curHits = 10;
    tsup::Env env({0.5, 0.9});
    applyPoisonCure(env.ctx, ch, 185);
    CHECK_EQ(ch.ailments.poisonLevel, 3);
}

// ---- Disease cure: success clears and costs 1 HP; failure leaves disease (SRD T9) ----

TEST(disease_cure_success_clears_disease_and_costs_1_hp) {
    Character ch;
    ch.ailments.diseaseLevel = 2;
    ch.vitals.curHits = 10;
    tsup::Env env({0.5, 0.9});
    applyDiseaseCure(env.ctx, ch, 196);
    CHECK_EQ(ch.ailments.diseaseLevel, 0);
    CHECK_EQ(ch.vitals.curHits, 9);
}

TEST(disease_cure_failure_keeps_disease) {
    Character ch;
    ch.ailments.diseaseLevel = 2;
    ch.vitals.curHits = 10;
    tsup::Env env({0.5, 0.9});
    applyDiseaseCure(env.ctx, ch, 195);
    CHECK_EQ(ch.ailments.diseaseLevel, 2);
}

