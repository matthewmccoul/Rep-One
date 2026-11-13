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

}  // namespace

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

// [c87] idFee ~= sellValue ("The Clerk's Rule").
// CONFLICT: wiki (%2Aid-and-uncurse.md) also gives 0.10-0.20 * base market value; the
// sellValue rule is chosen as the stated preferred behaviour. Sell value approximated by priceOfItem.
i64 identifyFee(const Ctx& ctx, const Item& item) {
    return priceOfItem(ctx, item, item.alignment);
}

}  // namespace mordor
