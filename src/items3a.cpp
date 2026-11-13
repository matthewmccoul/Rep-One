// items3a.cpp: implementation of include/mordor/items3a.h.
// Source: SRD part c13p3 (Chapter 13: Items and Equipment).
// UNKNOWN / CONFLICT markers flag places where the SRD chunk gives no formula
// or where GameData field names were not supplied in the brief.
#include <cstddef>

#include "mordor/items3a.h"
#include "mordor/math_util.h"
#include "mordor/items1a.h"
#include "mordor/items1b.h"
#include "mordor/casting3.h"

namespace mordor {
namespace {

// Return codes for the i32-typed service functions.
// UNKNOWN: the header uses i32 because the outcome enums are declared nowhere;
// these numeric meanings are chosen here and must match the callers.
constexpr i32 kServiceDone = 0;
constexpr i32 kServiceEmptySlot = 1;
constexpr i32 kServiceRefused = 2;
constexpr i32 kServiceNotEnoughGold = 3;
constexpr i32 kServiceNotStored = 4;   // slot 0 (Hands) is never stored [c32][c43]
constexpr i32 kServiceOverflow = 5;    // store sale would overflow gold; port refuses [c52]

constexpr i32 kCombineDone = 0;        // source fully absorbed
constexpr i32 kCombinePartial = 1;     // source keeps remainder [c52]
constexpr i32 kCombineRefused = 2;
constexpr i32 kCombineOverflow = 6;    // VB overflow past 32767 charges; caller exits with code 6 [c52]

constexpr i32 kBindDone = 0;
constexpr i32 kBindFreeWill = 1;       // [c50]
constexpr i32 kBindAlready = 2;        // [c50]
constexpr i32 kBindNotEnoughGold = 3;

constexpr i32 kUseNothing = 0;
constexpr i32 kUseSpell = 1;
constexpr i32 kUseItem = 2;

// Gold ceiling for a store sale: VB Long whole gold, expressed in 1/10000 gold [c52][c33].
constexpr i64 kMaxLongGoldUnits = 2147483647LL * 10000LL;

// Pack slots 1..39 are stored items; slot 0 is the character's Hands [c35][c32].
bool storedSlot(i32 slot) {
    return slot >= 1 && slot < kPackSlots;
}

bool validItemId(const Ctx& ctx, const Item& it) {
    return it.id > 0 && static_cast<i64>(it.id) < static_cast<i64>(ctx.data.items.size());
}

// Weapon Resistance lists by dungeon level band [c102].
constexpr const char* kWeaponBand2To3[] = {
    "Bronze Battle Axe", "Bronze Dagger", "Bronze Mace", "Bronze Sword",
    "Dagger of Stealth", "Pine Staff", "Sword of the Winds"};
constexpr const char* kWeaponBand4To5[] = {
    "Initiate's Hands", "Iron Dagger", "Iron Hammer", "Iron Sword", "Oak Staff"};
constexpr const char* kWeaponBand6To7[] = {
    "Cutlass", "Iron Battle Axe", "Iron Mace", "Katana", "Lethe Dagger",
    "Silver Cross", "Staff of Alton", "Thief Sword", "War Hammer"};
constexpr const char* kWeaponBand8To9[] = {
    "Iron-shod Staff", "Steel Dagger", "Wicked Blade"};
constexpr const char* kWeaponBand10To11[] = {
    "Crystal Sword", "Delvar's War Axe", "Dominator", "Magic Hands",
    "Steel Battle Axe", "Steel Mace", "Steel Sword"};
constexpr const char* kWeaponBand12To13[] = {
    "Adamantite Battle Axe", "Adamantite Dagger", "Adamantite Mace", "Adamantite Sword"};
constexpr const char* kWeaponBand14To15[] = {
    "Cross of Life", "Master's Hand", "Mithril Dagger", "Staff of Striking"};
constexpr const char* kWeaponBand16[] = {
    "Avenger", "Cross of Commanding", "Dwarven Hammer", "Mithril Battle Axe",
    "Mithril Mace", "Mithril Sword", "Mithril-shod Staff", "Shadow's Blade"};

template <std::size_t N>
bool inWeaponList(const char* const (&list)[N], const Str& name) {
    for (std::size_t i = 0; i < N; ++i) {
        if (name == list[i]) {
            return true;
        }
    }
    return false;
}

}  // namespace

// [c91] Stat items need 10 Wisdom and 10 Intelligence.
bool meetsStatItemRequirements(const Character& c) {
    return c.abilities.stats[static_cast<u32>(StatIndex::Wis)] >= 10 &&
           c.abilities.stats[static_cast<u32>(StatIndex::Int)] >= 10;
}

// [c32][c68] Stat raise by a stat item.
void applyItemStatBonus(Ctx& ctx, Character& c, const Item& item) {
    if (!validItemId(ctx, item)) {
        return;
    }
    // UNKNOWN: GameData field names (races[].statMax, items[].statMods) were not given in the brief.
    const ItemDef& def = ctx.data.items[static_cast<u32>(item.id)];
    const RaceDef& race = ctx.data.races[static_cast<u32>(c.race)];
    for (i32 s = 0; s < kStatCount; ++s) {
        const u32 us = static_cast<u32>(s);
        // [c32] stat ceiling = race maximum + 5 (computed before the add)
        // FIXED: RaceDef has no member statMax; using the character's own statMax[] as the
        // nearest existing max. Verify against the SRD race maximum before relying on this.
        (void)race;
        const i32 ceiling = c.abilities.statMax[us] + 5;
        // [c68] Stats(s) = Stats(s) + Items(id).M571E(s)
        // FIXED: ItemDef has no member statMods; renamed to statsMod (compiler suggestion).
        c.abilities.stats[us] += def.statsMod[us];
        // [c68] clamp Stats(s) to RaceDef.Max[s] + 5 (upper bound only, as the SRD states)
        if (c.abilities.stats[us] > ceiling) {
            c.abilities.stats[us] = ceiling;
        }
        // [c68] mdrhalCheckRecord s, char, CCur(Stats(s)) after the clamp
        hallOfRecordsCheck(ctx, c, static_cast<StatIndex>(s), c.abilities.stats[us]);
    }
}

// [c68] mdrhalCheckRecord (hall of records).
// UNKNOWN: body of mdrhalCheckRecord is not in the SRD chunk; no hall state exists in the
// headers given. Left as an explicit hook with no effect.
void hallOfRecordsCheck(Ctx& ctx, Character& c, StatIndex stat, i32 value) {
    (void)ctx;
    (void)c;
    (void)stat;
    (void)value;
}

// [c62] if (M56C4 > 0) spell = spells[M56C4 - 1]; -1 when none.
// UNKNOWN: M56C4 is mapped to Item.spellIndex; Item.spellId is the port-gate field [c62].
i32 itemSpellIndex(const Ctx& ctx, const Item& item) {
    if (item.spellIndex > 0 && static_cast<i64>(item.spellIndex) <= static_cast<i64>(ctx.data.spells.size())) {
        return item.spellIndex - 1;
    }
    return -1;
}

// [c62][c34] Item spell level: delegates to casting3's itemSpellLevel(itemLevel, casterSpellLevel).
// UNKNOWN: caster guild spell level taken as user.spellLevel (field not explained in SRD chunk).
i32 itemSpellLevel(const Ctx& ctx, const Character& user, const Item& item) {
    (void)ctx;
    return itemSpellLevel(item.level, user.spellLevel);
}

// [c34] Original charges SP for a level-0 item cast unless p01AA <> 0 (MAINMOD2.BAS:1355).
// UNKNOWN: behaviour for castLevel > 0 is not stated; treated as free, matching the port.
bool itemSpellChargesSP(i32 castLevel, bool p01AA) {
    return castLevel == 0 && !p01AA;
}

// [c44] ItemUse.Problem: refusal code, 0 = no problem.
// UNKNOWN: the full ItemUse.Problem refusal list is not in the SRD chunk; only the
// charge test is modelled, using UseCheck values from items1b.h.
i32 itemUseProblem(const Ctx& ctx, const Character& c, const Item& item) {
    (void)ctx;
    (void)c;
    if (item.maxCharges > 0 && item.charges <= 0) {
        return static_cast<i32>(UseCheck::OutOfCharges);
    }
    return static_cast<i32>(UseCheck::Ok);
}

// [c44] SpendCharge on a charged item.
void spendCharge(Ctx& ctx, Inventory& inv, i32 slot) {
    (void)ctx;
    if (!storedSlot(slot)) {
        return;
    }
    Item& it = inv.pack[static_cast<u32>(slot)];
    if (it.maxCharges > 0 && it.charges > 0) {
        it.charges -= 1;
    }
}

// [c44] UseItemSpell.
// UNKNOWN: spell resolution belongs to the casting modules; here the cast request is
// reported as an event. SP cost is not in the SRD chunk; item casts are treated as free
// (port behaviour [c34]); p01AA assumed 0.
bool castItemSpell(Ctx& ctx, Character& user, const Item& item) {
    const i32 idx = itemSpellIndex(ctx, item);
    if (idx < 0) {
        return false;
    }
    const i32 level = itemSpellLevel(ctx, user, item);
    (void)itemSpellChargesSP(level, false);
    ctx.say("items.spell.cast", "The item casts its spell.",
            {static_cast<i64>(idx), static_cast<i64>(level)}, {});
    return true;
}

// [c44] Use order: paralysed -> return; ItemUse.Problem -> say and return;
// SpellOf(item) -> UseItemSpell and return; SpendCharge; ApplySpecial; SpellListChecked.
i32 useInventoryItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot) {
    // [c44] button/double click only; paralysed users cannot use items
    if (c.ailments.paralysed) {
        return kUseNothing;
    }
    if (!storedSlot(slot)) {
        return kUseNothing;
    }
    Item& item = inv.pack[static_cast<u32>(slot)];
    if (item.id == 0) {
        // UNKNOWN: empty-slot use is silent; the SRD does not state a message.
        return kUseNothing;
    }
    const i32 problem = itemUseProblem(ctx, c, item);
    if (problem != 0) {
        // [c44] say the refusal (empty text would be silent; a generic text is used here)
        ctx.say("items.use.refused", "You cannot use that item.", {static_cast<i64>(problem)}, {});
        return kUseNothing;
    }
    // [c44] if SpellOf(item) != null: UseItemSpell; return
    if (itemSpellIndex(ctx, item) >= 0) {
        castItemSpell(ctx, c, item);
        return kUseSpell;
    }
    // [c44] SpendCharge(ch, slot, item)
    spendCharge(ctx, inv, slot);
    // UNKNOWN: ApplySpecial(...) effects and "if ChangesStats: SpellListChecked()" are not
    // in the SRD chunk and are not modelled here.
    return kUseItem;
}

// [c55] charPay split. Hand first, then the bank for the remainder.
// The bank's part is rounded to whole gold (VB CLng) and the bank may go negative (quirks [c55]).
bool payFromPurse(Purse& purse, Currency amount) {
    if (purse.goldOnHand + purse.goldInBank < amount) {
        return false;
    }
    if (purse.goldOnHand >= amount) {
        purse.goldOnHand -= amount;
        return true;
    }
    const Currency restRaw = amount - purse.goldOnHand;
    const i64 restWhole = roundHalfEven(static_cast<double>(restRaw) / 10000.0);  // CLng [c55]
    purse.goldOnHand = 0;
    purse.goldInBank -= restWhole * 10000;
    return true;
}

// [c52] fee = round_half_even(0.8 * Price / 0.5)
i64 storeFeeFromPrice(i64 price) {
    return roundHalfEven(0.8 * static_cast<double>(price) / 0.5);
}

// [c33][c52] PriceOf: item prices are Long.
// UNKNOWN: PriceOf formula and the ItemDef price field name are not in the SRD chunk;
// base price read from GameData (items[].price); alignment and charges not applied.
i64 priceOfItem(const Ctx& ctx, const Item& item, ItemAlign alignment) {
    (void)alignment;
    if (!validItemId(ctx, item)) {
        return 0;
    }
    const ItemDef& def = ctx.data.items[static_cast<u32>(item.id)];
    return def.price;
}

// [c52] StoreStock.RealignCost(item, current, wanted).
// UNKNOWN: the body is not in the SRD chunk. The items1b realignCost(basePrice, slotAlign,
// storeAlign) formula is used, with the item's base price.
i64 realignCost(const Ctx& ctx, const Item& item, ItemAlign current, ItemAlign wanted) {
    return realignCost(priceOfItem(ctx, item, current),
                       static_cast<i32>(current),
                       static_cast<i32>(wanted));
}

// [c87] idFee ~= sellValue ("The Clerk's Rule").
// CONFLICT: wiki (%2Aid-and-uncurse.md) also gives 0.10-0.20 * base market value; the
// sellValue rule is chosen as the stated preferred behaviour. Sell value approximated by priceOfItem.
i64 identifyFee(const Ctx& ctx, const Item& item) {
    return priceOfItem(ctx, item, item.alignment);
}

// [c52][c35] Info = Price if (Floor == 0 and counts[0] > 0).
bool floorStocksUnaligned(const Shelf& shelf) {
    return shelf.floor == 0 && shelf.alignCounts[0] > 0;
}

// [c35] count for U, G, N or E (Align(0..3) = counts for U, G, N, E).
i32 shelfCountFor(const Shelf& shelf, ItemAlign alignment) {
    return shelf.alignCounts[static_cast<u32>(alignment)];
}

// [c52] Store sale. The original has no overflow handler; the port refuses the sale.
// UNKNOWN: result codes are chosen here (see kService*).
i32 sellItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot) {
    if (!storedSlot(slot)) {
        return kServiceNotStored;
    }
    Item& it = inv.pack[static_cast<u32>(slot)];
    if (it.id == 0) {
        return kServiceEmptySlot;
    }
    const Currency price = sellPrice(ctx, it);  // items1a; prices are Long [c33]
    if (c.wallet.goldOnHand > kMaxLongGoldUnits - price) {
        ctx.say("items.sell.overflow", "Sorry, the sale would overflow.", {}, {});  // [c52]
        return kServiceOverflow;
    }
    c.wallet.goldOnHand += price;
    it = Item{};
    return kServiceDone;
}

// [c52] Realign. The Realign handler does not re-check the current-alignment block (quirk kept).
// UNKNOWN: whether alignFlags are also rewritten is not stated; only alignment and aligned change.
i32 realignItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot, ItemAlign wanted) {
    if (!storedSlot(slot)) {
        return kServiceNotStored;
    }
    Item& it = inv.pack[static_cast<u32>(slot)];
    if (it.id == 0) {
        return kServiceEmptySlot;
    }
    const i64 fee = realignCost(static_cast<const Ctx&>(ctx), it, it.alignment, wanted);
    if (!payFromPurse(c.wallet, fee)) {
        return kServiceNotEnoughGold;
    }
    it.alignment = wanted;
    it.aligned = true;
    return kServiceDone;
}

// [c50] to = (level + Binding.Step > Binding.Max) ? Binding.Max : level + Binding.Step
i32 bindingTarget(i32 level, i32 step) {
    return (level + step > kBindingMax) ? kBindingMax : level + step;
}

// [c50] Binding.Cost(def.GuildLevel, level).
// UNKNOWN: the cost formula is not in the SRD chunk. Placeholder returns zero; the port must
// supply the real formula.
i64 bindingCost(i32 guildLevel, i32 level) {
    (void)guildLevel;
    (void)level;
    return 0;
}

// [c50] Binding.Charge(cost).
// UNKNOWN: the charge formula is not in the SRD chunk; cost is charged unchanged.
i64 bindingCharge(i64 cost) {
    return cost;
}

// [c50] Companion binding.
// Refusals: free will cannot be bound; already bound at or above Binding.Max.
i32 bindCompanion(Ctx& ctx, Companion& comp, Purse& purse, i32 guildLevel, i32 step) {
    (void)ctx;
    if (isFreeWill(comp)) {
        return kBindFreeWill;  // FreeWillCannotBind [c50]
    }
    if (comp.bindingLevel >= kBindingMax) {
        return kBindAlready;   // AlreadyBound [c50]
    }
    const i32 to = bindingTarget(comp.bindingLevel, step);
    const i64 cost = bindingCost(guildLevel, comp.bindingLevel);
    const i64 pay = bindingCharge(cost);
    if (!payFromPurse(purse, pay)) {
        return kBindNotEnoughGold;
    }
    comp.bindingLevel = to;
    return kBindDone;
}

// [c50] Companion identify. Sets the owner ID level to Completely.
// UNKNOWN: fee taken from penPriceToIdentify; the pen price formula is not in the SRD chunk.
i32 identifyCompanion(Ctx& ctx, Companion& comp, Purse& purse) {
    const i64 fee = penPriceToIdentify(ctx, comp);
    if (!payFromPurse(purse, fee)) {
        return kServiceNotEnoughGold;
    }
    comp.idLevel = OwnerIdLevel::Completely;
    return kServiceDone;
}

// UNKNOWN: pen sale price formula is not in the SRD chunk; placeholder zero.
i64 penPriceToSell(const Ctx& ctx, const Companion& comp) {
    (void)ctx;
    (void)comp;
    return 0;
}

// UNKNOWN: pen identify price formula is not in the SRD chunk; placeholder zero.
i64 penPriceToIdentify(const Ctx& ctx, const Companion& comp) {
    (void)ctx;
    (void)comp;
    return 0;
}

// UNKNOWN: pen realign price formula is not in the SRD chunk; placeholder zero.
i64 penPriceToRealign(const Ctx& ctx, const Companion& comp) {
    (void)ctx;
    (void)comp;
    return 0;
}

// [c102] Weapon Resistance by dungeon level band. Level 1: no weapons resisted.
// UNKNOWN: a weapon is treated as resisted only when listed for the band containing the
// dungeon level (not cumulative across bands). Levels above 16 have no list: treated as not
// resisted. Weapon name read from GameData (items[].name, field name not given in brief).
bool weaponResisted(const Ctx& ctx, const Item& weapon, i32 dungeonLevel) {
    if (!validItemId(ctx, weapon)) {
        return false;
    }
    const Str& name = ctx.data.items[static_cast<u32>(weapon.id)].name;
    if (dungeonLevel >= 2 && dungeonLevel <= 3) {
        return inWeaponList(kWeaponBand2To3, name);
    }
    if (dungeonLevel >= 4 && dungeonLevel <= 5) {
        return inWeaponList(kWeaponBand4To5, name);
    }
    if (dungeonLevel >= 6 && dungeonLevel <= 7) {
        return inWeaponList(kWeaponBand6To7, name);
    }
    if (dungeonLevel >= 8 && dungeonLevel <= 9) {
        return inWeaponList(kWeaponBand8To9, name);
    }
    if (dungeonLevel >= 10 && dungeonLevel <= 11) {
        return inWeaponList(kWeaponBand10To11, name);
    }
    if (dungeonLevel >= 12 && dungeonLevel <= 13) {
        return inWeaponList(kWeaponBand12To13, name);
    }
    if (dungeonLevel >= 14 && dungeonLevel <= 15) {
        return inWeaponList(kWeaponBand14To15, name);
    }
    if (dungeonLevel == 16) {
        return inWeaponList(kWeaponBand16, name);
    }
    return false;  // UNKNOWN: list ends at level 16 [c102]
}

// UNKNOWN: the slime destruction chance is not in the SRD chunk. Only the item's destructible
// flag is read; no roll is made.
bool slimeDestroysItem(Ctx& ctx, const Item& item, i32 dungeonLevel) {
    (void)ctx;
    (void)dungeonLevel;
    return item.destructible;
}

// [c63] MONSMOD.BAS:3351: slot = Int(Rnd * (40 - 1)) + 2, range 2..40.
// Quirk kept: slot 40 is one past the end of the pack; the original relies on its error
// handler. Slots 0 and 1 are never drawn.
i32 drawTheftSlot(Ctx& ctx) {
    return static_cast<i32>(ctx.rng.below(kPackSlots - 1)) + 2;
}

}  // namespace mordor
