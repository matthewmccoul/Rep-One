#include "test_support.h"
#include "mordor/progression2.h"

#include <cmath>

using namespace mordor;

namespace {

GuildMembership& memb(Character& ch, Guild g) {
    return ch.guilds[static_cast<u32>(g)];
}

void setGuild(Character& ch, Guild g, i32 level, i64 exp) {
    GuildMembership& m = memb(ch, g);
    m.joined = true;
    m.level = level;
    m.experience = exp;
}

bool closeTo(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

}  // namespace

// ---- T5: CON loss after a successful raise (chance in percent) ----

TEST(prog2_con_loss_table_from_t5) {
    CHECK_EQ(conLossChance(21), 8);
    CHECK_EQ(conLossChance(23), 4);
    CHECK_EQ(conLossChance(24), 2);
    CHECK_EQ(conLossChance(25), 0);
    CHECK_EQ(conLossChance(27), 0);
}

TEST(prog2_con_loss_formula_and_floor) {
    // 50 - 2*CON, floored at 0.
    CHECK_EQ(conLossChance(0), 50);
    CHECK_EQ(conLossChance(10), 30);
    CHECK_EQ(conLossChance(17), 16);
    CHECK_EQ(conLossChance(30), 0);  // 50 - 60 = -10 floors to 0
}

// ---- Complication chance: min(1, 2*Age/MaxAge) ----

TEST(prog2_complication_chance_at_start_age) {
    Character ch;
    ch.ageDays = kAgeStartDays;  // 16 years
    CHECK(closeTo(complicationChance(ch, 100), 0.32, 1e-9));  // Human, 32.0%
    CHECK(closeTo(complicationChance(ch, 400), 0.08, 1e-9));  // Elf, 8.0%
    CHECK(closeTo(complicationChance(ch, 275), 32.0 / 275.0, 1e-9));  // Dwarf, 11.6%
}

TEST(prog2_complication_chance_caps_at_one) {
    Character ch;
    ch.ageDays = 50 * 365;  // exactly half of max age 100: 2*50/100 = 1
    CHECK(closeTo(complicationChance(ch, 100), 1.0, 1e-9));

    ch.ageDays = 60 * 365;  // 2*60/100 = 1.2 caps at 1
    CHECK(closeTo(complicationChance(ch, 100), 1.0, 1e-9));
}

// ---- Age in years ----

TEST(prog2_age_years_boundaries) {
    Character ch;
    ch.ageDays = kAgeStartDays - 1;
    CHECK_EQ(ageYears(ch), 15);
    ch.ageDays = kAgeStartDays;
    CHECK_EQ(ageYears(ch), 16);
    ch.ageDays = kAgeStartDays + 365;
    CHECK_EQ(ageYears(ch), 17);
}

TEST(prog2_start_age_constant_is_sixteen_years) {
    CHECK_EQ(kAgeStartDays, 16 * 365);
}

// ---- Day display quirks ----

TEST(prog2_describe_days_week_display) {
    // Withering: 28 days displays as "4 week(s)".
    CHECK_EQ(describeDays(28), Str("4 week(s)"));
}

TEST(prog2_describe_days_seven_days_quirk) {
    // Quirk: 7 days prints "7 day(s)" because the week branch needs days > 7.
    CHECK_EQ(describeDays(7), Str("7 day(s)"));
}

// ---- Aging events (T3) ----

TEST(prog2_age_by_adds_days) {
    tsup::Env env({0.5, 0.9});
    Character ch;
    ch.ageDays = kAgeStartDays;
    ageBy(env.ctx, ch, 28);
    CHECK_EQ(ch.ageDays, kAgeStartDays + 28);
}

TEST(prog2_withering_ages_28_days) {
    tsup::Env env({0.5, 0.9});
    Character ch;
    ch.ageDays = kAgeStartDays;
    withering(env.ctx, ch);
    CHECK_EQ(ch.ageDays, kAgeStartDays + 28);
}

TEST(prog2_complication_ages_fifteen_years) {
    Character ch;
    ch.ageDays = kAgeStartDays;
    complication(ch);
    CHECK_EQ(ch.ageDays, kAgeStartDays + 15 * 365);
}

// ---- Experience thresholds: structural relations ----

TEST(prog2_exp_for_this_level_zero_at_level_one) {
    Character ch;
    setGuild(ch, Guild::Warrior, 1, 0);
    CHECK_EQ(expForThisLevel(ch, Guild::Warrior), 0);
}

TEST(prog2_exp_for_this_level_equals_next_of_previous) {
    // ExpForThisLevel() at level L = fn12A1(L-1) = ExpForNextLevel() at level L-1.
    Character ch;
    setGuild(ch, Guild::Warrior, 4, 0);
    const i64 nextAtFour = expForNextLevel(ch, Guild::Warrior);
    memb(ch, Guild::Warrior).level = 5;
    const i64 thisAtFive = expForThisLevel(ch, Guild::Warrior);
    CHECK_EQ(thisAtFive, nextAtFour);
}

TEST(prog2_cap_is_next_threshold_minus_one) {
    // Fn13ED(level) = Fn12A1(level+1) - 1, so cap at L = ExpForNextLevel at L+1, minus 1.
    Character ch;
    setGuild(ch, Guild::Nomad, 5, 0);
    const i64 nextAtFive = expForNextLevel(ch, Guild::Nomad);
    memb(ch, Guild::Nomad).level = 4;
    const i64 capAtFour = experienceCapOf(ch, Guild::Nomad);
    CHECK_EQ(capAtFour, nextAtFive - 1);
}

TEST(prog2_is_pinned_boundary_at_cap) {
    Character ch;
    setGuild(ch, Guild::Nomad, 4, 0);
    const i64 cap = experienceCapOf(ch, Guild::Nomad);

    memb(ch, Guild::Nomad).experience = cap - 1;
    CHECK(!isPinned(ch, Guild::Nomad));

    memb(ch, Guild::Nomad).experience = cap;
    CHECK(isPinned(ch, Guild::Nomad));
}

TEST(prog2_can_level_up_boundary_at_next_threshold) {
    Character ch;
    setGuild(ch, Guild::Nomad, 1, 0);
    const i64 next = expForNextLevel(ch, Guild::Nomad);

    memb(ch, Guild::Nomad).experience = next - 1;
    CHECK(!canLevelUp(ch, Guild::Nomad));

    memb(ch, Guild::Nomad).experience = next;
    CHECK(canLevelUp(ch, Guild::Nomad));
}

// ---- Gold anchors (T7, engine convention: current level) ----

TEST(prog2_gold_for_next_level_anchors) {
    // 30.1 * L^2 * (1 + GGF/10): Warrior GGF 1 at L1 = 33.11 -> 33.
    Character warrior;
    setGuild(warrior, Guild::Warrior, 1, 0);
    CHECK_EQ(goldForNextLevel(warrior, Guild::Warrior), 33);

    // Nomad GGF 3 at L3 = 30.1 * 9 * 1.3 = 352.17 -> 352.
    Character nomad;
    setGuild(nomad, Guild::Nomad, 3, 0);
    CHECK_EQ(goldForNextLevel(nomad, Guild::Nomad), 352);
}

// ---- Totals and cross-guild level ----

TEST(prog2_total_experience_sums_guilds) {
    Character ch;
    setGuild(ch, Guild::Nomad, 2, 100);
    setGuild(ch, Guild::Warrior, 1, 250);
    setGuild(ch, Guild::Mage, 1, 0);
    CHECK_EQ(totalExperience(ch), 350);
}

TEST(prog2_highest_other_level_excludes_own_guild) {
    Character ch;
    setGuild(ch, Guild::Nomad, 9, 0);
    setGuild(ch, Guild::Warrior, 5, 0);
    setGuild(ch, Guild::Mage, 3, 0);
    CHECK_EQ(highestOtherLevel(ch, Guild::Nomad), 5);
    CHECK_EQ(highestOtherLevel(ch, Guild::Warrior), 9);
}

TEST(prog2_highest_other_level_zero_when_none) {
    Character ch;
    CHECK_EQ(highestOtherLevel(ch, Guild::Nomad), 0);
}

// ---- Awarding experience (pin cap) ----

TEST(prog2_award_exp_adds_amount_below_cap) {
    Character ch;
    setGuild(ch, Guild::Nomad, 1, 0);
    awardExp(ch, Guild::Nomad, 1);
    CHECK_EQ(memb(ch, Guild::Nomad).experience, 1);
}

TEST(prog2_award_exp_clamps_at_cap) {
    Character ch;
    setGuild(ch, Guild::Nomad, 4, 0);
    const i64 cap = experienceCapOf(ch, Guild::Nomad);
    memb(ch, Guild::Nomad).experience = cap - 5;
    awardExp(ch, Guild::Nomad, 100);
    CHECK_EQ(memb(ch, Guild::Nomad).experience, cap);
}

TEST(prog2_award_exp_pinned_gains_nothing) {
    Character ch;
    setGuild(ch, Guild::Nomad, 4, 0);
    const i64 cap = experienceCapOf(ch, Guild::Nomad);
    memb(ch, Guild::Nomad).experience = cap;
    awardExp(ch, Guild::Nomad, 500);
    CHECK_EQ(memb(ch, Guild::Nomad).experience, cap);
}

// ---- Monster kill XP: documented zero and relations ----

TEST(prog2_exp_awarded_zero_at_zero_inflicted) {
    // Quirk: ExpAwarded gives zero at h = 0, with or without the companion flag.
    CHECK_EQ(expAwarded(0, 5, 3, false, 1), 0);
    CHECK_EQ(expAwarded(0, 5, 3, true, 1), 0);
}

TEST(prog2_exp_awarded_grows_with_inflicted_and_monster_level) {
    const i64 small = expAwarded(400, 5, 3, false, 1);
    const i64 moreDamage = expAwarded(800, 5, 3, false, 1);
    const i64 strongerMonster = expAwarded(400, 10, 3, false, 1);
    CHECK(small > 0);
    CHECK(moreDamage > small);
    CHECK(strongerMonster > small);
}

TEST(prog2_exp_awarded_shrinks_with_guild_level) {
    // Guild level enters through GuildLevelFactor, which falls as level rises.
    const i64 lowLevel = expAwarded(400, 5, 1, false, 1);
    const i64 highLevel = expAwarded(400, 5, 333, false, 1);
    CHECK(lowLevel > highLevel);
}

TEST(prog2_companion_share_is_about_a_quarter) {
    // Companion flag applies kCompanionShare (0.25) within XP; rounding allows +/-1.
    const i64 solo = expAwarded(400, 5, 3, false, 1);
    const i64 companion = expAwarded(400, 5, 3, true, 1);
    CHECK(solo > 0);
    CHECK(closeTo(static_cast<double>(companion), static_cast<double>(solo) * kCompanionShare, 1.0));
}

TEST(prog2_party_penalty_four_members) {
    // Party penalty 0.04 per extra member: 4 members = 1 - 3*0.04 = 0.88.
    const i64 solo = expAwarded(400, 5, 3, false, 1);
    const i64 party = expAwarded(400, 5, 3, false, 4);
    const double factor = 1.0 - kPartyPenaltyPerMember * 3.0;
    CHECK(closeTo(factor, 0.88, 1e-12));
    CHECK(closeTo(static_cast<double>(party), static_cast<double>(solo) * factor, 1.0));
}

// ---- Constant tables ----

TEST(prog2_race_and_guild_constant_tables) {
    // T1 raw Exp 6 -> Ogre is not used here; Morloch raw Exp 4 maps to 1.05819132268.
    CHECK(closeTo(kRaceExpMult[static_cast<i32>(Race::Morloch)], 1.05819132268, 1e-11));
    CHECK(closeTo(kRaceExpMult[static_cast<i32>(Race::Troll)], 1.25647909518, 1e-11));
    CHECK(closeTo(kRaceExpMult[static_cast<i32>(Race::Human)], 1.0, 1e-11));

    // T2 EP values; Ninja and Villain share 2.625.
    CHECK(closeTo(kGuildExpFactor[static_cast<i32>(Guild::Wizard)], 3.75, 1e-12));
    CHECK(closeTo(kGuildExpFactor[static_cast<i32>(Guild::Healer)], 4.0, 1e-12));
    CHECK(closeTo(kGuildExpFactor[static_cast<i32>(Guild::Ninja)],
                  kGuildExpFactor[static_cast<i32>(Guild::Villain)], 1e-12));
}

TEST(prog2_documented_scalar_constants) {
    CHECK(closeTo(kCompanionShare, 0.25, 1e-12));
    CHECK(closeTo(kChestGuildShare, 0.5, 1e-12));
    CHECK(closeTo(kPartyPenaltyPerMember, 0.04, 1e-12));
    CHECK(closeTo(kGoldCoef, 30.1, 1e-12));
    CHECK(closeTo(kGoldFactorDivisor, 10.0, 1e-12));
}
