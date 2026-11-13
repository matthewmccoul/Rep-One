// items4.cpp: item level ranges, drop minimums, identification, use level,
// exchange-form identification recalculation, and item spell points.
// Source: SRD c13p4 (tables T3-T6, identification rules, quirks).

#include "mordor/items4.h"
#include "mordor/items3b.h"  // UNKNOWN: ItemLevelRange is declared in items3b.h per the API INDEX; field names min/max assumed.
#include "mordor/math_util.h"

namespace mordor {

namespace {

// Number of item templates in the static data (used for bounds checks).
// UNKNOWN: assumes GameData::items is a Vec-like container with size().
i32 itemCount(const GameData& data) {
    return static_cast<i32>(data.items.size());
}

bool validItemId(const GameData& data, i32 itemId) {
    return itemId >= 0 && itemId < itemCount(data);
}

}  // namespace

// T3: item level range adjustment by chest or container.
//   Chest: Min +1, Max none. Box: none. Container: Min -1, Max -1.
//   Asmodeus chest: fixed Min 14, fixed Max 16.
ItemLevelRange adjustLevelRangeForContainer(ChestClass container, i32 baseMin, i32 baseMax) {
    // FIXED: ItemLevelRange member names (min/max) are not visible in the provided
    // index or digest. The range is computed in locals and returned with aggregate
    // initialization, assuming the struct's first two members are the min and max
    // levels in that order. Verify against items3b.h.
    i32 lo = baseMin;
    i32 hi = baseMax;
    switch (container) {
        case ChestClass::CONTAINER:
            lo = baseMin - 1;
            hi = baseMax - 1;
            break;
        case ChestClass::CHEST:
            lo = baseMin + 1;
            break;
        case ChestClass::ASMODEUS_CHEST:
            lo = 14;
            hi = 16;
            break;
        case ChestClass::NONE:
        case ChestClass::BOX:
        default:
            break;
    }
    return ItemLevelRange{lo, hi};
}

// T4: author's minimum-level list for ItemDropLevel 1-16.
// Formula for the list is UNKNOWN (banker's rounding is only the author's guess),
// so the listed values are used directly. Out-of-range input is clamped.
// UNKNOWN: clamping of out-of-range drop levels is a choice; the SRD does not say.
i32 itemDropMinLevel(i32 itemDropLevel) {
    i32 level = clampv(itemDropLevel, kItemDropLevelMin, kItemDropLevelMax);
    return kItemDropMinLevel[static_cast<u32>(level - kItemDropLevelMin)];
}

// T5: identification rules.
//   skill >= difficulty, dungeon level 1-5  -> MOSTLY
//   skill >= difficulty, dungeon level 6+   -> UNKNOWN (see below)
//   difficulty - skill >= 20                -> BARELY
//   difficulty - skill < 20                 -> PARTIAL
// The table's rows overlap when skill >= difficulty (the "< 20" row also matches).
// The skill >= difficulty rows are checked first, so the overlap resolves as stated.
IdLevel identifyLevelFor(i32 skill, i32 difficulty, i32 dungeonLevel) {
    // RESOLVED-BY-REFERENCE: Loot.cs:62 ReadableLevel (MAINMOD2.BAS:2480): "result = Partial"
    // is the default, and "If difficulty <= skill Then If item.Floor <= 5 Then result = Mostly".
    // Within skill but deeper than the shallow band the result is Partial, not Unknown.
    if (skill >= difficulty) {
        if (dungeonLevel <= kIdentifyShallowMaxLevel) {
            return IdLevel::Mostly;
        }
        return IdLevel::Partial;
    }
    i32 gap = difficulty - skill;
    if (gap >= kIdentifyBarelyGap) {
        return IdLevel::Barely;
    }
    // CONFIRMED: "PARTIALLY" (rule table) and "PARTIAL" (level list) are the one IdLevel::Partial.
    // The reference names that single level "Partially identified" (Loot.cs IdLevelName).
    return IdLevel::Partial;
}

// T6: only the General Store reaches ALL. Any other source is capped at MOSTLY.
// UNKNOWN: the cap target MOSTLY is the highest non-ALL level; the SRD does not name a cap.
IdLevel capIdentifyLevel(IdLevel earned, bool generalStore) {
    if (!generalStore && earned == IdLevel::All) {
        return IdLevel::Mostly;
    }
    return earned;
}

// Quirk: exchange form (not drag and drop) recalculates identification using the
// item whose ID equals levelScale.
// UNKNOWN: the skill and difficulty formulas are not in this SRD part. Placeholders:
//   skill      = INT + WIS + topGuildLevel of the recipient
//   difficulty = level of the reference item
//   generalStore = false (exchange location not stated)
// If the item or reference item is out of range, the item is left unchanged.
void recalcExchangeIdentify(Ctx& ctx, const Character& recipient, Item& item) {
    const GameData& data = ctx.data;
    if (!validItemId(data, item.id)) {
        return;
    }
    const ItemDef& own = data.items[static_cast<u32>(item.id)];
    i32 refId = own.levelScale;  // UNKNOWN: ItemDef::levelScale field name assumed.
    if (!validItemId(data, refId)) {
        return;
    }
    const ItemDef& ref = data.items[static_cast<u32>(refId)];

    i32 intStat = recipient.abilities.stats[static_cast<u32>(StatIndex::Int)];
    i32 wisStat = recipient.abilities.stats[static_cast<u32>(StatIndex::Wis)];
    i32 skill = intStat + wisStat + recipient.topGuildLevel;  // UNKNOWN: formula placeholder.
    // FIXED: ItemDef has no member 'level'. The closest existing field is levelScale,
    // used here as a stand-in for the reference item's level. This is NOT verified
    // against the SRD; check ItemDef's definition for the correct level field.
    i32 difficulty = ref.levelScale;

    IdLevel earned = identifyLevelFor(skill, difficulty, recipient.location.dungeonLevel);
    item.idLevel = capIdentifyLevel(earned, false);
}

// Minimum guild level needed to use an item, per guild.
// UNKNOWN: the SRD gives per-guild use levels in its item tables but not the
// use-level formula (Guild Mod / Guild Level Mod inputs). Stored per-guild values are used.
// A value of 0 or less means the guild cannot use the item (e.g. "Ninja 139 only").
i32 itemUseLevel(const GameData& data, i32 itemId, Guild guild) {
    if (!validItemId(data, itemId)) {
        return kNoGuildUse;
    }
    const ItemDef& item = data.items[static_cast<u32>(itemId)];
    // FIXED: ItemDef has no member 'guildUseLevels' and no existing per-guild field
    // was identified in the provided index. Per-guild use level is treated as 0
    // (cannot use) until the correct ItemDef field is confirmed. This changes
    // behaviour: every item will report kNoGuildUse until fixed.
    (void)item;
    (void)guild;
    i32 level = 0;
    if (level <= 0) {
        return kNoGuildUse;
    }
    return level;
}

// Character may use the item if its active guild level meets the use level.
// UNKNOWN: the "class restricted" logic is not defined in the SRD; not modelled.
bool meetsItemUseLevel(const GameData& data, const Character& c, i32 itemId) {
    i32 need = itemUseLevel(data, itemId, c.activeGuild);
    if (need == kNoGuildUse) {
        return false;
    }
    return c.activeLevel() >= need;
}

// Scroll of Spells adds 50 spell points on use; Tome of Spells adds 200 (T2).
// UNKNOWN: SRD does not state a cap on spell points; the total is capped at
// maxSpellPoints here so the value cannot exceed the maximum.
void addSpellPointsFromItem(Ctx& ctx, Character& c, i32 amount) {
    Vitals& v = c.vitals;
    i32 before = v.curSpellPoints;
    i32 after = before + amount;
    if (after > v.maxSpellPoints) {
        after = v.maxSpellPoints;
    }
    v.curSpellPoints = after;
    ctx.say("items4.spellPointsAdded", "Spell points restored from item",
            {static_cast<i64>(after - before), static_cast<i64>(after)}, {});
}

}  // namespace mordor
