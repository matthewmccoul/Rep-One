#include <cmath>

#include "test_support.h"
#include "mordor/items3b.h"

using namespace mordor;

namespace {

bool approxEq(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

}  // namespace

// F1: MaxCarried = 39 - wielded - 1 (player-reported; slot count is a conflict,
// so only the stated arithmetic and the named constants are checked here).
TEST(sack_capacity_formula_and_constants) {
    CHECK_EQ(kSackSlotsReported, 39);
    CHECK_EQ(kHandPairDeduction, 1);
    CHECK_EQ(magicSackCapacity(0), 38);
    CHECK_EQ(magicSackCapacity(3), 35);
    CHECK_EQ(magicSackCapacity(38), 0);
}

// F2: maxLvl = ItemDropLevel; minLvl = max(1, Int(maxLvl/2)).
// Only even drop levels are used so floor and banker's rounding agree.
TEST(level_range_plain_even_drop_level) {
    ItemLevelRange r = itemLevelRangeFor(10, ItemSourceKind::Plain);
    CHECK_EQ(r.minLevel, 5);
    CHECK_EQ(r.maxLevel, 10);
}

TEST(level_range_plain_drop_one_minimum_is_one) {
    // Int(1/2) = 0 under either rounding mode; max(1, 0) = 1.
    ItemLevelRange r = itemLevelRangeFor(1, ItemSourceKind::Plain);
    CHECK_EQ(r.minLevel, 1);
    CHECK_EQ(r.maxLevel, 1);
}

TEST(level_range_plain_drop_sixteen) {
    ItemLevelRange r = itemLevelRangeFor(16, ItemSourceKind::Plain);
    CHECK_EQ(r.minLevel, 8);
    CHECK_EQ(r.maxLevel, 16);
}

// Chest: minLvl += 1; maxLvl unchanged.
TEST(level_range_chest_raises_minimum_only) {
    ItemLevelRange r = itemLevelRangeFor(10, ItemSourceKind::Chest);
    CHECK_EQ(r.minLevel, 6);
    CHECK_EQ(r.maxLevel, 10);
}

// Container: minLvl -= 1; maxLvl -= 1.
TEST(level_range_container_lowers_both_bounds) {
    ItemLevelRange r = itemLevelRangeFor(10, ItemSourceKind::Container);
    CHECK_EQ(r.minLevel, 4);
    CHECK_EQ(r.maxLevel, 9);
}

// Asmodeus chest: fixed bounds 14..16 regardless of drop level.
TEST(level_range_asmodeus_chest_fixed_bounds) {
    ItemLevelRange r = itemLevelRangeFor(10, ItemSourceKind::AsmodeusChest);
    CHECK_EQ(r.minLevel, kAsmodeusMinLevel);
    CHECK_EQ(r.maxLevel, kAsmodeusMaxLevel);
    CHECK_EQ(kAsmodeusMinLevel, 14);
    CHECK_EQ(kAsmodeusMaxLevel, 16);
}

// R6: one of the 10 category values is chosen at random (index = below(10)).
// ScriptedRng 0.25 -> Int(2.5) = index 2.
TEST(category_pick_selects_indexed_value) {
    ItemDropRow row;
    row.itemDropLevel = 10;
    for (i32 i = 0; i < kItemCategoryCount; ++i) {
        row.categories[static_cast<u32>(i)] = i;
    }
    tsup::Env env({0.25, 0.25});
    CategoryPick pick = pickItemCategory(env.ctx, row);
    CHECK(pick.kind == CategoryPickKind::Category);
    CHECK_EQ(pick.category, 2);
}

// R6: a value of -1 selects any random category.
// ScriptedRng 0.95 -> index 9.
TEST(category_pick_minus_one_means_any_category) {
    ItemDropRow row;
    row.itemDropLevel = 10;
    for (i32 i = 0; i < kItemCategoryCount; ++i) {
        row.categories[static_cast<u32>(i)] = i;
    }
    row.categories[9] = kAnyCategoryValue;
    tsup::Env env({0.95, 0.95});
    CategoryPick pick = pickItemCategory(env.ctx, row);
    CHECK(pick.kind == CategoryPickKind::AnyCategory);
}

// R6: a value below -1 names a specific item. ScriptedRng 0.85 -> index 8.
TEST(category_pick_below_minus_one_is_fixed_item) {
    ItemDropRow row;
    row.itemDropLevel = 10;
    for (i32 i = 0; i < kItemCategoryCount; ++i) {
        row.categories[static_cast<u32>(i)] = i;
    }
    row.categories[8] = -5;
    tsup::Env env({0.85, 0.85});
    CategoryPick pick = pickItemCategory(env.ctx, row);
    CHECK(pick.kind == CategoryPickKind::FixedItem);
}

// F3: running cumulative percentage; roll r in [0,100).
// Table: item 5 at 30%, item 9 at 70%. Roll 20 -> cum 30 >= 20 -> item 5.
TEST(spawn_roll_first_entry_below_cumulative) {
    Vec<ItemSpawnEntry> table;
    ItemSpawnEntry a;
    a.itemId = 5;
    a.chance = 30.0;
    a.percent = 30.0;
    ItemSpawnEntry b;
    b.itemId = 9;
    b.chance = 70.0;
    b.percent = 70.0;
    table.push_back(a);
    table.push_back(b);

    tsup::Env env({0.2, 0.2});
    ItemSpawnPick pick = rollItemSpawn(env.ctx, table);
    CHECK(pick.spawned);
    CHECK_EQ(pick.itemId, 5);
}

// Roll 50 -> cum 30 < 50 and cum 100 >= 50 -> item 9.
TEST(spawn_roll_falls_through_to_second_entry) {
    Vec<ItemSpawnEntry> table;
    ItemSpawnEntry a;
    a.itemId = 5;
    a.chance = 30.0;
    a.percent = 30.0;
    ItemSpawnEntry b;
    b.itemId = 9;
    b.chance = 70.0;
    b.percent = 70.0;
    table.push_back(a);
    table.push_back(b);

    tsup::Env env({0.5, 0.5});
    ItemSpawnPick pick = rollItemSpawn(env.ctx, table);
    CHECK(pick.spawned);
    CHECK_EQ(pick.itemId, 9);
}

// F4: difficulty = 25 + ((ln(damageMod+1) + 0.3) * ln(levelScale+1) * dungeonLevel).
// With damageMod = 0 and levelScale = 0 the log terms vanish: 25.
TEST(identify_difficulty_base_is_25_without_modifiers) {
    CHECK(approxEq(identifyDifficulty(0.0, 0.0, 7), 25.0));
    CHECK(approxEq(kIdentifyBaseDifficulty, 25.0));
}

// damageMod = e-1 -> ln(e) = 1; levelScale = e-1 -> ln(e) = 1.
// (1 + 0.3) * 1 * 5 = 6.5; 25 + 6.5 = 31.5.
TEST(identify_difficulty_with_unit_logs) {
    double e = std::exp(1.0);
    CHECK(approxEq(identifyDifficulty(e - 1.0, e - 1.0, 5), 31.5));
}

// F4: skill = (INTn + WISn + INTm + WISm) / 2 * ln(((max+5)/10) + 1).
// max = 15 -> (20/10)+1 = 3; sums 10+10+0+0 = 20, /2 = 10.
TEST(identify_skill_guild_factor_fifteen) {
    CHECK(approxEq(identifySkill(10, 10, 0, 0, 15), 10.0 * std::log(3.0)));
}

// max = 5 -> (10/10)+1 = 2; modifiers count: (4+6+3+7)/2 = 10.
TEST(identify_skill_guild_factor_five_with_modifiers) {
    CHECK(approxEq(identifySkill(4, 6, 3, 7, 5), 10.0 * std::log(2.0)));
}

// characterIdentifySkill reads base INT/WIS (StatIndex Int=1, Wis=2) and the
// highest guild level. Stats 10/10, mods 0, Nomad level 5 -> 10 * ln 2.
TEST(character_identify_skill_reads_base_stats_and_guild) {
    Character c;
    c.abilities.stats[static_cast<u32>(StatIndex::Int)] = 10;
    c.abilities.stats[static_cast<u32>(StatIndex::Wis)] = 10;
    c.abilities.statMods[static_cast<u32>(StatIndex::Int)] = 0;
    c.abilities.statMods[static_cast<u32>(StatIndex::Wis)] = 0;
    c.guilds[0].joined = true;
    c.guilds[0].level = 5;
    CHECK(approxEq(characterIdentifySkill(c), 10.0 * std::log(2.0)));
}

// R11 rule 1: skill >= difficulty and dungeon level 1-5 -> MOSTLY.
TEST(identify_level_mostly_at_shallow_depth) {
    CHECK_EQ(identifyLevelFromSkill(3, 30.0, 25.0), mordor::Mostly);
}

// Boundary: skill == difficulty at dungeon level 5 -> MOSTLY.
TEST(identify_level_mostly_boundary_equal_at_level_five) {
    CHECK_EQ(identifyLevelFromSkill(5, 25.0, 25.0), mordor::Mostly);
}

// R11 rule 2: difficulty - skill = 19 (< 20) -> PARTIAL.
TEST(identify_level_partial_gap_nineteen) {
    CHECK_EQ(identifyLevelFromSkill(3, 6.0, 25.0), mordor::Partial);
}

// R11 rule 3: difficulty - skill = 20 (>= 20) -> BARELY.
TEST(identify_level_barely_gap_twenty) {
    CHECK_EQ(identifyLevelFromSkill(3, 5.0, 25.0), mordor::Barely);
}

// R12: each Identify click at the General Store raises by one level; only the
// store reaches ALL. Clicking All stays at All.
TEST(click_step_store_raises_one_level_to_all) {
    CHECK_EQ(identifyClickStep(mordor::Barely, true), mordor::Partial);
    CHECK_EQ(identifyClickStep(mordor::Partial, true), mordor::Mostly);
    CHECK_EQ(identifyClickStep(mordor::Mostly, true), mordor::All);
    CHECK_EQ(identifyClickStep(mordor::All, true), mordor::All);
}

// R12: outside the General Store, identification cannot reach ALL.
TEST(click_step_outside_store_cannot_reach_all) {
    CHECK_EQ(identifyClickStep(mordor::Mostly, false), mordor::Mostly);
}

// F5: useLevel = max(1, round(itemGuildMod * guildLevelMod * 3.97)).
// 1*1*3.97 = 3.97 -> 4. 2*2*3.97 = 15.88 -> 16. 3*3*3.97 = 35.73 -> 36.
TEST(guild_use_level_rounds_product_times_scale) {
    CHECK_EQ(guildUseLevel(1.0, 1.0), 4);
    CHECK_EQ(guildUseLevel(2.0, 2.0), 16);
    CHECK_EQ(guildUseLevel(3.0, 3.0), 36);
    CHECK(approxEq(kGuildUseScale, 3.97));
}

// F5 minimum: max(1, ...). Zero modifiers give 0 -> 1.
// 0.5*0.5*3.97 = 0.9925 rounds to 1 under any mode.
TEST(guild_use_level_minimum_is_one) {
    CHECK_EQ(guildUseLevel(0.0, 0.0), 1);
    CHECK_EQ(guildUseLevel(0.5, 0.5), 1);
    CHECK_EQ(kMinUseLevel, 1);
}
