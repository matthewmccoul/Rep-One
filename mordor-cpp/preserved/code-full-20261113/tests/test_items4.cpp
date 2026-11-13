#include "test_support.h"
#include "mordor/items4.h"

// i32 lives in namespace mordor; bring it into scope for this translation unit.
using mordor::i32;

// Helpers local to this translation unit.
static i32 items4_lvl(mordor::IdLevel v) {
    return static_cast<i32>(v);
}

// ---- Constants (SRD T4, T5, T2 and quirk notes) ----

TEST(items4_drop_level_bounds) {
    CHECK_EQ(mordor::kItemDropLevelMin, 1);
    CHECK_EQ(mordor::kItemDropLevelMax, 16);
}

TEST(items4_spell_point_gain_constants) {
    // T2: Scroll of Spells adds 50 SP; Tome of Spells adds 200 SP.
    CHECK_EQ(mordor::kScrollOfSpellsGain, 50);
    CHECK_EQ(mordor::kTomeOfSpellsGain, 200);
}

TEST(items4_identify_constants) {
    // T5: dungeon levels 1-5 use the MOSTLY row; gap of 20 gives BARELY.
    CHECK_EQ(mordor::kIdentifyShallowMaxLevel, 5);
    CHECK_EQ(mordor::kIdentifyBarelyGap, 20);
}

TEST(items4_sentinel_constants) {
    CHECK_EQ(mordor::kNoGuildUse, -1);
    CHECK_EQ(mordor::kDropCategoryRandom, -1);
}

// ---- itemDropMinLevel (T4 author's min-level list) ----

TEST(items4_drop_min_full_table) {
    const i32 expected[16] = {1, 1, 2, 2, 2, 3, 4, 4, 4, 5, 6, 6, 6, 7, 8, 8};
    for (i32 lvl = 1; lvl <= 16; ++lvl) {
        CHECK_EQ(mordor::itemDropMinLevel(lvl), expected[lvl - 1]);
    }
}

TEST(items4_drop_min_low_boundary) {
    // ItemDropLevel 1 and 2 both list a minimum of 1.
    CHECK_EQ(mordor::itemDropMinLevel(1), 1);
    CHECK_EQ(mordor::itemDropMinLevel(2), 1);
    // Step up to 2 at drop level 3.
    CHECK_EQ(mordor::itemDropMinLevel(3), 2);
}

TEST(items4_drop_min_high_boundary) {
    CHECK_EQ(mordor::itemDropMinLevel(15), 8);
    CHECK_EQ(mordor::itemDropMinLevel(16), 8);
}

TEST(items4_drop_min_is_non_decreasing) {
    i32 prev = mordor::itemDropMinLevel(1);
    for (i32 lvl = 2; lvl <= 16; ++lvl) {
        i32 cur = mordor::itemDropMinLevel(lvl);
        CHECK(cur >= prev);
        prev = cur;
    }
}

// ---- identifyLevelFor (T5) ----

TEST(items4_identify_equal_skill_shallow_is_mostly) {
    // skill == difficulty on dungeon level 1-5 gives MOSTLY.
    mordor::IdLevel r = mordor::identifyLevelFor(50, 50, 1);
    CHECK_EQ(items4_lvl(r), items4_lvl(mordor::IdLevel::Mostly));
}

TEST(items4_identify_skill_above_difficulty_shallow_is_mostly) {
    mordor::IdLevel r = mordor::identifyLevelFor(90, 40, 3);
    CHECK_EQ(items4_lvl(r), items4_lvl(mordor::IdLevel::Mostly));
}

TEST(items4_identify_shallow_boundary_level_five) {
    // Level 5 is the last level on the MOSTLY row.
    mordor::IdLevel r = mordor::identifyLevelFor(60, 60, mordor::kIdentifyShallowMaxLevel);
    CHECK_EQ(items4_lvl(r), items4_lvl(mordor::IdLevel::Mostly));
}

TEST(items4_identify_gap_twenty_is_barely) {
    // difficulty - skill == 20 gives BARELY.
    mordor::IdLevel r = mordor::identifyLevelFor(100, 120, 3);
    CHECK_EQ(items4_lvl(r), items4_lvl(mordor::IdLevel::Barely));
}

TEST(items4_identify_gap_far_beyond_twenty_is_barely) {
    mordor::IdLevel r = mordor::identifyLevelFor(10, 80, 2);
    CHECK_EQ(items4_lvl(r), items4_lvl(mordor::IdLevel::Barely));
}

TEST(items4_identify_gap_nineteen_is_not_barely) {
    // difficulty - skill == 19 is below the BARELY threshold (PARTIALLY row).
    // Naming of PARTIAL vs PARTIALLY is unresolved in the SRD, so only
    // assert the result is neither BARELY nor MOSTLY.
    mordor::IdLevel r = mordor::identifyLevelFor(100, 119, 3);
    CHECK(items4_lvl(r) != items4_lvl(mordor::IdLevel::Barely));
    CHECK(items4_lvl(r) != items4_lvl(mordor::IdLevel::Mostly));
}

TEST(items4_identify_gap_one_is_not_mostly) {
    // skill just below difficulty never earns MOSTLY.
    mordor::IdLevel r = mordor::identifyLevelFor(59, 60, 1);
    CHECK(items4_lvl(r) != items4_lvl(mordor::IdLevel::Mostly));
    CHECK(items4_lvl(r) != items4_lvl(mordor::IdLevel::Barely));
}

// ---- capIdentifyLevel (T6: only the General Store reaches ALL) ----

TEST(items4_cap_all_non_general_becomes_mostly) {
    mordor::IdLevel r = mordor::capIdentifyLevel(mordor::IdLevel::All, false);
    CHECK_EQ(items4_lvl(r), items4_lvl(mordor::IdLevel::Mostly));
}

TEST(items4_cap_all_general_store_stays_all) {
    mordor::IdLevel r = mordor::capIdentifyLevel(mordor::IdLevel::All, true);
    CHECK_EQ(items4_lvl(r), items4_lvl(mordor::IdLevel::All));
}

TEST(items4_cap_below_limit_passes_through_non_general) {
    CHECK_EQ(items4_lvl(mordor::capIdentifyLevel(mordor::IdLevel::Barely, false)),
             items4_lvl(mordor::IdLevel::Barely));
    CHECK_EQ(items4_lvl(mordor::capIdentifyLevel(mordor::IdLevel::Partial, false)),
             items4_lvl(mordor::IdLevel::Partial));
    CHECK_EQ(items4_lvl(mordor::capIdentifyLevel(mordor::IdLevel::Mostly, false)),
             items4_lvl(mordor::IdLevel::Mostly));
}

TEST(items4_cap_below_limit_passes_through_general) {
    CHECK_EQ(items4_lvl(mordor::capIdentifyLevel(mordor::IdLevel::Barely, true)),
             items4_lvl(mordor::IdLevel::Barely));
    CHECK_EQ(items4_lvl(mordor::capIdentifyLevel(mordor::IdLevel::Mostly, true)),
             items4_lvl(mordor::IdLevel::Mostly));
}

// ---- addSpellPointsFromItem (T2 consumables) ----

TEST(items4_scroll_of_spells_adds_fifty) {
    tsup::Env env({0.5, 0.9});
    mordor::Character c;
    c.vitals.curSpellPoints = 10;
    c.vitals.maxSpellPoints = 100;
    mordor::addSpellPointsFromItem(env.ctx, c, mordor::kScrollOfSpellsGain);
    CHECK_EQ(c.vitals.curSpellPoints, 60);
}

TEST(items4_tome_of_spells_adds_two_hundred) {
    tsup::Env env({0.5, 0.9});
    mordor::Character c;
    c.vitals.curSpellPoints = 0;
    c.vitals.maxSpellPoints = 300;
    mordor::addSpellPointsFromItem(env.ctx, c, mordor::kTomeOfSpellsGain);
    CHECK_EQ(c.vitals.curSpellPoints, 200);
}

TEST(items4_repeated_scrolls_accumulate) {
    tsup::Env env({0.5, 0.9});
    mordor::Character c;
    c.vitals.curSpellPoints = 0;
    c.vitals.maxSpellPoints = 200;
    mordor::addSpellPointsFromItem(env.ctx, c, mordor::kScrollOfSpellsGain);
    mordor::addSpellPointsFromItem(env.ctx, c, mordor::kScrollOfSpellsGain);
    CHECK_EQ(c.vitals.curSpellPoints, 100);
}
