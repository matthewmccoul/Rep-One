#include "test_support.h"
#include "mordor/death1b.h"
#include "mordor/death2b.h"

#include <cmath>

// Lifespan in years used throughout. 1 year = 365 days (kAgeStartDays = 16 * 365).
// 100 years = 36500 days, so 36500 days is 100% of max age.

TEST(old_age_constants_match_srd) {
    CHECK(mordor::kOldAgeStartPct == 98.0);
    CHECK(mordor::kOldAgeBaseChance == 5.0);
}

TEST(old_age_percent_zero_age_is_zero) {
    CHECK(mordor::oldAgePercent(0, 100) == 0.0);
}

TEST(old_age_percent_full_max_age_is_100) {
    // 100 years of 365 days = 36500 days = 100% of a 100-year lifespan.
    CHECK(std::fabs(mordor::oldAgePercent(36500, 100) - 100.0) < 0.01);
}

TEST(old_age_percent_halves_when_lifespan_doubles) {
    // Same age, double the max age: half the percentage.
    double at100 = mordor::oldAgePercent(36500, 100);
    double at200 = mordor::oldAgePercent(36500, 200);
    CHECK(std::fabs(at100 - 100.0) < 0.01);
    CHECK(std::fabs(at200 - 50.0) < 0.01);
}

TEST(old_age_threshold_at_99_is_base_chance) {
    // Threshold = 5 + Ln(pct - 98); Ln(1) = 0 at pct 99.
    CHECK(std::fabs(mordor::oldAgeThreshold(99.0) - 5.0) < 1e-9);
}

TEST(old_age_threshold_at_one_e_above_98_is_6) {
    // pct - 98 = e gives Ln(e) = 1, so threshold = 6.
    double pct = 98.0 + std::exp(1.0);
    CHECK(std::fabs(mordor::oldAgeThreshold(pct) - 6.0) < 1e-9);
}

TEST(old_age_threshold_at_e_squared_above_98_is_7) {
    // pct - 98 = e^2 gives Ln = 2, so threshold = 7.
    double pct = 98.0 + std::exp(2.0);
    CHECK(std::fabs(mordor::oldAgeThreshold(pct) - 7.0) < 1e-9);
}

TEST(old_age_threshold_rises_as_age_increases) {
    // The chance rises as age approaches and passes max age.
    CHECK(mordor::oldAgeThreshold(99.5) < mordor::oldAgeThreshold(100.0));
    CHECK(mordor::oldAgeThreshold(100.0) < mordor::oldAgeThreshold(105.0));
}

TEST(old_age_check_applies_in_dungeon_above_98) {
    // 36135 days / 36500 = 99% of a 100-year lifespan.
    mordor::Character ch;
    ch.ageDays = 36135;
    CHECK(mordor::oldAgeCheckApplies(ch, 100, true));
}

TEST(old_age_check_does_not_apply_below_98) {
    // 35733 days / 36500 = 97.9%, below the 98% start.
    mordor::Character ch;
    ch.ageDays = 35733;
    CHECK(!mordor::oldAgeCheckApplies(ch, 100, true));
}

TEST(old_age_check_just_above_98_applies) {
    // 35813 days / 36500 = 98.1%, just above the 98% start.
    mordor::Character ch;
    ch.ageDays = 35813;
    CHECK(mordor::oldAgeCheckApplies(ch, 100, true));
}

TEST(old_age_check_never_applies_outside_dungeon) {
    // "may randomly die in the dungeon": 200% of max age, but not in dungeon.
    mordor::Character ch;
    ch.ageDays = 73000;
    CHECK(!mordor::oldAgeCheckApplies(ch, 100, false));
}

TEST(old_age_check_newborn_does_not_apply) {
    mordor::Character ch;
    ch.ageDays = 0;
    CHECK(!mordor::oldAgeCheckApplies(ch, 100, true));
}

TEST(old_age_roll_no_death_outside_dungeon) {
    // Even a roll of 0.0 cannot kill outside the dungeon.
    tsup::Env env({0.0, 0.0});
    mordor::Character ch;
    ch.ageDays = 73000;
    bool died = mordor::rollOldAgeDeath(env.ctx, ch, 100, false);
    CHECK(!died);
    CHECK(!ch.isDead());
}

TEST(old_age_roll_no_death_below_98) {
    tsup::Env env({0.0, 0.0});
    mordor::Character ch;
    ch.ageDays = 35733;  // 97.9% of 100 years
    bool died = mordor::rollOldAgeDeath(env.ctx, ch, 100, true);
    CHECK(!died);
    CHECK(!ch.isDead());
}

TEST(old_age_roll_low_value_kills_in_dungeon) {
    // 200% of max age: threshold = 5 + Ln(102), about 9.6. A roll of 0.0 is under it.
    tsup::Env env({0.0, 0.0});
    mordor::Character ch;
    ch.ageDays = 73000;
    bool died = mordor::rollOldAgeDeath(env.ctx, ch, 100, true);
    CHECK(died);
    CHECK(ch.isDead());
}

TEST(old_age_roll_high_value_spares_in_dungeon) {
    // Roll of 0.999 is 99.9%, far above the threshold of about 9.6.
    tsup::Env env({0.999, 0.999});
    mordor::Character ch;
    ch.ageDays = 73000;
    bool died = mordor::rollOldAgeDeath(env.ctx, ch, 100, true);
    CHECK(!died);
    CHECK(!ch.isDead());
}

TEST(old_age_roll_at_99_percent_threshold_is_5) {
    // At 99% the threshold is exactly 5. Roll 4% dies; roll 6% survives.
    mordor::Character chDie;
    chDie.ageDays = 36135;
    tsup::Env envDie({0.04, 0.04});
    CHECK(mordor::rollOldAgeDeath(envDie.ctx, chDie, 100, true));

    mordor::Character chLive;
    chLive.ageDays = 36135;
    tsup::Env envLive({0.06, 0.06});
    CHECK(!mordor::rollOldAgeDeath(envLive.ctx, chLive, 100, true));
    CHECK(!chLive.isDead());
}

TEST(party_copy_index_of_first_member_is_zero) {
    mordor::Party party;
    party.size = 3;
    party.members[0].name = "Ann";
    party.members[1].name = "Bob";
    party.members[2].name = "Cyd";
    CHECK_EQ(mordor::partyCopyIndexOf(party, party.members[0]), 0);
}

TEST(party_copy_index_of_third_member_is_two) {
    mordor::Party party;
    party.size = 3;
    party.members[0].name = "Ann";
    party.members[1].name = "Bob";
    party.members[2].name = "Cyd";
    CHECK_EQ(mordor::partyCopyIndexOf(party, party.members[2]), 2);
}

TEST(party_copy_index_resolves_a_copy_to_party_copy) {
    // R37: the picked character is resolved to the party's own copy.
    mordor::Party party;
    party.size = 3;
    party.members[0].name = "Ann";
    party.members[1].name = "Bob";
    party.members[2].name = "Cyd";
    mordor::Character copy = party.members[1];
    CHECK_EQ(mordor::partyCopyIndexOf(party, copy), 1);
}
