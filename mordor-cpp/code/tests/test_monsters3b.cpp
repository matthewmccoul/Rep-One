#include "test_support.h"
#include "mordor/monsters3b.h"

using namespace mordor;

// Not tested: groupSizeCInt (CInt is not defined in the SRD part) and
// newbieGuardExcludes (the meaning of "newbie" is not given in the SRD part).

TEST(constants_match_srd) {
    CHECK_EQ(kAttackAttemptCap, 5);
    CHECK_EQ(kSwingBoundFactor, 0.5);
    CHECK_EQ(kSwingBoundDivisor, 2.0);
    CHECK_EQ(kSwingBoundBonus, 1);
    CHECK_EQ(kMemberBaseHitsFloor, 1);
    CHECK_EQ(kRespawnSecondsPerArea, static_cast<i64>(5));
    CHECK_EQ(kJoinChancePerCha, 10);
    CHECK_EQ(kJoinChancePerFloor, 10);
    CHECK_EQ(kJoinChanceRollMax, 3000);
    CHECK_EQ(kLairPlacePercent, 75);
    CHECK_EQ(kNewbieMonsterGuildCap, 2);
}

// R32: attempts per group = min(living count, 5) [c23].
TEST(attack_attempts_is_living_count_up_to_cap) {
    CHECK_EQ(attackAttemptsPerGroup(0), 0);
    CHECK_EQ(attackAttemptsPerGroup(1), 1);
    CHECK_EQ(attackAttemptsPerGroup(3), 3);
    CHECK_EQ(attackAttemptsPerGroup(5), 5);
}

// R45: max(1, base Hits) [c7]. Base 0 is deliberately not tested (UNKNOWN).
TEST(member_base_hits_keeps_positive_base) {
    CHECK_EQ(memberBaseHits(1), 1);
    CHECK_EQ(memberBaseHits(7), 7);
    CHECK_EQ(memberBaseHits(40), 40);
}

// R46: most = Int(Hits * 0.5 / 2) + 1 [c66].
TEST(swing_bound_matches_audit_formula) {
    CHECK_EQ(monsterSwingBound(20), 6);  // Int(5.0) + 1
    CHECK_EQ(monsterSwingBound(10), 3);  // Int(2.5) + 1
    CHECK_EQ(monsterSwingBound(4), 2);   // Int(1.0) + 1
}

// Int() truncates: 9 -> 2.25 -> 2; 3 -> 0.75 -> 0.
TEST(swing_bound_truncates_fractional_part) {
    CHECK_EQ(monsterSwingBound(9), 3);
    CHECK_EQ(monsterSwingBound(3), 1);
    CHECK_EQ(monsterSwingBound(1), 1);
}

// Respawn: engine formula (5*(level/2.0))*60 = 150 * level [c9][c7].
TEST(respawn_level_seconds_is_150_per_level) {
    CHECK_EQ(respawnLevelSeconds(1), static_cast<i64>(150));
    CHECK_EQ(respawnLevelSeconds(2), static_cast<i64>(300));
    CHECK_EQ(respawnLevelSeconds(5), static_cast<i64>(750));
}

// Engine: 5 s per empty area below the header count [c9][c13].
// Sign convention is not pinned here; only the step size is checked.
TEST(respawn_empty_area_steps_by_five_seconds) {
    i64 a = respawnEmptyAreaSeconds(10, 4);
    i64 b = respawnEmptyAreaSeconds(10, 5);
    i64 d = a - b;
    CHECK(d == 5 || d == -5);
    i64 c = respawnEmptyAreaSeconds(10, 5);
    i64 e = respawnEmptyAreaSeconds(10, 6);
    i64 f = c - e;
    CHECK(f == 5 || f == -5);
}

// Lair: 75% roll. Scripted 0.5 -> 50 < 75 -> placed.
TEST(lair_roll_places_lair_below_75_percent) {
    tsup::Env env({0.5, 0.9});
    Area area{};
    area.lairId = 3;
    CHECK(rollLairPlacement(env.ctx, area));
}

// Scripted 0.9 -> 90 >= 75 -> not placed.
TEST(lair_roll_fails_at_or_above_75_percent) {
    tsup::Env env({0.9, 0.5});
    Area area{};
    area.lairId = 3;
    CHECK(!rollLairPlacement(env.ctx, area));
}

// Threshold expression: 10*CHA - 10*(floor - 1) [c23][c97][c25].
TEST(join_threshold_is_cha_term_minus_floor_term) {
    CHECK_EQ(joinChanceThreshold(10, 1), 100);  // 100 - 0
    CHECK_EQ(joinChanceThreshold(12, 3), 100);  // 120 - 20
    CHECK_EQ(joinChanceThreshold(5, 2), 40);    // 50 - 10
}

// Low CHA and deep floor give a non-positive threshold; clamp-agnostic check.
TEST(join_threshold_low_cha_high_floor_is_not_positive) {
    CHECK(joinChanceThreshold(1, 5) <= 0);  // 10 - 40 = -30
}

// With a non-positive threshold, even the lowest roll (1) fails.
TEST(join_roll_fails_when_threshold_not_positive) {
    tsup::Env env({0.0, 0.0});
    CHECK(!joinChanceRoll(env.ctx, 1, 5));
}

// Threshold 100; scripted 0.0 -> roll 1, which is well inside.
TEST(join_roll_succeeds_on_low_roll) {
    tsup::Env env({0.0, 0.9});
    CHECK(joinChanceRoll(env.ctx, 10, 1));
}

// Threshold 100; scripted 0.5 -> roll about 1501, which is well outside.
TEST(join_roll_fails_on_high_roll) {
    tsup::Env env({0.5, 0.0});
    CHECK(!joinChanceRoll(env.ctx, 10, 1));
}

// Clamp floor is UNKNOWN [c18][c25]; only values >= 1 are tested.
TEST(clamp_log_cha_passes_values_at_or_above_one) {
    CHECK_EQ(clampLogCha(1.0), 1.0);
    CHECK_EQ(clampLogCha(2.5), 2.5);
}

// Engine gate: lead.AlignmentValue == 1 (Neutral) [c7].
TEST(neutral_gate_requires_neutral_lead) {
    Character lead{};
    lead.alignment = Align::Neutral;
    CHECK(neutralGateMet(lead));
    lead.alignment = Align::Evil;
    CHECK(!neutralGateMet(lead));
}

// Audit mapping: 4->4, 5->5, 6->4 [c24].
TEST(companion_mode_table_index_direct_modes) {
    CHECK_EQ(companionModeTableIndex(4), 4);
    CHECK_EQ(companionModeTableIndex(5), 5);
    CHECK_EQ(companionModeTableIndex(6), 4);
}

// Audit mapping: modes above 7 subtract 7 [c24].
TEST(companion_mode_above_seven_subtracts_seven) {
    CHECK_EQ(companionModeTableIndex(8), 1);
    CHECK_EQ(companionModeTableIndex(9), 2);
    CHECK_EQ(companionModeTableIndex(11), 4);
}
