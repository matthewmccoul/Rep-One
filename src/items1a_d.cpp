// items1a_d.cpp: SRD c13p1 half d (R68-R92).
// Slime and monster destroy, theft, quicksand slot, Seer carrier and take,
// identify/recognise/rename chances, gear effect bits, and item prices.
#include <cstddef>
#include <string>

#include "mordor/items1a.h"
#include "mordor/store_prices.h"
#include "mordor/items1b.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

// UNKNOWN: destroy chance formula is not in the SRD (R71 says "checked against the destroy chance", R90 says Formulas). Placeholder 50 percent.
constexpr i32 kDestroyChancePlaceholder = 50;

// Name of the item for messages and display. ItemDef field names are not given in the SRD part; `name` is assumed.
// UNKNOWN: ItemDef::name, GameData::items indexed by item.id.
Str itemBaseName(const Ctx& ctx, const Item& item) {
    const auto& defs = ctx.data.items;
    if (item.id < 0 || static_cast<std::size_t>(item.id) >= defs.size()) {
        return Str("Unidentified Item");
    }
    return defs[static_cast<std::size_t>(item.id)].name;
}

// Item type label used for the Partial ID level (R78).
// UNKNOWN: NamedEntry::name for GameData::itemTypes indexed by item type.
Str itemTypeLabel(const Ctx& ctx, const Item& item) {
    const auto& types = ctx.data.itemTypes;
    const i32 idx = static_cast<i32>(item.type);
    if (idx < 0 || static_cast<std::size_t>(idx) >= types.size()) {
        return Str("Unidentified Item");
    }
    return types[static_cast<std::size_t>(idx)].name;
}

// R70 destructible: SpecialType < 500 or SpecialType > 500 + GuildCount - 1 (511).
// SpecialType is taken to be Item::ailments (state_item.h stores the guild marker there).
bool isDestructibleSpecial(i32 specialType) {
    return specialType < kGuildMarkerBase || specialType > kDestructibleSpecialMax;
}

// R70: a destructible item is destroyed and "<prefix><Name> Destroyed!" is written.
// UNKNOWN: the name shown uses the base item name, not the player's ID level.
void destroyItemSlot(Ctx& ctx, Inventory& inv, i32 slot, const char* code, const char* prefix) {
    Str name = itemBaseName(ctx, inv.pack[static_cast<std::size_t>(slot)]);
    inv.pack[static_cast<std::size_t>(slot)] = Item{};
    ctx.say(Str(code), Str(prefix) + name + Str(" Destroyed!"), {}, {name});
}

}  // namespace

// R69 + R70: the slot is 1..39 and an empty slot has no effect. The R68 roll is made by the caller (slimeRollReaches).
bool slimeDestroy(Ctx& ctx, Inventory& inv, i32 slot) {
    if (slot < kFirstItemSlot || slot > kLastItemSlot) {
        return false;
    }
    const Item& item = inv.pack[static_cast<std::size_t>(slot)];
    if (slotIsEmpty(item)) {
        return false;  // R69 empty slot: no effect
    }
    if (!isDestructibleSpecial(item.ailments)) {  // R70
        return false;
    }
    destroyItemSlot(ctx, inv, slot, "items.slime_destroyed", "SLIME: ");
    return true;
}

// R71 monster destroy. Slot is 1..39. An item with floor - 2 <= LevelFound and destructible is checked against the destroy chance.
// UNKNOWN: destroy chance formula is not given (R90). Placeholder 50 percent. Message prefix is also UNKNOWN.
bool monsterDestroyCheck(Ctx& ctx, Inventory& inv, i32 slot, i32 monsterFloor, i32 levelFound) {
    if (slot < kFirstItemSlot || slot > kLastItemSlot) {
        return false;
    }
    const Item& item = inv.pack[static_cast<std::size_t>(slot)];
    if (slotIsEmpty(item)) {
        return false;
    }
    if (monsterFloor - kDestroyFloorOffset > levelFound) {  // R71 floor - 2 <= LevelFound
        return false;
    }
    if (!isDestructibleSpecial(item.ailments)) {  // R70 / R71
        return false;
    }
    if (!ctx.rng.percent(kDestroyChancePlaceholder)) {  // UNKNOWN: placeholder chance
        return false;
    }
    destroyItemSlot(ctx, inv, slot, "items.monster_destroyed", "MONSTER: ");
    return true;
}

// R72: theft slot = Int(Rnd x 39) + 2, giving 2..40.
// Quirk: slot 40 is off the array, so one roll in 39 misses and slot 1 is never stolen. The value is returned as is.
i32 theftSlotRoll(Ctx& ctx) {
    // R72 / Formulas: TheftSlot = Int(Rnd * 39) + 2, range 2..40 (original quirk).
    return static_cast<i32>(ctx.rng.below(39)) + 2;
}

// R72: theft is decided by score - monLvl x 2.
// UNKNOWN: the success test is not given (R90, Formulas). Uses the margin, clamped to 0..100, as a percent chance.
bool theftDecided(Ctx& ctx, i32 score, i32 monsterLevel) {
    const i32 margin = score - monsterLevel * 2;
    const i32 chance = clampv(margin, 0, 100);
    return ctx.rng.percent(chance);
}

// R73 quicksand rolls a slot (1..39) and acts if the slot is occupied.
// UNKNOWN: the quicksand outcome is not given. This only reports the occupied slot.
void quicksandSlotRoll(Ctx& ctx, Inventory& inv) {
    const i32 slot = pickItemSlot(ctx);  // one slot in 1..39
    if (slotIsEmpty(inv.pack[static_cast<std::size_t>(slot)])) {
        return;
    }
    ctx.say("items.quicksand_slot", "Quicksand reaches an occupied slot.",
            {static_cast<i64>(slot)}, {});
}

// R74: for each carrier, band = Rnd(3). A hit needs a type match and a drop level from floor to floor+band. Named items are certain.
// UNKNOWN: the order of the band roll against the named check is not stated. The roll is made first for every carrier.
// UNKNOWN: no target type is passed, so the type match is taken as the item type code being valid (>= 0).
bool seerCarrierHit(Ctx& ctx, i32 itemTypeCode, i32 dropLevel, i32 floor, bool named) {
    const i32 band = static_cast<i32>(ctx.rng.below(kSeerBandSides));
    if (named) {
        return true;  // R74 named items are certain
    }
    const bool typeMatch = itemTypeCode >= 0;  // UNKNOWN
    const bool levelInBand = dropLevel >= floor && dropLevel <= floor + band;
    return typeMatch && levelInBand;
}

// R75: the Seer names take carriers, where take is 1..3.
// UNKNOWN: the take formula is in Formulas (not given). Uniform 1..kSeerTakeMax is assumed.
i32 seerTakeCount(Ctx& ctx) {
    return 1 + static_cast<i32>(ctx.rng.below(kSeerTakeMax));
}

// R78: item name at each ID level. The port counts upward: Unknown=0, Barely=1, Partial=2, Mostly=3, All=4.
// All: full name, alignment tag (g)/(n)/(e) when aligned, "[n]" when charged, "(cursed)" when cursed.
// Mostly: name. Partial: type. Barely and Unknown: "Unidentified Item".
// UNKNOWN: Barely should give a vaguer description. The description table is not specified, so the fallback is used.
// UNKNOWN: "charged" is taken as maxCharges > 0.
Str itemDisplayName(const Ctx& ctx, const Item& item, IdLevel level) {
    if (level == IdLevel::Unknown || level == IdLevel::Barely) {
        return Str("Unidentified Item");
    }
    if (level == IdLevel::Partial) {
        return itemTypeLabel(ctx, item);
    }
    Str out = itemBaseName(ctx, item);
    if (level == IdLevel::Mostly) {
        return out;
    }
    // level == IdLevel::All
    if (item.aligned) {
        switch (item.alignment) {
            case ItemAlign::Good:    out += Str(" (g)"); break;
            case ItemAlign::Neutral: out += Str(" (n)"); break;
            case ItemAlign::Evil:    out += Str(" (e)"); break;
            default: break;
        }
    }
    if (item.maxCharges > 0) {
        out += Str(" [") + Str(std::to_string(item.charges)) + Str("]");
    }
    if (item.cursed != CurseNone) {
        out += Str(" (cursed)");
    }
    return out;
}

// R77 reading difficulty is compared with identify skill.
// UNKNOWN: the identify chance formula is not in this SRD part. Placeholder: skill, minus 10 when dark.
// Quirk (R77 / c30): modifiers are taken as they are, with no floor at 0.
i32 identifyChance(f64 skill, bool dark) {
    // R18/T-8, SRD ch13 Chances: identify = clamp(int((dark ? skill*0.75 : skill) + 20), 10, 90)
    // The reference truncates with (int); the clamp makes truncation and VbInt floor agree here.
    const f64 base = (dark ? skill * 0.75 : skill) + 20.0;
    i64 chance = static_cast<i64>(base);
    if (chance < 10) chance = 10;
    if (chance > 90) chance = 90;
    return static_cast<i32>(chance);
}

// UNKNOWN: recognise chance formula is not in this SRD part. Placeholder: skill, minus 5 when dark. No floor, as for identifyChance.
// SRD Formulas, Chances: recognise = clamp(int((dark ? skill : skill*1.25) + 20), 5, 80)
i32 recogniseChance(f64 skill, bool dark) {
    const f64 base = (dark ? skill : skill * 1.25) + 20.0;
    i64 chance = static_cast<i64>(vbInt(base));
    if (chance < 5) chance = 5;
    if (chance > 80) chance = 80;
    return static_cast<i32>(chance);
}

// UNKNOWN: rename chance formula is not in this SRD part. Placeholder: skill, minus 10 in fog. No floor, as for identifyChance.
// SRD Formulas, Rename: p = (inFog ? skill*0.75 : skill) + 20; clamp 10..90
i32 renameChance(f64 skill, bool inFog) {
    const f64 base = (inFog ? skill * 0.75 : skill) + 20.0;
    i64 chance = static_cast<i64>(vbInt(base));
    if (chance < 10) chance = 10;
    if (chance > 90) chance = 90;
    return static_cast<i32>(chance);
}

// R88 GearEffects: OR of effect bits over equipped items that count under fit (Table T-5).
// Critical Hit and Backstab have no effect. The computation lives in gearEffects (items1b.h), so it is delegated here to avoid a second copy of Table T-5.
u32 gearEffectBits(const Ctx& ctx, const Character& c, const Inventory& inv) {
    return gearEffects(ctx, c, inv);
}

// R92 prices are Longs rounded half to even, not floored. The formulas are not given.
// UNKNOWN: no price formula in the SRD part. Placeholder 0 gold, passed through roundHalfEven as the rule requires.
namespace {
Currency placeholderPrice() {
    return roundHalfEven(0.0);
}
}  // namespace

Currency modItemCost(Ctx& ctx, const Item& item) {
    // Store-less default: one copy on the shelf. The game layer uses storeBuyPrice with the real shelf count.
    return storeBuyPrice(ctx.data, item, 1);
}

Currency identifyFee(Ctx& ctx, const Item& item) {
    return storeIdentifyFee(ctx.data, item, 1);   // store-less default; the game layer passes the shelf count
}

Currency sellPrice(Ctx& ctx, const Item& item) {
    return storeSellPrice(ctx.data, item, 1, true);   // store-less default; the game layer passes the shelf counts
}

// FIXED: list-price overload used by uncurseFee below. Defined in items2b_2.cpp; it is not in any header in the API index.
Currency uncurseFee(Currency listPrice);

Currency uncurseFee(Ctx& ctx, const Item& item) {
    // The reference has no uncurse fee. The SRD's R64 rule (uncurseFee(listPrice), items2b_2.cpp) applies to the list price.
    for (const ItemDef& d : ctx.data.items) {
        if (d.id == item.id) return uncurseFee(static_cast<Currency>(d.price) * 10000);
    }
    return 0;
}

}  // namespace mordor
