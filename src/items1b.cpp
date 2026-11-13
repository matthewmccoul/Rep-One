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

i32 clampI(i32 v, i32 lo, i32 hi) { return v < lo ? lo : (v > hi ? hi : v); }

bool packIndexValid(i32 i) { return i > 0 && i < kPackSlots; }

const ItemDef* defOf(const Ctx& ctx, i32 itemId) {
    if (itemId <= 0 || static_cast<u32>(itemId) >= ctx.data.items.size()) return nullptr;
    return &ctx.data.items[static_cast<u32>(itemId)];
}

const RaceDef* raceOf(const Ctx& ctx, const Character& c) {
    u32 r = static_cast<u32>(c.race);
    return r < ctx.data.races.size() ? &ctx.data.races[r] : nullptr;
}

Str itemNameOf(const Ctx& ctx, const Item& it) {
    const ItemDef* d = defOf(ctx, it.id);
    return d ? d->name : Str();
}

bool isGuildIssuedItem(const Item& it) { return it.guildIssued || it.isGuildItem(); }

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

Str swapMessage(SwapResult r) {
    switch (r) {
        case SwapResult::SourceGuildIssued: return Str("Source Item is a Guild-Issued Item!");
        case SwapResult::DestGuildIssued: return Str("Destination Item is a Guild-Issued Item!");
        case SwapResult::SourceEquippedOrBanked: return Str("Source Item is either Equipped or in the Bank!");
        case SwapResult::DestEquippedOrBanked: return Str("Destination Item is either Equipped or in the Bank!");
        default: return Str();
    }
}

Str alignTag(ItemAlign a) {
    switch (a) {
        case ItemAlign::Good: return Str(" (g)");
        case ItemAlign::Neutral: return Str(" (n)");
        case ItemAlign::Evil: return Str(" (e)");
        default: return Str();
    }
}

// UNKNOWN: combine alignment compatibility rule (SRD gives none).
bool alignCompatible(ItemAlign a, ItemAlign b) {
    return a == b || a == ItemAlign::Unaligned || b == ItemAlign::Unaligned;
}

Str combineSuffix(CombineCheck r) {
    switch (r) {
        case CombineCheck::NotCompatibleClass: return Str(" is not compatible class");
        case CombineCheck::NotSameType: return Str(" is not of the same type");
        case CombineCheck::NotCompatibleAlignment: return Str(" is not compatible Alignment");
        default: return Str();
    }
}

// Special type 1, 13, 14: stat mods. UNKNOWN: which mods and reversed flag per item.
void applyStatMods(Ctx& ctx, Character& c, const ItemDef& d, bool reversed) {
    for (i32 i = 0; i < kStatCount; ++i) {
        // FIXED: ItemDef member statMods does not exist; using statsMod (name suggested by compiler).
        applyStatMod(ctx, c, static_cast<StatIndex>(i), d.statsMod[i], reversed);
    }
}

// R-slime: roll 1..100 (Rnd-based). Hit when roll < 50 + dungeonLevel*2.
i32 slimeRollValue(Ctx& ctx) { return ctx.rng.below(100) + 1; }
bool slimeHits(i32 roll, i32 dungeonLevel) {
    return roll < kSlimeBaseRoll + dungeonLevel * kSlimeDungeonFactor;
}

}  // namespace

// ---------------------------------------------------------------- Pack

// R-pack: SlotEmpty(i) = i > 0 && i < 40 && slot.ItemId == 0
bool slotEmpty(const Inventory& inv, i32 internalIndex) {
    if (!packIndexValid(internalIndex)) return false;
    return inv.pack[internalIndex].id == 0;
}

// R-pack: first i in 1..39 with SlotEmpty(i), else -1.
i32 firstFreeSlot(const Inventory& inv) {
    for (i32 i = 1; i < kPackSlots; ++i) {
        if (slotEmpty(inv, i)) return i;
    }
    return -1;
}

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

// R-wear: IsWearable(it) = MaxWornOfClass(WearClassOf(it)) != 0
bool isWearable(const Ctx& ctx, const Item& it) {
    return maxWornOfClass(ctx, wearClassOf(ctx, it)) != 0;
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

// R-curse: CanUnequip(slot) = !slot.SlotCursed
bool canUnequip(const Item& slot) { return !slot.slotCursed; }

// R-curse: if it.Cursed != 0 -> SlotCursed; if FitFor == Opposed -> SlotCursed.
// Mapping: FitClass::Subtract = Opposed.
void applyEquipCurse(Ctx& ctx, Item& slot, FitClass fit) {
    (void)ctx;
    if (slot.cursed != CurseNone) slot.slotCursed = true;
    if (fit == FitClass::Subtract) slot.slotCursed = true;
}

// R-uncurse order: not cursed, then not equipped, else Ok.
UncurseResult checkUncurse(const Item& slot) {
    if (!slot.slotCursed) return UncurseResult::NotCursed;
    if (!slot.equipped) return UncurseResult::NotEquipped;
    return UncurseResult::Ok;
}

// UNKNOWN: uncurse fee and FIDELITY field details (M572A, M577E) not given.
UncurseResult uncurseSlot(Ctx& ctx, Item& slot) {
    UncurseResult r = checkUncurse(slot);
    if (r == UncurseResult::NotCursed) {
        ctx.say("items.uncurseNotCursed", "Item is not cursed!", {}, {});
        return r;
    }
    if (r == UncurseResult::NotEquipped) {
        ctx.say("items.uncurseNotEquipped", "Item isn't equipped!", {}, {});
        return r;
    }
    slot.slotCursed = false;
    return UncurseResult::Ok;
}

// R-forced: if EquipProblem(item) != null and !SlotCursed then Equipped = false.
void forcedUnequipCheck(Ctx& ctx, const Character& c, const Inventory& inv, Item& slot) {
    if (equipProblem(ctx, c, inv, slot) != EquipProblem::None && !slot.slotCursed) {
        slot.equipped = false;
    }
}

// ---------------------------------------------------------------- Drop, give, bank

// T-10: guild-issued items cannot be dropped.
bool canDropSlot(const Item& slot) { return !isGuildIssuedItem(slot); }

// T-10 give refusals, in order: guild-issued, equipped, no free slot.
GiveRefusal checkGive(const Item& slot, const Inventory& recipient) {
    if (isGuildIssuedItem(slot)) return GiveRefusal::GuildIssued;
    if (slot.equipped) return GiveRefusal::Equipped;
    if (firstFreeSlot(recipient) < 0) return GiveRefusal::NoFreeSlot;
    return GiveRefusal::None;
}

bool giveSlot(Ctx& ctx, Inventory& from, i32 fromSlot, Inventory& to) {
    if (!packIndexValid(fromSlot)) return false;
    Item& src = from.pack[fromSlot];
    if (src.id <= 0) return false;
    GiveRefusal r = checkGive(src, to);
    if (r != GiveRefusal::None) {
        Str name = itemNameOf(ctx, src);
        if (r == GiveRefusal::GuildIssued) {
            ctx.say("items.giveGuildIssued", "Cannot give Guild-Issued Items!", {}, {});
        } else if (r == GiveRefusal::Equipped) {
            ctx.say("items.giveEquipped", "Cannot give equipped items!", {}, {});
        } else {
            ctx.say("items.giveNoSlot", Str("\"") + name + Str("\" has no free slots!"), {}, {name});
        }
        return false;
    }
    i32 dest = firstFreeSlot(to);
    to.pack[dest] = src;
    src = Item{};
    return true;
}

// T-10: equipped items cannot be banked.
bool canBankItem(const Item& slot) { return !slot.equipped; }

// R-bank deposit: if amount > hand or amount <= 0: Beep; bank += amount; hand -= amount.
BankResult depositGold(Ctx& ctx, Purse& purse, Currency amount) {
    (void)ctx;
    if (amount > purse.goldOnHand || amount <= 0) return BankResult::Beep;
    purse.goldInBank += amount;
    purse.goldOnHand -= amount;
    return BankResult::Ok;
}

// R-bank withdrawal: if amount > bank or bank < 0: Beep; hand += amount; bank -= amount.
BankResult withdrawGold(Ctx& ctx, Purse& purse, Currency amount) {
    (void)ctx;
    if (amount > purse.goldInBank || purse.goldInBank < 0) return BankResult::Beep;
    purse.goldOnHand += amount;
    purse.goldInBank -= amount;
    return BankResult::Ok;
}

// CONFLICT: SRD says re-identify if probe < threshold; the port reads it as
// greater-than [c28]. Preferred port behaviour used.
bool shouldReidentifyOnRetrieval(i32 probe, i32 threshold) { return probe > threshold; }

// UNKNOWN: check order (free slot before gold) is a chosen order. T-10 lists both.
BuyResult buyItem(Ctx& ctx, Purse& purse, Inventory& inv, const Item& item, Currency price) {
    i32 slot = firstFreeSlot(inv);
    if (slot < 0) {
        ctx.say("items.buyNoSlot", "No free slots!", {}, {});
        return BuyResult::NoFreeSlot;
    }
    if (purse.goldOnHand < price) {
        ctx.say("items.buyNoGold", "Not enough gold!", {}, {});
        return BuyResult::NotEnoughGold;
    }
    purse.goldOnHand -= price;
    inv.pack[slot] = item;
    return BuyResult::Ok;
}

// ---------------------------------------------------------------- Swap

// R-swap (internal): refuse if slotFrom or slotTo is outside 1..39.
// Then T-10 order: guild-issued source, guild-issued dest, equipped source, equipped dest.
SwapResult checkSwap(const Inventory& from, i32 slotFrom, const Inventory& to, i32 slotTo) {
    if (!packIndexValid(slotFrom) || !packIndexValid(slotTo)) return SwapResult::OutOfRange;
    const Item& a = from.pack[slotFrom];
    const Item& b = to.pack[slotTo];
    if (isGuildIssuedItem(a)) return SwapResult::SourceGuildIssued;
    if (isGuildIssuedItem(b)) return SwapResult::DestGuildIssued;
    if (a.equipped) return SwapResult::SourceEquippedOrBanked;
    if (b.equipped) return SwapResult::DestEquippedOrBanked;
    return SwapResult::Ok;
}

bool swapPackSlots(Ctx& ctx, Inventory& from, i32 slotFrom, Inventory& to, i32 slotTo) {
    SwapResult r = checkSwap(from, slotFrom, to, slotTo);
    if (r == SwapResult::OutOfRange) return false;  // refused silently [R-swap]
    if (r != SwapResult::Ok) {
        ctx.say("items.swapRefused", swapMessage(r), {}, {});
        return false;
    }
    Item tmp = from.pack[slotFrom];
    from.pack[slotFrom] = to.pack[slotTo];
    to.pack[slotTo] = tmp;
    return true;
}

// R-swap (player): A, B in 2..40 (player numbering). Internal index = player - 1.
// Buffers pointing at either slot follow the item only if buffersFollow.
bool swapPlayerSlots(Ctx& ctx, Character& c, Inventory& inv, i32 playerA, i32 playerB, bool buffersFollow) {
    (void)ctx;
    if (!isValidPlayerSlot(playerA) || !isValidPlayerSlot(playerB)) return false;
    i32 a = playerA - 1;
    i32 b = playerB - 1;
    Item tmp = inv.pack[a];
    inv.pack[a] = inv.pack[b];
    inv.pack[b] = tmp;
    if (buffersFollow) {
        for (i32 k = 0; k < kSpellBuffers; ++k) {
            i32& v = c.order.spellBuffers[k];
            if (v == a) {
                v = b;
            } else if (v == b) {
                v = a;
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------- Slime, theft

// R-slime: no effect if roll >= 50 + dungeonLevel*2.
bool slimeStrikes(Ctx& ctx, i32 dungeonLevel) {
    return slimeHits(slimeRollValue(ctx), dungeonLevel);
}

// R-slime: slot = Next(39) + 1, internal 1..39.
i32 slimeSlot(Ctx& ctx) { return ctx.rng.below(kSlimeSlotDraw) + 1; }

// R-destroy: non-paralysed = (45 - LevelFound) + (dex+dex)*2 - monDex*2.
// Paralysed = VbInt((45 - LevelFound) + (dex+dex)/2.0 - monDex*2).  Clamp 25..90.
i32 destroyChance(i32 levelFound, i32 charDex, i32 monDex, bool paralysed) {
    i32 raw;
    if (!paralysed) {
        raw = (kDestroyBase - levelFound) + (charDex + charDex) * kDestroyDexFactor
            - monDex * kDestroyMonDexFactor;
    } else {
        double v = static_cast<double>(kDestroyBase - levelFound)
            + static_cast<double>(charDex + charDex) / 2.0
            - static_cast<double>(monDex * kDestroyMonDexFactor);
        raw = static_cast<i32>(vbInt(v));
    }
    return clampI(raw, kDestroyMin, kDestroyMax);
}

// R-destructible: SpecialType < 500 OR SpecialType > 500 + GuildCount - 1.
bool isDestructible(i32 specialType) {
    return specialType < kGuildMarkerBase || specialType > kGuildMarkerBase + kGuildCount - 1;
}

// R-slime gold: Int(Rnd * (((DL + monAtt) * (3353.452 * monLvl)) / max(1, GuildLevel))).
// UNKNOWN: result is used as Currency units unchanged.
Currency slimeGold(Ctx& ctx, i32 dungeonLevel, i32 monAtt, i32 monLvl, i32 guildLevel) {
    f64 denom = static_cast<f64>(guildLevel < 1 ? 1 : guildLevel);
    f64 scale = (static_cast<f64>(dungeonLevel + monAtt) * (kSlimeGoldCoef * static_cast<f64>(monLvl))) / denom;
    return vbInt(ctx.rng.nextDouble() * scale);
}

// R-theft: v = score - monLvl*2. Gold if v < 50; item if v <= 10.
TheftFlags theftFlags(i32 score, i32 monLvl) {
    i32 v = score - monLvl * kMonLvlScoreFactor;
    TheftFlags f;
    f.gold = v < kTheftScoreLimit;
    f.item = v <= kItemTheftScoreLimit;
    return f;
}

// R-theft slot: Int(Rnd * 39) + 2, internal 2..40 (kTheftDraw = 39).
i32 theftSlot(Ctx& ctx) { return ctx.rng.below(kTheftDraw) + 2; }

// R-slime flow: strike roll; slot draw; empty slot = no effect;
// destroy if destructible and chance rolls; theft flags and gold from strike roll.
// UNKNOWN: "score" is the strike roll (1..100). Item theft slot is not modelled;
// callers may use theftSlot().
SlimeResult rollSlime(Ctx& ctx, Inventory& inv, const SlimeInput& in) {
    SlimeResult r;  // slot = -1 means no effect
    i32 roll = slimeRollValue(ctx);
    if (!slimeHits(roll, in.dungeonLevel)) return r;
    i32 slot = slimeSlot(ctx);
    if (slotEmpty(inv, slot) || inv.pack[slot].id <= 0) return r;
    r.slot = slot;
    Item& it = inv.pack[slot];
    const ItemDef* d = defOf(ctx, it.id);
    i32 special = d ? d->specialType : 0;
    if (isDestructible(special)) {
        i32 chance = destroyChance(it.level, in.charDex, in.monDex, in.paralysed);
        if (ctx.rng.percent(chance)) {
            r.destroyed = true;
            Str name = itemNameOf(ctx, it);
            ctx.say("items.slimeDestroyed", Str("SLIME: ") + name + Str(" Destroyed!"), {}, {name});
            it = Item{};
        }
    }
    r.theft = theftFlags(roll, in.monLvl);
    if (r.theft.gold) {
        r.goldTaken = slimeGold(ctx, in.dungeonLevel, in.monAtt, in.monLvl, in.guildLevel);
    }
    return r;
}

// ---------------------------------------------------------------- Alignment fit, worth

// R-worth: Full -> raw; Half -> round_half_even(raw / 2.0); Subtract (Opposed) -> -raw.
i32 attDefWorth(i32 raw, FitClass fit) {
    switch (fit) {
        case FitClass::Full: return raw;
        case FitClass::Half: return static_cast<i32>(roundHalfEven(static_cast<f64>(raw) / 2.0));
        case FitClass::Subtract: return -raw;
    }
    return raw;
}

// R-halved: Int(x) / 2 with half-to-even (5 -> 2, 7 -> 4).
i32 attackDefenceHalved(i32 x) {
    return static_cast<i32>(roundHalfEven(static_cast<f64>(x) / 2.0));
}

// R-halved: Int(x / 2), which floors (5 -> 2, 7 -> 3).
i32 statModHalved(i32 x) {
    return static_cast<i32>(vbInt(static_cast<f64>(x) / 2.0));
}

// R-effect-bit: counts iff item alignment == 0 (unaligned) or item-1 == char alignment.
bool effectBitCounts(ItemAlign itemAlign, Align charAlign) {
    if (itemAlign == ItemAlign::Unaligned) return true;
    return static_cast<i32>(itemAlign) - 1 == static_cast<i32>(charAlign);
}

// R-effects: OR over counting equipped items of (Abilities & 0x3F).
// UNKNOWN: scans equipped pack items (Inventory.worn not used).
u32 gearEffects(const Ctx& ctx, const Character& c, const Inventory& inv) {
    u32 e = 0;
    for (i32 i = 1; i < kPackSlots; ++i) {
        const Item& it = inv.pack[i];
        if (!it.equipped || it.id <= 0) continue;
        if (!effectBitCounts(it.alignment, c.alignment)) continue;
        const ItemDef* d = defOf(ctx, it.id);
        if (d) e |= (d->abilities & kEffectMask);
    }
    return e & kEffectMask;
}

// ---------------------------------------------------------------- Combine

// R-combine: combinable kinds 23..28, 30, 31.
bool isCombinableType(i32 typeNumber) {
    return (typeNumber >= 23 && typeNumber <= 28) || typeNumber == 30 || typeNumber == 31;
}

// T-10 combine refusals. UNKNOWN: "class" is taken as the combinable-kind test.
// Order: class, same type, alignment, overflow (sum outside -32768..32767).
CombineCheck checkCombine(const Ctx& ctx, const Item& first, const Item& next) {
    (void)ctx;
    // R54: the first item's type must be combinable; each later item is compared on type.
    // The item's own type field is used (no definition lookup by id).
    if (!isCombinableType(first.type) || !isCombinableType(next.type)) return CombineCheck::NotCompatibleClass;
    if (first.type != next.type) return CombineCheck::NotSameType;
    // GAP: R54 also requires IdLevel::All on both items and equal item ids. CombineCheck has no
    // NotIdentified / NotSameId value in the supplied headers, so those checks are not modelled.
    if (!alignCompatible(first.alignment, next.alignment)) return CombineCheck::NotCompatibleAlignment;
    const i64 sum = static_cast<i64>(first.charges) + static_cast<i64>(next.charges);
    if (sum > kChargesMax || sum < kChargesMin) return CombineCheck::Overflow;   // R56
    return CombineCheck::Ok;
}

// R-combine: first.Charges = sum; item.ItemId = 0.
CombineCheck combineCharges(Ctx& ctx, Item& first, Item& next) {
    CombineCheck r = checkCombine(ctx, first, next);
    if (r == CombineCheck::Ok) {
        first.charges = first.charges + next.charges;
        next.id = 0;
        return r;
    }
    if (r != CombineCheck::Overflow) {
        ctx.say("items.combineRefused", itemNameOf(ctx, next) + combineSuffix(r), {}, {});
    }
    return r;
}

// R-use: MustBeEquipped = (1 <= T <= 22) or T in {29, 32, 33, 34}.
bool mustBeEquipped(i32 typeNumber) {
    return (typeNumber >= kMustEquipTypeMin && typeNumber <= kMustEquipTypeMax)
        || typeNumber == 29 || typeNumber == 32 || typeNumber == 33 || typeNumber == 34;
}

// R-use: Consumed = (23 <= T <= 28) or T in {30, 31}.
bool isConsumedType(i32 typeNumber) {
    return (typeNumber >= kConsumedTypeMin && typeNumber <= kConsumedTypeMax)
        || typeNumber == 30 || typeNumber == 31;
}

// Use checks (T-10 messages are emitted by the caller). Order chosen:
// dungeon, empty slot, no effect, must be equipped, out of charges.
// UNKNOWN: "no effect" test (no spell and no special type) is inferred.
UseCheck checkUse(const Ctx& ctx, const Character& c, const Inventory& inv, i32 internalSlot, bool inDungeon) {
    (void)c;
    if (!inDungeon) return UseCheck::NotInDungeon;
    if (!packIndexValid(internalSlot)) return UseCheck::NoEffect;
    const Item& it = inv.pack[internalSlot];
    const ItemDef* d = defOf(ctx, it.id);
    if (!d) return UseCheck::NoEffect;
    if (d->specialType == 0 && it.spellId == 0) return UseCheck::NoEffect;
    if (mustBeEquipped(d->type) && !it.equipped) return UseCheck::MustBeEquipped;
    if (isConsumedType(d->type) && it.charges <= 0) return UseCheck::OutOfCharges;
    return UseCheck::Ok;
}

// R-use: consumed items vanish when empty.
void removeIfExhausted(Ctx& ctx, Inventory& inv, i32 internalSlot) {
    if (!packIndexValid(internalSlot)) return;
    Item& it = inv.pack[internalSlot];
    const ItemDef* d = defOf(ctx, it.id);
    if (d && isConsumedType(d->type) && it.charges <= 0) it = Item{};
}

// T-4 special types. Type 25 (ages 28 days) is not in T-4 and is not implemented.
// UNKNOWN: reversed flag for specialType stat mods is taken as false.
void applySpecialType(Ctx& ctx, Character& c, const ItemDef& def, i32 specialType) {
    switch (static_cast<SpecialEffect>(specialType)) {
        case SpecialEffect::StatMods:
            applyStatMods(ctx, c, def, false);
            break;
        case SpecialEffect::SpellPoints50:
            c.vitals.curSpellPoints += 50;  // UNKNOWN: no cap applied
            break;
        case SpecialEffect::SpellPoints200:
            c.vitals.curSpellPoints += 200;  // UNKNOWN: no cap applied
            break;
        case SpecialEffect::PotionOfYouth:
            youngerBy(ctx, c, kYouthYears);
            applyStatMods(ctx, c, def, false);
            break;
        case SpecialEffect::DragonsBlood:
            youngerBy(ctx, c, kDragonsBloodYears);
            applyStatMods(ctx, c, def, false);
            break;
        default:
            break;
    }
}

// R-age: age -= years*365; if age < 16*365: age = 16*365 and message.
void youngerBy(Ctx& ctx, Character& c, i32 years) {
    c.ageDays -= years * kYearDays;
    if (c.ageDays < kCharacterMinAge) {
        c.ageDays = kCharacterMinAge;
        ctx.say("items.cannotGetYounger", "You cannot get any younger.", {}, {});
    }
}

// R-stat: ceiling = race.Max[i] + 5; reversed and mod > 0 -> stat -= mod, else stat += mod;
// clamp to ceiling (silent), then to race.Min. Returns the new stat value.
// UNKNOWN: Hall of Records check on each raise [c11] is not modelled.
// FIXED: RaceDef has no statMax / statMin members. The ceiling now uses the
// character's own abilities.statMax (closest existing equivalent in the API index),
// and the race.Min clamp is omitted because no existing member holds it.
// The owner of RaceDef should confirm the intended source for both limits.
i32 applyStatMod(Ctx& ctx, Character& c, StatIndex stat, i32 mod, bool reversed) {
    i32 idx = static_cast<i32>(stat);
    i32 cur = c.abilities.stats[idx];
    const RaceDef* race = raceOf(ctx, c);
    if (!race) return cur;
    // R (ch13): ceiling = race.Max[i] + 5, taken from the race record (ItemUse.cs ApplyStatMods).
    // The cap is silent; a raise that reaches it is still applied.
    i32 ceiling = race->max[idx] + kStatCeilingBonus;
    i32 s = cur;
    if (reversed && mod > 0) {
        s -= mod;
    } else {
        s += mod;
    }
    if (s > ceiling) s = ceiling;
    // UNKNOWN: ch13 also has "if stat < race.Min[i] then stat = race.Min[i]". The RaceDef
    // minimum field name is not in the packet, so the floor is not applied here. See NEED.
    c.abilities.stats[idx] = s;
    return s;
}

// ---------------------------------------------------------------- Identification

bool rollIdentifyTick(Ctx& ctx, f64 skill, bool dark) {
    return ctx.rng.percent(identifyChance(skill, dark));
}

bool rollRecogniseTick(Ctx& ctx, f64 skill, bool dark) {
    return ctx.rng.percent(recogniseChance(skill, dark));
}

bool rollRename(Ctx& ctx, f64 identifySkill, bool inFog) {
    return ctx.rng.percent(renameChance(identifySkill, inFog));
}

// ---------------------------------------------------------------- Found alignment, charges

// R-align: for a in 0..3, if bit a set add it; empty -> Unaligned.
// UNKNOWN: loop bound 0..3 taken from the bit list in the SRD.
Vec<ItemAlign> allowedAlignments(u8 alignmentFlags) {
    Vec<ItemAlign> out;
    for (i32 a = 0; a < 4; ++a) {
        if (((static_cast<u32>(alignmentFlags) >> a) & 1u) != 0u) {
            out.push_back(static_cast<ItemAlign>(a));
        }
    }
    if (out.empty()) out.push_back(ItemAlign::Unaligned);
    return out;
}

// R-align found: Align <= 1 -> Unaligned; else uniform choice among set bits of
// foundAlign in 0..3 (empty set -> Unaligned).
// UNKNOWN: alignmentFlags is not used by the SRD formula for this step.
ItemAlign rollFoundAlignment(Ctx& ctx, u8 alignmentFlags, i32 foundAlign) {
    (void)alignmentFlags;
    if (foundAlign <= 1) return ItemAlign::Unaligned;
    Vec<ItemAlign> cands;
    for (i32 a = 0; a < 4; ++a) {
        if (((foundAlign >> a) & 1) != 0) cands.push_back(static_cast<ItemAlign>(a));
    }
    if (cands.empty()) return ItemAlign::Unaligned;
    i32 pick = ctx.rng.below(static_cast<i32>(cands.size()));
    return cands[static_cast<u32>(pick)];
}

// R-charges: if base <= 0 -> 0. n = base; if base > 1: n += (rng(2)==0 ? 1 : -1) *
// Int(NextDouble * base / 2). Return max(0, n).
i32 rollFoundCharges(Ctx& ctx, i32 baseCharges) {
    if (baseCharges <= 0) return 0;
    i32 n = baseCharges;
    if (baseCharges > 1) {
        i32 sign = ctx.rng.below(2) == 0 ? 1 : -1;
        i32 amount = static_cast<i32>(vbInt(ctx.rng.nextDouble() * static_cast<f64>(baseCharges) / 2.0));
        n += sign * amount;
    }
    return n < 0 ? 0 : n;
}

// R-type names: ItemDescriptions[ItemTypes[kind].Index].
Str itemKindName(const Ctx& ctx, i32 kind) {
    if (kind < 0 || static_cast<u32>(kind) >= ctx.data.itemTypes.size()) return Str();
    i32 idx = ctx.data.itemTypes[static_cast<u32>(kind)].index;
    if (idx < 0 || static_cast<u32>(idx) >= ctx.data.itemDescriptions.size()) return Str();
    return ctx.data.itemDescriptions[static_cast<u32>(idx)].name;
}

// ---------------------------------------------------------------- Seer


// R-seer: hit if entry == item.Type and item.Floor <= m.ItemDropLevel <= item.Floor + band.
bool seerMatchesByType(i32 entry, const ItemDef& item, i32 band, i32 monsterDropLevel) {
    return entry == item.type
        && item.floor <= monsterDropLevel
        && monsterDropLevel <= item.floor + band;
}

// R-seer: certain if entry < -1 and Abs(entry) - 2 == item.Id.
bool seerMatchesById(i32 entry, i32 itemId) {
    return entry < -1 && (-entry - 2) == itemId;
}

// R-seer take: Round(n - Rnd*n, half-to-even); clamp max 3, max n, min 1.
i32 seerTakeCount(Ctx& ctx, i32 n) {
    i64 take = roundHalfEven(static_cast<f64>(n) - ctx.rng.nextDouble() * static_cast<f64>(n));
    if (take > kSeerTakeMax) take = kSeerTakeMax;
    if (take > n) take = n;
    if (take < 1) take = 1;
    return static_cast<i32>(take);
}

// ---------------------------------------------------------------- Lock opening, realign

// R-lock: if Items(i).M56C4 > 0 then spell = SpellDefs(M56C4 - 1).
// FIXED: ItemDef has no lockSpell member and no existing member in the API index
// holds the lock spell position. The lookup is disabled (returns nullptr) until
// the owner of ItemDef names the field. Original logic is kept in comments below.
//   if (!d || d->lockSpell <= 0) return nullptr;
//   u32 idx = static_cast<u32>(d->lockSpell - 1);
//   if (idx >= ctx.data.spells.size()) return nullptr;
//   return &ctx.data.spells[idx];
const SpellDef* lockOpeningSpell(const Ctx& ctx, const Item& it) {
    const ItemDef* d = defOf(ctx, it.id);
    (void)d;
    return nullptr;
}

// R-realign: mult = |slot - (store + 1)| > 1 ? 2 : 1; cost = round_half_even(0.72 * Price * mult).
i64 realignCost(i64 basePrice, i32 slotAlignIndex, i32 storeAlignIndex) {
    i32 diff = slotAlignIndex - (storeAlignIndex + 1);
    if (diff < 0) diff = -diff;
    i32 mult = diff > kRealignFarGap ? kRealignFarMult : kRealignNearMult;
    f64 raw = kRealignRate * static_cast<f64>(basePrice) * static_cast<f64>(mult);
    return roundHalfEven(raw);
}

// T-11: refused while equipped [c22].
bool canRealign(const Item& slot) { return !slot.equipped; }

}  // namespace mordor
