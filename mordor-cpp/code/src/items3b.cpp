// items3b.cpp: item spawn, identification, guild use, charge items, stat mods, thieving.
// Implements include/mordor/items3b.h. SRD rules R1-R18 and formulas F1-F6 (Part C).
//
// BUILD STATUS: NOT YET COMPILING. The ItemDef members 'category', 'level', 'chance' and
// 'timesCast' do not exist in ItemDef (game_data.h, not supplied). Their correct names must be
// confirmed from that header before these lines can compile. No names were guessed.
//
// ASSUMED (not supplied in the brief; reconcile with game_data.h, events.h, rng.h):
//   ItemDef fields: level (i32), category (i32), chance (double), damageMod (double),
//     levelScale (double; used as an item index in R13), statMods (Arr<i32,kStatCount>),
//     spellId (i32), spellLevel (i32), timesCast (i32).
//   GameData::items is an indexable Vec<ItemDef>; an item's id is its index in that Vec.
//   IEventSink nums are Vec<i64>; strs are Vec<Str>.
//   IRng::below(n) returns an integer in [0,n); IRng::nextDouble() returns [0,1).
//   Abilities::stats holds natural stats; Abilities::statMods holds modifier stats.

#include <cmath>

#include "mordor/items3b.h"

namespace mordor {
namespace {

// R11: "dungeon level 1-5" for the MOSTLY band. Lower bound.
constexpr i32 kMostlyMinDungeonLevel = 1;

const ItemDef& itemDefAt(const Ctx& ctx, i32 id) {
    return ctx.data.items[static_cast<u64>(id)];
}

// Item table fields, as the reference reads them (Loot.cs item drop): category is the item Type, level is the
// Floor the item is found from, and the chance weight is Rarity.
i32 defCategory(const ItemDef& def) {
    return def.type;
}

i32 defLevel(const ItemDef& def) {
    return def.floor;
}

double defChance(const ItemDef& def) {
    return static_cast<double>(def.rarity);
}

i32 defTimesCast(const ItemDef&) {
    return 0;
}

// R6/R7: an any-category value (-1) matches every category.
bool categoryMatches(const ItemDef& def, i32 category) {
    return category == kAnyCategoryValue || defCategory(def) == category;
}

// F6 constant: LN(2) * 4 * LN(2) * 1.25 * 1.25 * 1.5
double thievingPenalty() {
    const double ln2 = std::log(2.0);
    return ln2 * 4.0 * ln2 * 1.25 * 1.25 * 1.5;
}

// F6 body for one input set: dexTerm + wisTerm + (4.5 + EXP(LN(GLvl+1)/LN(15))) * ((Mod/10)*14) - penalty
double thievingScore(double dexTerm, double wisTerm, double glvl, double mod) {
    const double levelTerm = 4.5 + std::exp(std::log(glvl + 1.0) / std::log(15.0));
    return dexTerm + wisTerm + levelTerm * ((mod / 10.0) * 14.0) - thievingPenalty();
}

}  // namespace

// F1 (R1, R2): magic sack capacity. Player-reported figure.
// CONFLICT: the 39-slot count is attributed to a developer comment in c102 and to a player post in c103.
// UNKNOWN: no clamp is stated; the result is negative if itemsWielded exceeds 38. Kept as written.
i32 magicSackCapacity(i32 itemsWielded) {
    return kSackSlotsReported - itemsWielded - kHandPairDeduction;
}

// F2 (R5): item level range from ItemDropLevel and the container source.
ItemLevelRange itemLevelRangeFor(i32 itemDropLevel, ItemSourceKind source) {
    ItemLevelRange range{};
    if (source == ItemSourceKind::AsmodeusChest) {
        // F2: Asmodeus chest fixed range.
        range.minLevel = kAsmodeusMinLevel;
        range.maxLevel = kAsmodeusMaxLevel;
        return range;
    }

    i32 maxLvl = itemDropLevel;
    // F2: minLvl = max(1, Int(maxLvl / 2)).
    // CONFLICT: the formula as written uses Int() (floor). The author's listed minimums match
    // round-half-to-even instead. The formula as written is used here.
    i32 minLvl = static_cast<i32>(vbInt(static_cast<double>(maxLvl) / 2.0));
    if (minLvl < kMinItemLevel) {
        minLvl = kMinItemLevel;
    }

    if (source == ItemSourceKind::Chest) {
        // F2: Chest: minLvl += 1.
        minLvl += kChestMinLevelShift;
    } else if (source == ItemSourceKind::Container) {
        // F2: Container: minLvl -= 1; maxLvl -= 1.
        // UNKNOWN: no re-clamp to kMinItemLevel after the subtraction is stated.
        minLvl -= kContainerLevelShift;
        maxLvl -= kContainerLevelShift;
    }

    range.minLevel = minLvl;
    range.maxLevel = maxLvl;
    return range;
}

// R6: choose one of the 10 category values at random.
// -1 selects any category. A value below -1 names a fixed item.
CategoryPick pickItemCategory(Ctx& ctx, const ItemDropRow& row) {
    CategoryPick pick{};
    const i32 slot = static_cast<i32>(ctx.rng.below(kItemCategoryCount));
    const i32 value = row.categories[static_cast<u32>(slot)];

    if (value == kAnyCategoryValue) {
        pick.kind = CategoryPickKind::AnyCategory;
        pick.category = kAnyCategoryValue;
    } else if (value < kAnyCategoryValue) {
        // UNKNOWN: mapping from the negative encoding to an item ID is not stated.
        // Simplest choice: the ID is the negated value.
        pick.kind = CategoryPickKind::FixedItem;
        pick.fixedItemId = -value;
    } else {
        pick.kind = CategoryPickKind::Category;
        pick.category = value;
    }
    return pick;
}

// R7: scan item IDs. Start at the first ID matching the min level and the category.
// Stop before the first ID (scanning from that start) that fails the max level or the category.
// UNKNOWN: whether the stop scan includes the start ID is not stated. Included here.
Vec<i32> listCandidateItems(const Ctx& ctx, const ItemLevelRange& range, i32 category) {
    Vec<i32> out{};
    const i32 count = static_cast<i32>(ctx.data.items.size());

    i32 start = -1;
    for (i32 i = 0; i < count; ++i) {
        const ItemDef& def = itemDefAt(ctx, i);
        // UNRESOLVED: ItemDef has no member 'level' (stubbed by defLevel). See defCategory.
        if (defLevel(def) >= range.minLevel && categoryMatches(def, category)) {
            start = i;
            break;
        }
    }
    if (start < 0) {
        return out;
    }

    i32 end = count;
    for (i32 j = start; j < count; ++j) {
        const ItemDef& def = itemDefAt(ctx, j);
        // UNRESOLVED: ItemDef has no member 'level' (stubbed by defLevel). See defCategory.
        if (!(defLevel(def) <= range.maxLevel && categoryMatches(def, category))) {
            end = j;
            break;
        }
    }

    for (i32 k = start; k < end; ++k) {
        out.push_back(k);
    }
    return out;
}

// F3 (R8): spawn percentage = Chance / sum(Chance over candidates) * 100.
// UNKNOWN: behaviour when the Chance sum is zero is not stated. Percent is 0 there.
Vec<ItemSpawnEntry> buildSpawnTable(const Ctx& ctx, const Vec<i32>& candidateIds) {
    Vec<ItemSpawnEntry> table{};
    double sum = 0.0;
    for (i32 id : candidateIds) {
        // UNRESOLVED: ItemDef has no member 'chance' (stubbed by defChance).
        sum += defChance(itemDefAt(ctx, id));
    }
    for (i32 id : candidateIds) {
        const ItemDef& def = itemDefAt(ctx, id);
        ItemSpawnEntry entry{};
        entry.itemId = id;
        entry.chance = defChance(def);  // UNRESOLVED: 'chance' stubbed by defChance.
        entry.percent = (sum > 0.0) ? (defChance(def) / sum * 100.0) : 0.0;  // UNRESOLVED: 'chance'.
        table.push_back(entry);
    }
    return table;
}

// F3 (R9): roll in [0,100). Walk the running sum. Spawn the first item with roll <= cumulative.
// UNKNOWN: the roll is drawn even when the table is empty. If the sum falls short of the roll,
// nothing spawns.
ItemSpawnPick rollItemSpawn(Ctx& ctx, const Vec<ItemSpawnEntry>& table) {
    ItemSpawnPick pick{};
    pick.roll = ctx.rng.nextDouble() * 100.0;

    double cum = 0.0;
    for (const ItemSpawnEntry& entry : table) {
        cum += entry.percent;
        if (pick.roll <= cum) {
            pick.spawned = true;
            pick.itemId = entry.itemId;
            return pick;
        }
    }
    return pick;
}

// F4 (R10): difficulty = 25 + ((ln(damageMod+1) + .3) * ln(levelScale+1) * dungeonLevel).
double identifyDifficulty(double damageMod, double levelScale, i32 dungeonLevel) {
    return kIdentifyBaseDifficulty +
           ((std::log(damageMod + 1.0) + kIdentifyLnBias) * std::log(levelScale + 1.0) *
            static_cast<double>(dungeonLevel));
}

// F4: skill = (INT_nat + WIS_nat + INT_mod + WIS_mod) / 2 * ln(((maxGuildLvl + 5) / 10) + 1).
// Source: community-derived.
double identifySkill(i32 naturalInt, i32 naturalWis, i32 modInt, i32 modWis, i32 maxGuildLevel) {
    const double sum =
        static_cast<double>(naturalInt + naturalWis + modInt + modWis) / 2.0;
    const double guildTerm =
        std::log(((static_cast<double>(maxGuildLevel) + kIdentifyGuildOffset) / kIdentifyGuildDivisor) + 1.0);
    return sum * guildTerm;
}

// F4 applied to a character. Uses the highest guild level across all guilds.
double characterIdentifySkill(const Character& c) {
    const u32 intIx = static_cast<u32>(StatIndex::Int);
    const u32 wisIx = static_cast<u32>(StatIndex::Wis);

    i32 maxLvl = 0;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const i32 lvl = c.guilds[static_cast<u32>(g)].level;
        if (lvl > maxLvl) {
            maxLvl = lvl;
        }
    }

    return identifySkill(c.abilities.stats[intIx], c.abilities.stats[wisIx],
                         c.abilities.statMods[intIx], c.abilities.statMods[wisIx], maxLvl);
}

// R11: result rules, in order.
//   skill >= difficulty and dungeon level 1-5: MOSTLY
//   difficulty - skill < 20:                   PARTIAL
//   difficulty - skill >= 20:                  BARELY
// UNKNOWN: skill >= difficulty at dungeon level 6 or deeper is not stated.
// Simplest choice: it falls through to the gap rule, which gives PARTIAL here.
// UNKNOWN: whether "PARTIALLY" (rules) and "PARTIAL" (level list) are the same level.
// Both are treated as IdLevel::Partial.
IdLevel identifyLevelFromSkill(i32 dungeonLevel, double skill, double difficulty) {
    const bool inMostlyBand =
        dungeonLevel >= kMostlyMinDungeonLevel && dungeonLevel <= kIdentifyMostlyMaxDungeonLevel;
    if (skill >= difficulty && inMostlyBand) {
        return IdLevel::Mostly;
    }
    if (difficulty - skill < kIdentifyPartialGap) {
        return IdLevel::Partial;
    }
    return IdLevel::Barely;
}

// R10: on find, compare the finder's skill with the item's difficulty.
// UNKNOWN: whether the find result is ever combined with an item's existing idLevel is not stated.
// The computed level is returned as is.
IdLevel identifyItemOnFind(const Ctx& ctx, const Character& finder, const Item& item, i32 dungeonLevel) {
    const ItemDef& def = itemDefAt(ctx, item.id);
    const double difficulty = identifyDifficulty(def.damageMod, def.levelScale, dungeonLevel);  // F4
    const double skill = characterIdentifySkill(finder);
    return identifyLevelFromSkill(dungeonLevel, skill, difficulty);  // R10, R11
}

// R12: each Identify click at the General Store raises identification by one level.
// Only the General Store reaches ALL.
// UNKNOWN: the effect of a click away from the General Store is not stated.
// Simplest choice: it raises by one level, capped at MOSTLY.
IdLevel identifyClickStep(IdLevel current, bool atGeneralStore) {
    const i32 cur = static_cast<i32>(current);
    const i32 allLevel = static_cast<i32>(IdLevel::All);
    const i32 mostly = static_cast<i32>(IdLevel::Mostly);

    if (atGeneralStore) {
        i32 next = cur + 1;
        if (next > allLevel) {
            next = allLevel;
        }
        return static_cast<IdLevel>(next);
    }

    if (cur >= mostly) {
        return current;
    }
    return static_cast<IdLevel>(cur + 1);
}

// R13: exchange-form quirk. If the recipient can identify better than the item's current level,
// the difficulty is recalculated using the item whose ID equals this item's levelScale.
// UNKNOWN: whether the recalculated level can fall below item.idLevel is not stated.
// Simplest choice: the result is never below item.idLevel.
// UNKNOWN: if levelScale is not a valid item index, the item's own definition is used.
IdLevel exchangeIdentifyLevel(const Ctx& ctx, const Item& item, const Character& recipient, i32 dungeonLevel) {
    const ItemDef& def = itemDefAt(ctx, item.id);
    const double skill = characterIdentifySkill(recipient);

    const IdLevel normal = identifyLevelFromSkill(
        dungeonLevel, skill, identifyDifficulty(def.damageMod, def.levelScale, dungeonLevel));
    if (static_cast<i32>(normal) <= static_cast<i32>(item.idLevel)) {
        // Recipient cannot identify better. Nothing changes.
        return item.idLevel;
    }

    // R13 quirk: the item whose ID equals this item's levelScale supplies the difficulty.
    const i32 quirkId = static_cast<i32>(def.levelScale);
    const i32 count = static_cast<i32>(ctx.data.items.size());
    const ItemDef& quirkDef = (quirkId >= 0 && quirkId < count) ? itemDefAt(ctx, quirkId) : def;

    const IdLevel recalculated = identifyLevelFromSkill(
        dungeonLevel, skill, identifyDifficulty(quirkDef.damageMod, quirkDef.levelScale, dungeonLevel));

    if (static_cast<i32>(recalculated) < static_cast<i32>(item.idLevel)) {
        return item.idLevel;
    }
    return recalculated;
}

// F5 (R14): useLevel = max(1, round(itemGuildMod * guildLevelMod * 3.97)).
// UNKNOWN: the rounding mode of round() is not stated. Round-half-even is used, matching F2.
i32 guildUseLevel(double itemGuildMod, double guildLevelMod) {
    i64 useLevel = roundHalfEven(itemGuildMod * guildLevelMod * kGuildUseScale);
    if (useLevel < kMinUseLevel) {
        useLevel = kMinUseLevel;
    }
    return static_cast<i32>(useLevel);
}

// R15: charge items cast the stated spell at a fixed level. Each use spends one charge.
// UNKNOWN: whether a use casts the spell once or timesCast times is not stated. Reported as timesCast.
// UNKNOWN: the spell effect itself is applied by the casting module. This function spends the charge
// and reports the cast. The caller applies the effect to `user`.
ChargeCastResult castChargeItem(Ctx& ctx, Character& user, Item& item, const ItemDef& def) {
    (void)user;
    ChargeCastResult result{};
    result.spellId = def.spellId;
    // FIXED: ItemDef has 'spellLvl', not 'spellLevel' (compiler suggestion).
    result.spellLevel = def.spellLvl;
    // UNRESOLVED: ItemDef has no member 'timesCast' (stubbed by defTimesCast).
    result.timesCast = defTimesCast(def);

    if (item.charges <= 0) {
        result.cast = false;
        result.chargesLeft = item.charges;
        return result;
    }

    item.charges -= 1;
    result.cast = true;
    result.chargesLeft = item.charges;

    ctx.say("item.charge_cast", "The item casts its spell.",
            {static_cast<i64>(result.spellId), static_cast<i64>(result.spellLevel)}, {});
    return result;
}

// R16: items with spell-point bonuses add SP on use (Scroll of Spells 50, Tome of Spells 200).
// The caller passes the bonus. Returns the new current spell points.
// UNKNOWN: whether current SP is capped at maxSpellPoints is not stated. No cap is applied.
i32 addItemSpellPoints(Ctx& ctx, Character& user, i32 points) {
    user.vitals.curSpellPoints += points;
    ctx.say("item.spell_points", "Spell points restored by an item.",
            {static_cast<i64>(points), static_cast<i64>(user.vitals.curSpellPoints)}, {});
    return user.vitals.curSpellPoints;
}

// R17: equipping applies the item's stat modifiers. Unequipping removes them.
// UNKNOWN: whether removal reverses the exact amounts applied is not stated. Symmetric removal is used.
void applyItemStatMods(Ctx& ctx, Character& c, const ItemDef& def, bool equipping) {
    const i32 sign = equipping ? 1 : -1;
    for (i32 s = 0; s < kStatCount; ++s) {
        const u32 ix = static_cast<u32>(s);
        // FIXED: ItemDef has 'statsMod', not 'statMods' (compiler suggestion).
        c.abilities.statMods[ix] += sign * def.statsMod[ix];
    }
    ctx.say("item.stat_mods",
            equipping ? "Item stat modifiers applied." : "Item stat modifiers removed.",
            {static_cast<i64>(sign)}, {});
}

// R18 / F6: Thieving Ability, normalized to a 0-100 score.
// X uses the character's inputs. Y is the baseline (GLvl 1, Dex 2, Wis 2, ThievingMod 5).
// Z is the maximum (GLvl 999, Dex 30, Wis 40, ThievingMod 9).
// CONFLICT: the character's term uses Dex/2 + Wis/2. The Y and Z reference terms use Dex/4 and Wis/2
// as written. Translated literally.
// CONFLICT: Z as written is 30/4 + 40/2. R18 says 40 Dex and 30 Wis. Engine assignment UNKNOWN.
// UNKNOWN: out-of-range inputs can give values outside 0-100. No clamp is stated, so none is applied.
// The result is CInt-style, so round-half-even.
i32 thievingAbility(i32 thiefGuildLevel, i32 dex, i32 wis, double thievingMod) {
    const double x = thievingScore(static_cast<double>(dex) / 2.0, static_cast<double>(wis) / 2.0,
                                   static_cast<double>(thiefGuildLevel), thievingMod);
    const double y = thievingScore(2.0 / 4.0, 2.0 / 2.0, 1.0, 5.0);
    const double z = thievingScore(30.0 / 4.0, 40.0 / 2.0, 999.0, 9.0);

    const i64 score = roundHalfEven((x - y) / (z - y) * 100.0);
    return static_cast<i32>(score);
}

}  // namespace mordor
