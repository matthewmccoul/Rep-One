#include "test_support.h"
#include "mordor/monsters2b.h"

using namespace mordor;

// Companion breath chance: base 50; 80 when the lead is more than a quarter
// stronger than the companion; 20 when more than a quarter weaker.
TEST(breath_chance_base_when_strengths_match) {
    CHECK_EQ(companionBreathChance(60, 60), 50);
    CHECK_EQ(companionBreathChance(100, 100), 50);
}

TEST(breath_chance_stronger_and_weaker_lead) {
    CHECK_EQ(companionBreathChance(150, 100), 80);
    CHECK_EQ(companionBreathChance(200, 100), 80);
    CHECK_EQ(companionBreathChance(50, 100), 20);
    CHECK_EQ(companionBreathChance(100, 200), 20);
}

// Exactly a quarter is not "more than a quarter" (125 == 1.25 * 100).
// An 80 vs 100 deficit is exactly 20% and is also not more than a quarter.
// Both readings of "quarter" agree on these values.
TEST(breath_chance_quarter_boundary_is_not_more_than) {
    CHECK_EQ(companionBreathChance(125, 100), 50);
    CHECK_EQ(companionBreathChance(80, 100), 50);
}

TEST(breath_roll_follows_chance) {
    // Roll near 0 succeeds under any positive chance.
    tsup::Env low({0});
    CHECK(companionBreathRoll(low.ctx, 150, 100));   // chance 80
    CHECK(companionBreathRoll(low.ctx, 50, 100));    // chance 20

    // Roll near 1 (about 99) fails under chance 80 and chance 20.
    tsup::Env high({99});
    CHECK(!companionBreathRoll(high.ctx, 150, 100));
    CHECK(!companionBreathRoll(high.ctx, 50, 100));
}

TEST(breath_roll_mid_value_separates_20_from_80) {
    // Rnd is in [0,1) (VB Rnd), so a roll of about 0.50 is 50 on the percent scale.
    // Lead 150 vs companion 100 is more than a quarter stronger: chance 80 [c6].
    tsup::Env strong({0.50});
    CHECK(companionBreathRoll(strong.ctx, 150, 100));

    // Lead 50 vs companion 100 is more than a quarter weaker: chance 20 [c6].
    tsup::Env weak({0.50});
    CHECK(!companionBreathRoll(weak.ctx, 50, 100));
}

TEST(any_class_roll_is_one_percent) {
    tsup::Env hit({0});
    CHECK(rollAnyClass(hit.ctx));

    // Int(0.02 * 100) + 1 = 3, which is not below 2.
    tsup::Env miss({2});
    CHECK(!rollAnyClass(miss.ctx));
}

TEST(lair_draw_is_75_percent) {
    // Rnd is in [0,1) (VB Rnd). Lair draw succeeds below 75 [c7][c24].
    tsup::Env hit({0.0});
    CHECK(rollLairDraw(hit.ctx));

    // Roll of about 0.50 is under 75.
    tsup::Env mid({0.50});
    CHECK(rollLairDraw(mid.ctx));

    // Roll of about 0.99 is not under 75.
    tsup::Env miss({0.99});
    CHECK(!rollLairDraw(miss.ctx));
}

TEST(neutral_lead_peaced_is_40_percent) {
    // Rnd is in [0,1) (VB Rnd). Peaced succeeds below 40 [c7][c24].
    // Roll of about 0.30 is under 40.
    tsup::Env low({0.30});
    CHECK(rollNeutralLeadPeaced(low.ctx));

    // Roll of about 0.50 is not under 40.
    tsup::Env mid({0.50});
    CHECK(!rollNeutralLeadPeaced(mid.ctx));
}

TEST(quest_class_head_start_is_40_percent) {
    // Rnd is in [0,1) (VB Rnd). Class head start succeeds below 40 [c19].
    // Roll of about 0.30 is under 40.
    tsup::Env low({0.30});
    CHECK(rollQuestClassHeadStart(low.ctx));

    // Roll of about 0.50 is not under 40.
    tsup::Env mid({0.50});
    CHECK(!rollQuestClassHeadStart(mid.ctx));
}

TEST(attempts_per_group_capped_at_five) {
    CHECK_EQ(attemptsPerGroup(0), 0);
    CHECK_EQ(attemptsPerGroup(3), 3);
    CHECK_EQ(attemptsPerGroup(5), 5);
    CHECK_EQ(attemptsPerGroup(9), 5);
}

TEST(at_nought_member_still_counts_as_alive) {
    Creature c;
    c.alive = true;
    c.atNought = true;
    c.hits = 0;
    CHECK(countsAsAlive(c));

    Creature gone;
    gone.alive = false;
    gone.atNought = false;
    gone.hits = 0;
    CHECK(!countsAsAlive(gone));
}

TEST(dead_character_ends_blow_sequence_alive_does_not) {
    CHECK(!endsBlowSequence(DeathState::Alive));
    CHECK(endsBlowSequence(DeathState::Dead));
}

TEST(charm_forces_join_only_when_damage_exceeds_remaining_hits) {
    CHECK(charmForcesJoin(10, 9));
    CHECK(!charmForcesJoin(9, 9));
    CHECK(!charmForcesJoin(5, 9));
}

TEST(fear_releases_non_forlife_companion_with_hits) {
    Companion c;
    c.hits = 5;
    c.forLife = false;
    CHECK(fearReleases(c));

    c.hits = 0;
    CHECK(!fearReleases(c));

    c.hits = 3;
    c.forLife = true;
    CHECK(!fearReleases(c));
}

TEST(newbie_protection_level_one_below_area_18) {
    Character ch;
    ch.level = 1;
    ch.guilds[0].level = 1;
    CHECK(newbieProtected(ch, 17));
    CHECK(!newbieProtected(ch, 18));

    ch.level = 2;
    ch.guilds[0].level = 2;
    CHECK(!newbieProtected(ch, 5));
}

TEST(asmodeus_identity_is_by_id_only) {
    Creature c;
    c.id = 277;
    c.monsterClass = 5;
    CHECK(isAsmodeus(c));

    c.id = 276;
    CHECK(!isAsmodeus(c));

    // Morgul and Shadow Ninja share Id 307; neither is Asmodeus.
    c.id = 307;
    CHECK(!isAsmodeus(c));
}

TEST(spawn_mask_bit_n_is_kind_n) {
    CHECK(spawnMaskAllowsKind(1 << 2, 2));    // Demon
    CHECK(!spawnMaskAllowsKind(1 << 2, 3));   // Devil not set
    CHECK(spawnMaskAllowsKind(1 << 17, 17));  // Indigini
    CHECK(!spawnMaskAllowsKind(0, 0));
}

// Trap-table floor values read as bitmasks: floor n is bit n-1.
TEST(floor_mask_reading_from_trap_table) {
    // Gargantuan: value 1 is floor 1 only.
    CHECK(floorMaskAllows(1, 1));
    CHECK(!floorMaskAllows(1, 2));

    // Phoenix: value 64 is bit 6, floor 7 only.
    CHECK(floorMaskAllows(64, 7));
    CHECK(!floorMaskAllows(64, 6));

    // Kobold: value 31 is floors 1 through 5.
    CHECK(floorMaskAllows(31, 5));
    CHECK(!floorMaskAllows(31, 6));
}

TEST(pariah_eligible_on_levels_12_to_15) {
    CHECK(!pariahLevelEligible(11));
    CHECK(pariahLevelEligible(12));
    CHECK(pariahLevelEligible(15));
    CHECK(!pariahLevelEligible(16));
}

// Respawn: base 1200 + 150 per floor - 5 per room of shortfall; floor minimum 120.
TEST(respawn_delay_probe_values_and_floor_clamp) {
    CHECK_EQ(respawnDelaySeconds(1, 0), 1350);
    CHECK_EQ(respawnDelaySeconds(1, 1), 1345);
    CHECK_EQ(respawnDelaySeconds(1, 10), 1300);
    CHECK_EQ(respawnDelaySeconds(15, 0), 3450);

    // 1350 - 5000 is below the 120 s floor.
    CHECK_EQ(respawnDelaySeconds(1, 1000), 120);
}

// Stale when older than 2t: age must be strictly greater than 2 * respawnT.
TEST(standing_group_stale_when_older_than_twice_t) {
    CHECK(standingGroupStale(1000, 0, 400));  // age 1000 > 800
    CHECK(!standingGroupStale(800, 0, 400));  // age exactly 2t
    CHECK(!standingGroupStale(700, 0, 400));
}

// JoinOffer is read only from group 0.
TEST(join_offer_read_only_from_group_zero) {
    Encounter e;
    e.groups[1].joinOffer = true;
    CHECK(!readJoinOffer(e));

    e.groups[0].joinOffer = true;
    CHECK(readJoinOffer(e));

    e.groups[0].joinOffer = false;
    CHECK(!readJoinOffer(e));
}

TEST(widen_spread_steps_by_one) {
    tsup::Env env({50});
    Encounter e;
    e.spread = 0;
    widenEncounterSpread(env.ctx, e);
    CHECK_EQ(e.spread, 1);
    widenEncounterSpread(env.ctx, e);
    CHECK_EQ(e.spread, 2);
}

// Stud squares raise the target floor by one.
TEST(encounter_target_floor_adds_one_on_stud) {
    CHECK_EQ(encounterTargetFloor(5, false), 5);
    CHECK_EQ(encounterTargetFloor(5, true), 6);
}

TEST(library_knowledge_scale_mapping) {
    CHECK_EQ(static_cast<i32>(libraryKnowledgeFor(0)),
             static_cast<i32>(LibraryKnowledge::UNKNOWN_STATE));
    CHECK_EQ(static_cast<i32>(libraryKnowledgeFor(1)),
             static_cast<i32>(LibraryKnowledge::SEEN));
    CHECK_EQ(static_cast<i32>(libraryKnowledgeFor(2)),
             static_cast<i32>(LibraryKnowledge::DEFEATED));
    CHECK_EQ(static_cast<i32>(libraryKnowledgeFor(3)),
             static_cast<i32>(LibraryKnowledge::FULLY_KNOWN));
    CHECK_EQ(static_cast<i32>(libraryKnowledgeFor(4)),
             static_cast<i32>(LibraryKnowledge::FULLY_KNOWN));
}

// Companion spawn mode table: modes 0-3 and 7 are window kinds;
// modes 4, 5 and (mode - 7) above 7 name table indices.
TEST(companion_spawn_window_mode_table) {
    CompanionSpawnWindow w0 = companionSpawnWindow(0);
    CHECK_EQ(static_cast<i32>(w0.kind), static_cast<i32>(SpawnWindowKind::FloorOffsets));
    CHECK_EQ(w0.offsetLo, -3);
    CHECK_EQ(w0.offsetHi, -1);

    CompanionSpawnWindow w1 = companionSpawnWindow(1);
    CHECK_EQ(static_cast<i32>(w1.kind), static_cast<i32>(SpawnWindowKind::FloorOffsets));
    CHECK_EQ(w1.offsetLo, -3);
    CHECK_EQ(w1.offsetHi, 0);

    CompanionSpawnWindow w2 = companionSpawnWindow(2);
    CHECK_EQ(static_cast<i32>(w2.kind), static_cast<i32>(SpawnWindowKind::FloorOffsets));
    CHECK_EQ(w2.offsetLo, 0);
    CHECK_EQ(w2.offsetHi, 0);

    CompanionSpawnWindow w3 = companionSpawnWindow(3);
    CHECK_EQ(static_cast<i32>(w3.kind), static_cast<i32>(SpawnWindowKind::RepeatPrevious));

    CompanionSpawnWindow w7 = companionSpawnWindow(7);
    CHECK_EQ(static_cast<i32>(w7.kind), static_cast<i32>(SpawnWindowKind::KeepPrevious));

    CompanionSpawnWindow w4 = companionSpawnWindow(4);
    CHECK_EQ(static_cast<i32>(w4.kind), static_cast<i32>(SpawnWindowKind::TableIndex));
    CHECK_EQ(w4.tableIndex, 4);

    CompanionSpawnWindow w5 = companionSpawnWindow(5);
    CHECK_EQ(static_cast<i32>(w5.kind), static_cast<i32>(SpawnWindowKind::TableIndex));
    CHECK_EQ(w5.tableIndex, 5);

    CompanionSpawnWindow w9 = companionSpawnWindow(9);
    CHECK_EQ(static_cast<i32>(w9.kind), static_cast<i32>(SpawnWindowKind::TableIndex));
    CHECK_EQ(w9.tableIndex, 2);
}

// A lair monster is one whose Id equals the area's LairId.
TEST(lair_monster_matches_area_lair_id) {
    Area a;
    a.lairId = 307;

    Creature c;
    c.id = 307;
    CHECK(isLairMonster(a, c));

    c.id = 306;
    CHECK(!isLairMonster(a, c));
}
