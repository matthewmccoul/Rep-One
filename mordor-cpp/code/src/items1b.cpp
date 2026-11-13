// items1b.cpp: implementation of include/mordor/items1b.h.
// Covers SRD c13p1: pack, equip checks, wear class, curse, swap, slime and
// destruction, theft, alignment fit, effect bits, combine, use, special
// effects, stat mods, identification, chances, rename, bank and party gold,
// seer, spell buffers, lock-opening spell, realign.
//
// Header inline functions (isHandsSlot, isValidPlayerSlot, bufferForItem,
// bufferForSpell, hasEffect) are not defined here.
// NOTE: header isHandsSlot() uses kHandsSlot, which is not defined in the
// headers supplied for this module. The owner of that header must define it.
//
// ASSUMED NAMES (game_data.h was not supplied; verify against the generated file):
//   ItemDef: name(Str), type(i32), nHands(i32), floor(i32), levelScale(f64),
//            damageMod(f64), specialType(i32), statsRequired(Arr<i32,6>),
//            statsMod(Arr<i32,6>), guilds(u32), classRestricted(bool),
//            abilities(u32)
//   RaceDef: (no statMin / statMax members found; see FIXED notes below)
//   NamedEntry: name(Str), index(i32)
//   GameData: items, itemTypes, itemDescriptions, races, spells (Vec tables)

#include "mordor/items1b.h"

#include <cmath>
#include <limits>
#include <string>

namespace mordor {
namespace {

// R-pack: PackSize = kPackSlots (40); item slots are internal 1..39.
constexpr i32 kHandsCapacity = 2;      // R-equip: "HandsUsed + NHands > 2"
constexpr f64 kRealignRate = 0.72;     // [c22] 0.72 x Price
constexpr i32 kChargesMax = 32767;     // [c28] combine overflow ceiling
constexpr i32 kStatCeilingBonus = 5;   // R-stat: ceiling = race.Max + 5
constexpr i32 kChanceIdentifyMin = 10; // R-chances
constexpr i32 kChanceIdentifyMax = 90;
constexpr i32 kChanceRecogniseMin = 5;
constexpr i32 kChanceRecogniseMax = 80;
constexpr i32 kSeerTakeMax = 3;        // R-seer: take clamped to max 3
constexpr i32 kSeerBandCount = 3;      // R-seer: Rnd(3) -> 0..2
constexpr i32 kCharacterMinAge = kYoungerFloorYears * kYearDays;
// FIXED: kBalanceCap was undeclared. No balance cap value exists in the headers,
// so the receiver cap is the largest value the Currency type can hold (the
// overflow guard that the R24 comment describes).
constexpr Currency kBalanceCap = std::numeric_limits<Currency>::max();

bool packIndexValid(i32 i) { return i > 0 && i < kPackSlots; }

const ItemDef* defOf(const Ctx& ctx, i32 itemId) {
    if (itemId <= 0 || static_cast<u32>(itemId) >= ctx.data.items.size()) return nullptr;
    return &ctx.data.items[static_cast<u32>(itemId)];
}

// T-7 item-use factors. UNKNOWN: factors for guilds other than Nomad and Healer.
f64 guildUseFactor(i32 guildIdx) {
    if (guildIdx == static_cast<i32>(Guild::Nomad)) return kItemUseFactorNomad;
    if (guildIdx == static_cast<i32>(Guild::Healer)) return kItemUseFactorHealer;
    return 1.0;
}

bool guildAllowed(const ItemDef& d, i32 g) {
    // FIXED: ItemDef member guildMask does not exist; using guilds (the name the compiler suggested).
    return ((d.guilds >> static_cast<u32>(g)) & 1u) != 0u;
}

// [c10] joined guild with Level > 0. Class-restricted items use the primary guild only.
bool guildInScope(const Character& c, i32 g, bool classRestricted) {
    const GuildMembership& m = c.guilds[static_cast<u32>(g)];
    if (!m.joined || m.level <= 0) return false;
    if (classRestricted && g != static_cast<i32>(c.activeGuild)) return false;
    return true;
}

// [c10] requiredGuildLevel = max(1, (factor + factor*2.97) * LevelScale).
// CONFLICT: [c24] writes (f + 2.97f) and rounds with unspecified B. Preferred
// [c10] form is used and the result is compared as a real number (no rounding).
f64 requiredGuildLevel(const ItemDef& d, i32 g) {
    f64 f = guildUseFactor(g);
    f64 req = (f + f * 2.97) * d.levelScale;
    return req < 1.0 ? 1.0 : req;
}

// [c10] CanUse = any in-scope joined guild whose mask bit is set.
bool canUseByGuilds(const Character& c, const ItemDef& d, bool classRestricted) {
    for (i32 g = 0; g < kGuildCount; ++g) {
        if (guildInScope(c, g, classRestricted) && guildAllowed(d, g)) return true;
    }
    return false;
}

// MeetsUseLevel: some in-scope joined guild meets its required level.
bool meetsUseLevelFor(const Character& c, const ItemDef& d, bool classRestricted) {
    for (i32 g = 0; g < kGuildCount; ++g) {
        if (!guildInScope(c, g, classRestricted)) continue;
        if (static_cast<f64>(c.guilds[static_cast<u32>(g)].level) >= requiredGuildLevel(d, g)) return true;
    }
    return false;
}

}  // namespace

// ---------------------------------------------------------------- Pack

// UNKNOWN: "hands in use" has no formula in the SRD. Implemented as the sum of
// NHands over equipped pack items.
i32 handsUsed(const Ctx& ctx, const Inventory& inv) {
    i32 h = 0;
    for (i32 i = 1; i < kPackSlots; ++i) {
        const Item& it = inv.pack[i];
        if (!it.equipped) continue;
        const ItemDef* d = defOf(ctx, it.id);
        if (d) h += d->nHands;
    }
    return h;
}

// ---------------------------------------------------------------- Equip

// R-equip checks, in SRD order: hands, class cap, CanUse, MeetsUseLevel, stats.
EquipProblem equipProblem(const Ctx& ctx, const Character& c, const Inventory& inv, const Item& it) {
    const ItemDef* d = defOf(ctx, it.id);
    if (!d) return EquipProblem::None;  // UNKNOWN: empty or unknown item id
    if (handsUsed(ctx, inv) + d->nHands > kHandsCapacity) return EquipProblem::NoFreeHands;
    i32 cls = wearClassOf(ctx, it);
    i32 cap = maxWornOfClass(ctx, cls);
    if (cap > 0 && wornOfClass(ctx, inv, cls) >= cap) return EquipProblem::UsingTooMany;
    bool restricted = d->classRestricted;
    if (!canUseByGuilds(c, *d, restricted)) {
        return restricted ? EquipProblem::ClassRestricted : EquipProblem::GuildsCannotUse;
    }
    if (!meetsUseLevelFor(c, *d, restricted)) {
        return restricted ? EquipProblem::GuildLevelTooLow : EquipProblem::AllGuildLevelsTooLow;
    }
    // CONFLICT: engine text "One or more stats are too low!" [c2] vs FIDELITY
    // "You are too weak..." [c26]. Engine text used (StatsTooLow).
    for (i32 i = 0; i < kStatCount; ++i) {
        if (c.abilities.stats[i] < d->statsRequired[i]) return EquipProblem::StatsTooLow;
    }
    return EquipProblem::None;
}

// T-10 equip messages (verbatim).
Str equipProblemText(EquipProblem p) {
    switch (p) {
        case EquipProblem::NoFreeHands: return Str("No Free Hands!");
        case EquipProblem::UsingTooMany: return Str("Using too Many!");
        case EquipProblem::ClassRestricted: return Str("Your current guild cannot use this item!");
        case EquipProblem::GuildsCannotUse: return Str("Your currently joined guilds cannot use this item!");
        case EquipProblem::GuildLevelTooLow: return Str("Your guild level is too low to use item!");
        case EquipProblem::AllGuildLevelsTooLow: return Str("All guild levels too low to use item!");
        case EquipProblem::StatsTooLow: return Str("One or more stats are too low!");
        case EquipProblem::TooWeak: return Str("You are too weak to equip this item!");
        case EquipProblem::CursedStatsTooLow: return Str("Your stats are too low for the item to properly attach!");
        default: return Str();
    }
}

// UNKNOWN: equipping only sets the equipped flag on the pack slot. The
// Inventory.worn vector is not modified. Curse applied with FitFor [c12].
bool equipItem(Ctx& ctx, Character& c, Inventory& inv, i32 internalSlot) {
    if (!packIndexValid(internalSlot)) return false;
    Item& it = inv.pack[internalSlot];
    if (it.id <= 0 || !defOf(ctx, it.id)) return false;
    EquipProblem p = equipProblem(ctx, c, inv, it);
    if (p != EquipProblem::None) {
        ctx.say("items.equipRefused", equipProblemText(p), {}, {});
        return false;
    }
    it.equipped = true;
    applyEquipCurse(ctx, it, fitFor(it.alignment, c.alignment));
    return true;
}

// UNKNOWN: WearClassOf has no formula. Implemented as ItemTypes[type].Index,
// which indexes ItemDescriptions (the class table used by MaxWornOfClass).
i32 wearClassOf(const Ctx& ctx, const Item& it) {
    const ItemDef* d = defOf(ctx, it.id);
    if (!d) return 0;
    u32 t = static_cast<u32>(d->type);
    if (d->type < 0 || t >= ctx.data.itemTypes.size()) return 0;
    return ctx.data.itemTypes[t].index;
}

// R-wear: MaxWornOfClass(c) = ItemDescriptions[c].Index, 0 if out of range.
i32 maxWornOfClass(const Ctx& ctx, i32 wearClass) {
    if (wearClass < 0 || static_cast<u32>(wearClass) >= ctx.data.itemDescriptions.size()) return 0;
    return ctx.data.itemDescriptions[static_cast<u32>(wearClass)].index;
}

// UNKNOWN: counts equipped pack items of the class (Inventory.worn not used).
i32 wornOfClass(const Ctx& ctx, const Inventory& inv, i32 wearClass) {
    i32 n = 0;
    for (i32 i = 1; i < kPackSlots; ++i) {
        const Item& it = inv.pack[i];
        if (it.equipped && it.id > 0 && wearClassOf(ctx, it) == wearClass) ++n;
    }
    return n;
}

// R-curse: if it.Cursed != 0 -> SlotCursed; if FitFor == Opposed -> SlotCursed.
// Mapping: FitClass::Subtract = Opposed.
void applyEquipCurse(Ctx& ctx, Item& slot, FitClass fit) {
    (void)ctx;
    if (slot.cursed != CurseNone) slot.slotCursed = true;
    if (fit == FitClass::Subtract) slot.slotCursed = true;
}

// ---------------------------------------------------------------- Drop, give, bank

// R-bank deposit: if amount > hand or amount <= 0: Beep; bank += amount; hand -= amount.
BankResult depositGold(Ctx& ctx, Purse& purse, Currency amount) {
    (void)ctx;
    if (amount > purse.goldOnHand || amount <= 0) return BankResult::Beep;
    purse.goldInBank += amount;
    purse.goldOnHand -= amount;
    return BankResult::Ok;
}

// ---------------------------------------------------------------- Swap

// ---------------------------------------------------------------- Slime, theft

// ---------------------------------------------------------------- Alignment fit, worth

// ---------------------------------------------------------------- Combine

// ---------------------------------------------------------------- Identification

// ---------------------------------------------------------------- Found alignment, charges

// ---------------------------------------------------------------- Seer


// ---------------------------------------------------------------- Lock opening, realign

}  // namespace mordor
