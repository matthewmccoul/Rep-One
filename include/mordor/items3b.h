#pragma once
// SRD part c13p3: items and equipment. Carrying capacity, item generation,
// identification, guild use, charge and stat items, Thieving Ability.
// Declarations only. Sources are archive, forum and wiki material; no engine
// source was provided, so every routine here is unverified against code.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"

namespace mordor {

// ---- constants ----

// R1, R2, F1: player-reported magic sack slot count.
// UNKNOWN: 39 is a forum figure (attributed to a developer comment in one
// source and to a player post in another). Original count not confirmed.
inline constexpr i32 kSackSlotsReported = 39;

// F1: deduction for "one pair of hands".
// CONFLICT: Inventory.freeHands is 2 (one per hand). F1 text subtracts one
// pair. Header follows the F1 text literally; implementers may compare with
// Inventory.freeHands when the original behaviour is confirmed.
inline constexpr i32 kHandPairDeduction = 1;

// R6: number of category values in the item drop row.
inline constexpr i32 kItemCategoryCount = 10;

// R6: category value meaning "any random category".
inline constexpr i32 kAnyCategoryValue = -1;

// F2: lowest item level ever produced.
inline constexpr i32 kMinItemLevel = 1;

// F2: Asmodeus chest fixed level band.
inline constexpr i32 kAsmodeusMinLevel = 14;
inline constexpr i32 kAsmodeusMaxLevel = 16;

// F2: chest raises the minimum level by one.
inline constexpr i32 kChestMinLevelShift = 1;

// F2: container lowers both bounds by one.
inline constexpr i32 kContainerLevelShift = 1;

// F4: base of the identification difficulty formula.
inline constexpr double kIdentifyBaseDifficulty = 25.0;

// F4: additive bias inside the difficulty bracket.
inline constexpr double kIdentifyLnBias = 0.3;

// F4: guild level offset and divisor in the skill formula.
inline constexpr double kIdentifyGuildOffset = 5.0;
inline constexpr double kIdentifyGuildDivisor = 10.0;

// R11: MOSTLY needs skill >= difficulty and dungeon level at or below this.
// UNKNOWN: result when skill >= difficulty below level 1 or above this level
// is not stated. Header falls through to the gap test (see identifyLevelFromSkill).
inline constexpr i32 kIdentifyMostlyMaxDungeonLevel = 5;

// R11: PARTIAL when (difficulty - skill) is below this; BARELY at or above it.
inline constexpr double kIdentifyPartialGap = 20.0;

// F5: scale in the guild use-level formula.
inline constexpr double kGuildUseScale = 3.97;

// F5: floor of the guild use level.
inline constexpr i32 kMinUseLevel = 1;

// R16: spell points added on use.
inline constexpr i32 kScrollOfSpellsSpBonus = 50;
inline constexpr i32 kTomeOfSpellsSpBonus = 200;

// ---- enums ----

// F2 input: which container or chest modifier applies to the item level range.
enum class ItemSourceKind : i32 {
    Plain = 0,
    Chest = 1,
    Container = 2,
    AsmodeusChest = 3
};

// R6 outcome of choosing a category from the drop row.
enum class CategoryPickKind : i32 {
    Category = 0,
    AnyCategory = 1,
    FixedItem = 2
};

// ---- result and input structs ----

// F2 output: inclusive item level band used by R7.
struct ItemLevelRange {
    i32 minLevel = kMinItemLevel;
    i32 maxLevel = kMinItemLevel;
};

// R6 input: one row of Wabbit's Editor item drop data.
// categories[k] is the k-th of the 10 values below Item Drop Level.
struct ItemDropRow {
    i32 itemDropLevel = 0;
    Arr<i32, kItemCategoryCount> categories{};
};

// R6 output.
// UNKNOWN: mapping of a fixed-item value (below -1) to an item id. Header
// assumes fixedItemId = -value.
struct CategoryPick {
    CategoryPickKind kind = CategoryPickKind::Category;
    i32 category = 0;
    i32 fixedItemId = -1;
};

// R8 output: one candidate with its Chance and derived spawn percentage (F3).
struct ItemSpawnEntry {
    i32 itemId = 0;
    double chance = 0.0;
    double percent = 0.0;
};

// R9 output. roll is the draw in [0, 100).
struct ItemSpawnPick {
    bool spawned = false;
    i32 itemId = -1;
    double roll = 0.0;
};

// R15 output.
struct ChargeCastResult {
    bool cast = false;
    i32 spellId = 0;
    i32 spellLevel = 0;
    i32 timesCast = 0;
    i32 chargesLeft = 0;
};

// ---- rule routines ----

// R1 carried count limit for the magic sack, computed as F1.
// Rule text is player-reported; see kSackSlotsReported.
i32 magicSackCapacity(i32 itemsWielded);

// R2 same deduction as R1: wielded items and one pair of hands come off the slot count.
// Declared as R1 (magicSackCapacity); no separate routine.

// R3 worn Blessings of Morash limit. UNKNOWN: limit for Demise not stated.
// No routine declared until the limit is known.

// R4 cards summoned from the questmaster's house. UNCONFIRMED player claim
// (one post). No routine declared.

// R5 item level band for a drop, including chest or container modifiers (F2).
// CONFLICT: F2 is written with Int(maxLvl / 2), which is floor for positive values.
// Author's table (T4) matches round-half-to-even. This header declares the written
// formula; the implementation uses vbInt unless the table is confirmed as authoritative.
ItemLevelRange itemLevelRangeFor(i32 itemDropLevel, ItemSourceKind source);

// R6 chooses one category from the drop row at random.
// A value of kAnyCategoryValue selects any category (caller resolves).
// A value below -1 names a fixed item that always spawns.
CategoryPick pickItemCategory(Ctx& ctx, const ItemDropRow& row);

// R7 candidate item ids for a category and level band.
// Scan item ids from the first id that matches both the min-level bound and the
// category, stopping before the first later id that fails the max-level bound
// or the category.
// UNKNOWN: whether the stop test is evaluated on the start id itself is not stated.
// UNKNOWN: category kAnyCategoryValue is not valid here; caller must resolve it first.
Vec<i32> listCandidateItems(const Ctx& ctx, const ItemLevelRange& range, i32 category);

// R8 spawn percentages for candidates: pct = Chance / sum(Chance) * 100 (F3).
// Chance values are read from ctx.data.items.
// UNKNOWN: result when the sum of Chance is zero is not stated; return an empty table.
Vec<ItemSpawnEntry> buildSpawnTable(const Ctx& ctx, const Vec<i32>& candidateIds);

// R9 draws a roll in [0, 100) and spawns the first entry whose running sum
// reaches the roll (roll <= running sum). Empty table gives spawned = false.
ItemSpawnPick rollItemSpawn(Ctx& ctx, const Vec<ItemSpawnEntry>& table);

// R10 / F4 identification difficulty of an item.
// Pure: damageMod and levelScale come from the item's Wabbit's Editor data.
double identifyDifficulty(double damageMod, double levelScale, i32 dungeonLevel);

// R10 / F4 identification skill from explicit inputs.
// Natural and modifier INT and WIS, plus the highest guild level.
// UNKNOWN: whether the division by 2 and by 10 is integer or real division.
// Header uses real division.
double identifySkill(i32 naturalInt, i32 naturalWis, i32 modInt, i32 modWis, i32 maxGuildLevel);

// R10 / F4 identification skill of a character.
// Natural stats from abilities.stats, modifiers from abilities.statMods,
// highest guild level from guilds.
double characterIdentifySkill(const Character& c);

// R11 result rules, applied in order:
//   skill >= difficulty and dungeonLevel in [1, 5]  -> Mostly
//   difficulty - skill < 20                          -> Partial
//   difficulty - skill >= 20                         -> Barely
// UNKNOWN: result when skill >= difficulty at dungeon level 6 or deeper is not stated.
//   Header falls through to the gap test, which gives Partial.
// UNKNOWN: whether PARTIALLY (rules) and PARTIAL (level list) are the same level.
//   Header maps both to IdLevel::Partial.
IdLevel identifyLevelFromSkill(i32 dungeonLevel, double skill, double difficulty);

// R10 identification on find or trade: compares skill with difficulty (F4) and applies R11.
// Reads the item's ItemDef from ctx.data.items.
IdLevel identifyItemOnFind(const Ctx& ctx, const Character& finder, const Item& item, i32 dungeonLevel);

// R12 one General Store Identify click raises identification by one level.
// Only the General Store can reach IdLevel::All.
// UNKNOWN: cap for non-store identification is not stated; header caps at Mostly.
IdLevel identifyClickStep(IdLevel current, bool atGeneralStore);

// R13 exchange-form recalculation. Uses the item whose id equals the current
// item's levelScale. Applies only to the exchange form, not drag-and-drop.
// Example: a Barely item passed to a Partial-capable recipient can become Mostly.
// UNKNOWN: drag-and-drop behaviour beyond "not the exchange form".
// UNKNOWN: source of dungeonLevel for an exchanged item; passed explicitly.
IdLevel exchangeIdentifyLevel(const Ctx& ctx, const Item& item, const Character& recipient, i32 dungeonLevel);

// R14 guild use level (F5): max(1, round(itemGuildMod * guildLevelMod * 3.97)).
// The canUse test (guildLevel >= useLevel) is CanUse in guilds3.h; not redeclared here.
// UNKNOWN: rounding mode of round() in F5. Header uses roundHalfEven.
i32 guildUseLevel(double itemGuildMod, double guildLevelMod);

// R15 charge items cast the stated spell a fixed number of times at a fixed
// spell level. Spell and charge data come from the item definition.
ChargeCastResult castChargeItem(Ctx& ctx, Character& user, Item& item, const ItemDef& def);

// R16 items with spell-point bonuses add SP on use (Scroll 50, Tome 200).
// Returns the new curSpellPoints.
// UNKNOWN: whether the added points are capped at maxSpellPoints is not stated.
i32 addItemSpellPoints(Ctx& ctx, Character& user, i32 points);

// R17 equipping an item applies its stat modifiers to abilities.statMods.
// equipping = false removes them.
// UNKNOWN: removal on unequip is not stated in the source; header assumes it is symmetric.
void applyItemStatMods(Ctx& ctx, Character& c, const ItemDef& def, bool equipping);

// R18 / F6 Thieving Ability, normalized 0-100, shown on the Guild tab.
// Uses Dex/2 + Wis/2 in X, Y and Z.
// CONFLICT: Y and Z are written with Dex/4 (and Z's Dex 30 / Wis 40 inputs are
// listed in the reverse order of the R18 reachability note). Header applies one
// consistent expression to all three.
// CONFLICT: thieving disarm uses Wis/4 in a different source; not used here.
// UNKNOWN: whether thievingMod is a constant or from item data.
i32 thievingAbility(i32 thiefGuildLevel, i32 dex, i32 wis, double thievingMod);

}  // namespace mordor
