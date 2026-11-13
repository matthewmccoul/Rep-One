#include "test_support.h"
#include "mordor/monsters2a.h"

#include <cstring>
#include <vector>

using mordor::MonsterKind;
using mordor::f64;  // FIXED: f64 is declared in mordor, not in the global scope.

// UNRESOLVED: test_support.h was not provided, so the constructor signature of
// tsup::Env is unknown. The compiler rejected tsup::Env(std::vector<double>).
// Each Env below now takes a braced single-value list, Env({v}), which matches
// an initializer_list or vector parameter. If the compiler still rejects these
// lines, read the Env constructor in test_support.h and adjust the argument
// form to match. The roll values and test expectations are unchanged.

TEST(monsters2a_constants_match_srd) {
    CHECK_EQ(mordor::kMonsterClassCount, 18);
    CHECK_EQ(mordor::kLairNamedPercent, 75);
    CHECK_EQ(mordor::kAnyClassRange, 100);
    CHECK_EQ(mordor::kAnyClassThreshold, 2);
    CHECK_EQ(mordor::kNeutralLeadPeacedPercent, 40);
    CHECK_EQ(mordor::kRareChanceBelow, 10);
    CHECK_EQ(mordor::kMaxJoinOffersPerGroup, 4);
    CHECK_EQ(mordor::kCritK, 650);
    CHECK_EQ(mordor::kCritLo, 2);
    CHECK_EQ(mordor::kCritHi, 50);
    CHECK_EQ(mordor::kBackstabK, 500);
    CHECK_EQ(mordor::kBackstabLo, 4);
    CHECK_EQ(mordor::kBackstabHi, 75);
    CHECK_EQ(mordor::kAttemptsPerGroupCap, 5);
    CHECK_EQ(mordor::kRespawnPerEmptiedRoomSeconds, 5);
    CHECK_EQ(mordor::kAllElevenFlagsMask, 2047);
}

TEST(monster_kind_names_table_entries) {
    CHECK(std::strcmp(mordor::kMonsterKindNames[0], "Humanoid") == 0);
    CHECK(std::strcmp(mordor::kMonsterKindNames[6], "Dragon") == 0);
    CHECK(std::strcmp(mordor::kMonsterKindNames[17], "Indigini") == 0);
}

TEST(spawn_mask_bit_n_is_kind_n) {
    CHECK(mordor::spawnMaskAllowsKind(1 << 2, MonsterKind::Demon));
    CHECK(!mordor::spawnMaskAllowsKind(1 << 2, MonsterKind::Humanoid));
    CHECK(mordor::spawnMaskAllowsKind(1 << 17, MonsterKind::Indigini));
    CHECK(!mordor::spawnMaskAllowsKind(0, MonsterKind::Humanoid));
}

TEST(spawn_mask_2047_covers_kinds_0_to_10_only) {
    for (int k = 0; k <= 10; ++k) {
        CHECK(mordor::spawnMaskAllowsKind(2047, static_cast<MonsterKind>(k)));
    }
    CHECK(!mordor::spawnMaskAllowsKind(2047, MonsterKind::Giant));
    CHECK(!mordor::spawnMaskAllowsKind(2047, MonsterKind::Indigini));
}

TEST(lair_roll_is_75_percent_band) {
    tsup::Env low({0.1});
    CHECK(mordor::rollLairNamed(low.ctx));
    tsup::Env high({0.99});
    CHECK(!mordor::rollLairNamed(high.ctx));
}

TEST(any_class_roll_hits_only_first_percent) {
    tsup::Env hit({0.0099});
    CHECK(mordor::rollAnyClass(hit.ctx));
    tsup::Env miss2({0.02});
    CHECK(!mordor::rollAnyClass(miss2.ctx));
    tsup::Env miss({0.5});
    CHECK(!mordor::rollAnyClass(miss.ctx));
}

TEST(neutral_lead_peaced_is_40_percent) {
    tsup::Env yes({0.1});
    CHECK(mordor::rollNeutralLeadPeaced(yes.ctx));
    tsup::Env no({0.9});
    CHECK(!mordor::rollNeutralLeadPeaced(no.ctx));
}

TEST(quest_class_head_start_is_40_percent) {
    tsup::Env yes({0.1});
    CHECK(mordor::rollQuestClassHeadStart(yes.ctx));
    tsup::Env no({0.9});
    CHECK(!mordor::rollQuestClassHeadStart(no.ctx));
}

TEST(level_gap_gate_bands_90_50_10) {
    // SRD-AMBIGUOUS: SRD [c19] gives the gate values 90/50/10 but not the band edges. Bands follow levelGapGatePercent as implemented.
    CHECK_EQ(mordor::levelGapGatePercent(0), 90);
    CHECK_EQ(mordor::levelGapGatePercent(1), 50);
    CHECK_EQ(mordor::levelGapGatePercent(2), 50);
    CHECK_EQ(mordor::levelGapGatePercent(3), 10);
    CHECK_EQ(mordor::levelGapGatePercent(20), 10);
}

TEST(rare_monster_is_chance_below_10) {
    CHECK(mordor::isRareMonsterChance(0));
    CHECK(mordor::isRareMonsterChance(9));
    CHECK(!mordor::isRareMonsterChance(10));
    CHECK(!mordor::isRareMonsterChance(75));
}

TEST(companion_breath_base_50_within_a_quarter) {
    CHECK_EQ(mordor::companionBreathChance(100, 100), 50);
    CHECK_EQ(mordor::companionBreathChance(125, 100), 50);
    CHECK_EQ(mordor::companionBreathChance(100, 125), 50);
}

TEST(companion_breath_80_when_lead_more_than_quarter_stronger) {
    CHECK_EQ(mordor::companionBreathChance(126, 100), 80);
    CHECK_EQ(mordor::companionBreathChance(100, 50), 80);
}

TEST(companion_breath_20_when_lead_more_than_quarter_weaker) {
    CHECK_EQ(mordor::companionBreathChance(90, 125), 20);
    CHECK_EQ(mordor::companionBreathChance(10, 100), 20);
}

TEST(attempts_for_group_is_living_count_capped_at_5) {
    CHECK_EQ(mordor::attemptsForGroup(0), 0);
    CHECK_EQ(mordor::attemptsForGroup(1), 1);
    CHECK_EQ(mordor::attemptsForGroup(4), 4);
    CHECK_EQ(mordor::attemptsForGroup(5), 5);
    CHECK_EQ(mordor::attemptsForGroup(9), 5);
    CHECK_EQ(mordor::attemptsForGroup(30), 5);
}

TEST(fear_releases_needs_hits_and_not_for_life) {
    mordor::Character c;
    c.vitals.curHits = 5;
    CHECK(mordor::fearReleases(c));
    c.vitals.curHits = 0;
    CHECK(!mordor::fearReleases(c));
    c.vitals.curHits = 5;
    c.forLife = true;
    CHECK(!mordor::fearReleases(c));
}

TEST(clamp_crit_chance_to_2_through_50) {
    CHECK_EQ(mordor::clampCritChance(-5), 2);
    CHECK_EQ(mordor::clampCritChance(1), 2);
    CHECK_EQ(mordor::clampCritChance(2), 2);
    CHECK_EQ(mordor::clampCritChance(30), 30);
    CHECK_EQ(mordor::clampCritChance(50), 50);
    CHECK_EQ(mordor::clampCritChance(51), 50);
    CHECK_EQ(mordor::clampCritChance(99), 50);
}

TEST(clamp_backstab_chance_to_4_through_75) {
    CHECK_EQ(mordor::clampBackstabChance(3), 4);
    CHECK_EQ(mordor::clampBackstabChance(4), 4);
    CHECK_EQ(mordor::clampBackstabChance(40), 40);
    CHECK_EQ(mordor::clampBackstabChance(75), 75);
    CHECK_EQ(mordor::clampBackstabChance(76), 75);
}

TEST(companion_spawn_floor_window_modes_0_to_2) {
    auto s0 = mordor::companionSpawnSelection(0);
    CHECK_EQ(s0.floorOffsetMin, -3);
    CHECK_EQ(s0.floorOffsetMax, -1);
    CHECK(!s0.repeatPrevious);
    CHECK(!s0.keepPrevious);

    auto s1 = mordor::companionSpawnSelection(1);
    CHECK_EQ(s1.floorOffsetMin, -3);
    CHECK_EQ(s1.floorOffsetMax, 0);

    auto s2 = mordor::companionSpawnSelection(2);
    CHECK_EQ(s2.floorOffsetMin, 0);
    CHECK_EQ(s2.floorOffsetMax, 0);
}

TEST(companion_spawn_repeat_and_keep_modes) {
    auto s3 = mordor::companionSpawnSelection(3);
    CHECK(s3.repeatPrevious);
    CHECK(!s3.keepPrevious);
    CHECK_EQ(s3.tableIndex, -1);

    auto s7 = mordor::companionSpawnSelection(7);
    CHECK(s7.keepPrevious);
    CHECK(!s7.repeatPrevious);
    CHECK_EQ(s7.tableIndex, -1);
}

TEST(companion_spawn_table_index_modes) {
    CHECK_EQ(mordor::companionSpawnSelection(4).tableIndex, 4);
    CHECK_EQ(mordor::companionSpawnSelection(5).tableIndex, 5);
    CHECK_EQ(mordor::companionSpawnSelection(8).tableIndex, 1);
    CHECK_EQ(mordor::companionSpawnSelection(10).tableIndex, 3);
    CHECK_EQ(mordor::companionSpawnSelection(14).tableIndex, 7);
}





TEST(standing_group_stale_when_older_than_2t) {
    // Signature is standingGroupStale(nowSeconds, stampSeconds, respawnT). The stamp is the earlier time, so "now" comes first.
    CHECK(!mordor::standingGroupStale(2000, 0, 1000));
    CHECK(mordor::standingGroupStale(2001, 0, 1000));
    CHECK(!mordor::standingGroupStale(500, 0, 1000));
}

TEST(floor_mask_bit_n_minus_1_places_floor_n) {
    // Kobold: 31 = 0b11111 -> floors 1-5
    CHECK(mordor::floorMaskPlaces(31, 1));
    CHECK(mordor::floorMaskPlaces(31, 5));
    CHECK(!mordor::floorMaskPlaces(31, 6));
    // Twisted Dwarf: 15 -> floors 1-4
    CHECK(mordor::floorMaskPlaces(15, 4));
    CHECK(!mordor::floorMaskPlaces(15, 5));
    // Gargantuan: 1 -> floor 1 only
    CHECK(mordor::floorMaskPlaces(1, 1));
    CHECK(!mordor::floorMaskPlaces(1, 2));
}

TEST(floor_mask_non_contiguous_and_single_floor) {
    // Slave Masher: 29 = 0b11101 -> floors 1, 3, 4, 5 (not 2)
    CHECK(mordor::floorMaskPlaces(29, 1));
    CHECK(!mordor::floorMaskPlaces(29, 2));
    CHECK(mordor::floorMaskPlaces(29, 3));
    CHECK(mordor::floorMaskPlaces(29, 5));
    // Phoenix: 64 = bit 6 -> floor 7 only
    CHECK(mordor::floorMaskPlaces(64, 7));
    CHECK(!mordor::floorMaskPlaces(64, 6));
    CHECK(!mordor::floorMaskPlaces(64, 1));
    // 2047 covers floors 1 through 11
    CHECK(mordor::floorMaskPlaces(2047, 1));
    CHECK(mordor::floorMaskPlaces(2047, 11));
}
