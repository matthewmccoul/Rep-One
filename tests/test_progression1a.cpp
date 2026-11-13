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
