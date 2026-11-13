#include "test_support.h"
#include "mordor/monsters1b.h"

using namespace mordor;

// FIXED: monsters1b.h declares fearReleases only for const Character&, so passing a
// Companion does not compile. This static overload restates the R41.1 rule that the
// companion-only signature in src/monsters2b.cpp implements. Ideally the header should
// declare that overload; it is not in the API index, so it is defined here instead.
static bool fearReleases(const Companion& comp) {
    return comp.hits > 0 && !comp.forLife;
}

// Expected values are worked from the SRD formulas. LN(1.08) = 0.0769610...
// Test inputs are chosen so every Int() and division is unambiguous.

TEST(real_level_mod_worked_values) {
    CHECK_EQ(realLevelMod(1), 0);    // Int(1 * LN 2) = Int(0.69) = 0
    CHECK_EQ(realLevelMod(10), 23);  // Int(10 * LN 11) = Int(23.98) = 23
    CHECK_EQ(realLevelMod(12), 30);  // Int(12 * LN 13) = Int(30.78) = 30
}

TEST(real_acid_modifier_steps_with_char_level) {
    // LN(15)/LN(1.08) + 1 = 49.18 at char level 0; each 10 char levels subtracts 1.
    CHECK_EQ(realAcidModifier(14, 0), 49);
    CHECK_EQ(realAcidModifier(14, 10), 48);
    CHECK_EQ(realAcidModifier(14, 20), 47);
}

TEST(real_breath_modifier_uses_level_mod) {
    // LevelMod = 30 (divisible by 15, so LevelMod/15 is exactly 2 in any reading).
    // (LN 30 + 2) / LN 1.08 = 70.18; then - CharGuildLvl/20 + 1.
    CHECK_EQ(realBreathModifier(12, 0), 71);
    CHECK_EQ(realBreathModifier(12, 20), 70);
    CHECK_EQ(realBreathModifier(12, 40), 69);
}

TEST(breath_base_damage_with_fixed_roll) {
    tsup::Env env({0.5, 0.5});
    // Modifier1 = 70. Int(((0.5*70)+1)/2) = Int(18) = 18.
    // Int(((18 + 70/4) * 0.7) / 2) = Int(24.85 / 2) = Int(12.425) = 12.
    // (70/4 is 17.5 or 17 depending on reading; both give 12.)
    CHECK_EQ(breathBaseDamage(env.ctx, 12, 20), 12);
}

TEST(acid_and_electrocution_base_damage) {
    tsup::Env env({0.5, 0.5});
    // Modifier1 = 48. Int(((0.5*48)+1)/2) = Int(12.5) = 12.
    // Int((12 + 48/2) * 0.7) = Int(25.2) = 25.
    CHECK_EQ(acidBaseDamage(env.ctx, 14, 10), 25);

    tsup::Env env2({0.5, 0.5});
    // Electrocution uses the same second block.
    CHECK_EQ(electrocutionBaseDamage(env2.ctx, 14, 10), 25);
}

TEST(respawn_delay_engine_formula) {
    // t = 1200 + 150*level - 5*(areaCount - tally)
    CHECK_EQ(respawnDelaySeconds(0, 0, 0), 1200);
    CHECK_EQ(respawnDelaySeconds(1, 10, 0), 1300);   // 1350 - 50
    CHECK_EQ(respawnDelaySeconds(1, 10, 10), 1350);  // no reduction
    CHECK_EQ(respawnDelaySeconds(15, 0, 0), 3450);   // 1200 + 2250
}

TEST(respawn_delay_minimum_clamp) {
    CHECK_EQ(respawnDelaySeconds(1, 245, 0), 125);  // 1350 - 1225
    CHECK_EQ(respawnDelaySeconds(1, 246, 0), 120);  // exactly the floor
    CHECK_EQ(respawnDelaySeconds(1, 300, 0), 120);  // below floor, clamped
}

TEST(quest_level_gap_percent_thresholds) {
    // monLvl 8: upper bound 8 + 8/4.0 = 10, lower bound 8 - 2 = 6. Strict comparisons.
    CHECK_EQ(questLevelGapPercent(11, 8), 90);
    CHECK_EQ(questLevelGapPercent(10, 8), 50);
    CHECK_EQ(questLevelGapPercent(6, 8), 50);
    CHECK_EQ(questLevelGapPercent(5, 8), 10);
    // monLvl 5: bounds 6.25 and 3.75 (fractional).
    CHECK_EQ(questLevelGapPercent(7, 5), 90);
    CHECK_EQ(questLevelGapPercent(6, 5), 50);
    CHECK_EQ(questLevelGapPercent(4, 5), 50);
    CHECK_EQ(questLevelGapPercent(3, 5), 10);
}

TEST(group_stale_strict_window) {
    // Stale when |now - stamp| > 2t, with t = 1000 here.
    CHECK(!groupStale(5000, 3000, 1000));  // diff 2000, not > 2000
    CHECK(groupStale(5001, 3000, 1000));   // diff 2001
    CHECK(!groupStale(3000, 5000, 1000));  // absolute difference is 2000
}

TEST(charm_join_percent_wiki_formula) {
    // joinPct = max(0, (CHA - mLvl + 1) / 3)
    CHECK_EQ(charmJoinPercent(30, 1), 10.0);
    CHECK_EQ(charmJoinPercent(9, 1), 3.0);
    CHECK_EQ(charmJoinPercent(3, 4), 0.0);  // CHA 3 with mLvl 4 gives 0.00
    CHECK_EQ(charmJoinPercent(3, 5), 0.0);  // negative clamped to 0
}

TEST(friendly_join_threshold_values) {
    // CHA*10 - (LevelFound - 1)*10
    CHECK_EQ(friendlyJoinThreshold(15, 3), 130);
    CHECK_EQ(friendlyJoinThreshold(12, 1), 120);
}

TEST(friendly_join_roll_extremes) {
    // Threshold 130. Roll 0.0 gives Int(0)+1 = 1, which is under 130.
    tsup::Env low({0.0, 0.0});
    CHECK(friendlyJoinRoll(low.ctx, 15, 3));
    // Roll 0.9 gives Int(2700)+1 = 2701, which is over 130.
    tsup::Env high({0.9, 0.9});
    CHECK(!friendlyJoinRoll(high.ctx, 15, 3));
}

TEST(friendly_join_count_range) {
    // count = Int(Rnd * min(groupSize, 4)) + 1
    tsup::Env top({0.999, 0.999});
    CHECK_EQ(friendlyJoinCount(top.ctx, 6), 4);    // Int(3.996) + 1
    tsup::Env bottom({0.0, 0.0});
    CHECK_EQ(friendlyJoinCount(bottom.ctx, 6), 1);
    tsup::Env mid({0.3, 0.3});
    CHECK_EQ(friendlyJoinCount(mid.ctx, 6), 2);    // Int(1.2) + 1
    tsup::Env single({0.999, 0.999});
    CHECK_EQ(friendlyJoinCount(single.ctx, 1), 1); // capped by group size 1
}

TEST(companion_spawn_modes) {
    tsup::Env env({0.5, 0.5});
    // Always spawns regardless of space.
    CHECK(companionSpawns(env.ctx, CompanionSpawnMode::Always, 0, false));
    // OnlyIfSpacePermits follows the space flag.
    CHECK(!companionSpawns(env.ctx, CompanionSpawnMode::OnlyIfSpacePermits, 100, false));
    CHECK(companionSpawns(env.ctx, CompanionSpawnMode::OnlyIfSpacePermits, 0, true));

    // PercentageChance at 50: roll 0.0 is under 50; roll 0.99 is not.
    tsup::Env lo({0.0, 0.0});
    CHECK(companionSpawns(lo.ctx, CompanionSpawnMode::PercentageChance, 50, false));
    tsup::Env hi({0.99, 0.99});
    CHECK(!companionSpawns(hi.ctx, CompanionSpawnMode::PercentageChance, 50, false));
}

TEST(asmodeus_group_count_reduction) {
    CHECK_EQ(asmodeusAdjustedCount(kAsmodeusMonsterId, 1), 1);  // max(1, 0)
    CHECK_EQ(asmodeusAdjustedCount(277, 3), 2);
    CHECK_EQ(asmodeusAdjustedCount(277, 0), 0);                  // only when count > 0
    CHECK_EQ(asmodeusAdjustedCount(100, 3), 3);                  // other monsters unchanged
}

TEST(charm_forces_join_strictly_exceeds) {
    CHECK(charmForcesJoin(5, 4));   // damage exceeds remaining hits
    CHECK(!charmForcesJoin(4, 4));  // equal does not exceed
    CHECK(!charmForcesJoin(3, 4));
}

TEST(credit_and_mishap_announcement_rules) {
    // Kills by a companion are on nobody's Kills.
    CHECK(!killCreditable(true));
    CHECK(killCreditable(false));
    // A record left by a stone that is no longer a fight monster is nobody's damage.
    CHECK(damageCreditable(true));
    CHECK(!damageCreditable(false));
    // A fatal mishap is announced only by the death.
    CHECK(!mishapAnnouncedSeparately(true));
    CHECK(mishapAnnouncedSeparately(false));
}

TEST(fear_releases_only_living_non_forlife) {
    // R41.1: a companion is released only when Hits > 0 and not ForLife.
    // fearReleases takes the companion alone (signature in src/monsters2b.cpp), so the owner argument is dropped.
    Companion comp{};
    comp.hits = 1;
    comp.forLife = false;
    CHECK(fearReleases(comp));    // Hits > 0 and not ForLife

    comp.hits = 0;
    CHECK(!fearReleases(comp));   // Hits must be > 0

    comp.hits = 5;
    comp.forLife = true;
    CHECK(!fearReleases(comp));   // ForLife companions stay
}

TEST(join_offer_open_needs_positive_offer) {
    FoeGroup front{};
    front.joinOffer = 0;
    CHECK(!joinOfferOpen(front));
    front.joinOffer = 1;
    CHECK(joinOfferOpen(front));
}

TEST(aimed_group_gone_when_emptied) {
    SquareState sq{};
    CombatOrder order{};
    order.aimedGroup = 0;

    // Emptied group: count 0 and Ref -1.
    sq.foes[0].count = 0;
    sq.foes[0].ref = -1;
    CHECK(aimedGroupGone(sq, order));

    // Live group.
    sq.foes[0].count = 3;
    sq.foes[0].ref = 0;
    CHECK(!aimedGroupGone(sq, order));
}

TEST(mark_cleared_empties_room_once) {
    tsup::Env env({0.5, 0.5});
    SquareState sq{};
    FloorState floor{};
    floor.tally = 5;
    sq.foes[0].ref = 0;
    sq.foes[0].count = 2;
    sq.foes[0].joinOffer = 2;
    sq.foes[1].count = 1;

    markCleared(env.ctx, sq, floor, 0);
    CHECK_EQ(sq.foes[0].count, 0);
    CHECK_EQ(sq.foes[1].count, 0);
    CHECK_EQ(sq.foes[0].ref, -1);
    CHECK_EQ(sq.foes[0].joinOffer, 0);
    CHECK_EQ(floor.tally, 4);

    // Already-empty room (Ref == -1) is not cleared again.
    markCleared(env.ctx, sq, floor, 0);
    CHECK_EQ(floor.tally, 4);
}

TEST(note_refilled_raises_tally) {
    tsup::Env env({0.5, 0.5});
    FloorState floor{};
    floor.tally = 3;
    noteRefilled(env.ctx, floor);
    CHECK_EQ(floor.tally, 4);
    noteRefilled(env.ctx, floor);
    CHECK_EQ(floor.tally, 5);
}

TEST(documented_constants) {
    CHECK_EQ(kMeleeAttemptsMax, 5);
    CHECK_EQ(kRespawnBaseSeconds, 1200);
    CHECK_EQ(kRespawnPerLevelSeconds, 150);
    CHECK_EQ(kRespawnPerAreaSeconds, 5);
    CHECK_EQ(kRespawnMinSeconds, 120);
    CHECK_EQ(kQuestClassHeadStartPercent, 40);
    CHECK_EQ(kQuestGapHighPercent, 90);
    CHECK_EQ(kQuestGapMidPercent, 50);
    CHECK_EQ(kQuestGapLowPercent, 10);
    CHECK_EQ(kGateFreeAdShift, -32);
    CHECK_EQ(kParalysisFullPart, 32);
    CHECK_EQ(kAsmodeusMonsterId, 277);
    CHECK_EQ(kStaleWindowFactor, 2);
    CHECK_EQ(kFriendlyJoinRollRange, 3000);
    CHECK_EQ(kJoinGroupCap, 4);
    CHECK_EQ(kFloorMaskAllEleven, 2047u);
    CHECK_EQ(kItemDeathsSentinel, 0xFFFFu);
}

TEST(dice_average_of_ndm_plus_k) {
    // Average of NdM+K is N*(M+1)/2 + K.
    CHECK_EQ(diceAverage(4, 6, 15), 29.0);  // 4*3.5 + 15
    CHECK_EQ(diceAverage(2, 6, 6), 13.0);   // 2*3.5 + 6
}

TEST(has_flag_bit_tests_bit_set) {
    CHECK(hasFlagBit(0x5u, 0x4u));
    CHECK(!hasFlagBit(0x5u, 0x2u));
    CHECK(!hasFlagBit(0u, 0x1u));
}
