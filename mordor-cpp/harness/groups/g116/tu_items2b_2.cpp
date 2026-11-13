// items2b_2.cpp: Part B items, second half. Store, identify/realign/uncurse fees,
// combine, companion confinement, exchange window, gold charge, misc helpers.
//
// STATUS: the 8 reported errors are fixed. UncurseOutcome gained Refused and CannotAfford;
// RealignOutcome gained Refused, NeedsConfirm and CannotAfford (all ADDED in items1a.h).
// The uses below now match those enumerators.
//
// CONFLICT: enumerator values for EquipOutcome, UseOutcome, DropOutcome, RealignOutcome,
// BuyOutcome, CombineOutcome (items1a.h) and SwapResult (items1b.h) were not supplied.
// FIXED: CombineOutcome::NothingDone -> NothingToCombine (existing enumerator per compiler).
// CONFLICT: combine refusal pairing. The SRD says the type-differs and class/id-differs
// message pairings are swapped and kept. Here: TypeDiffers for different types,
// ItemIdDiffers for different ids. Text per refusal is UNKNOWN.
// UNKNOWN: store, fee, price and binding formulas (storeSalePrice, identifyFee,
// realignFee, bindingCost, priceToIdentify, priceToRealign, priceToSell) are not in the
// dossier. Only half-even rounding is given for fees. Functions return 0 pending data.

#include "mordor/items1a.h"
#include "mordor/items1b.h"
#include "mordor/items2b.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

i32 freeSlot(const Inventory& inv) {
    for (i32 i = 1; i <= kLastPackIndex; ++i) {
        if (isEmptySlot(inv.pack[i])) return i;
    }
    return -1;
}

bool validSlot(i32 slot) {
    return slot >= 1 && slot <= kLastPackIndex;
}

}  // namespace


// ---- R59: buying. Depletion decision is made by the caller from these flags.
bool buyDepletesShelf(bool dungeonItem, bool alignedFloorZeroCopy) {
    return dungeonItem || alignedFloorZeroCopy;
}

// ---- R59: free pack slot first, then gold via CharPay (chargePurse arithmetic).
BuyOutcome buyFromStore(Ctx& ctx, Character& buyer, Inventory& inv, const Item& stock,
                        Currency price, bool depletesShelf) {
    (void)depletesShelf;  // R59: caller depletes the shelf
    i32 free = freeSlot(inv);
    if (free < 0) {
        ctx.say("items.buyFull", "SORRY! Pack is full.", {}, {});
        return BuyOutcome::NoFreeSlot;
    }
    if (!chargePurse(buyer, price)) {
        ctx.say("items.buyGold", "Not enough gold!", {}, {});
        return BuyOutcome::NotEnoughGold;
    }
    Item bought = stock;
    bought.idLevel = IdLevel::All;  // R59: fully identified; charges carried
    inv.pack[free] = bought;
    return BuyOutcome::Bought;  // R59: no Library filing
}

// UNKNOWN: store sale formula not in the dossier. Returns 0 pending data.
Currency storeSalePrice(const Ctx& ctx, const Item& item) {
    (void)ctx;
    (void)item;
    return 0;
}

// ---- R60-R62: sale to store.
SaleOutcome sellToStore(Ctx& ctx, Character& seller, Inventory& inv, i32 slot,
                        Align storeAlign, bool payeeCanReceive) {
    SaleOutcome out;
    if (!validSlot(slot) || isEmptySlot(inv.pack[slot])) {
        out.result = SaleResult::EmptySlot;
        return out;
    }
    Item& it = inv.pack[slot];
    if (it.guildIssued || it.isGuildItem()) {  // R60
        out.result = SaleResult::GuildIssued;
        return out;
    }
    if (it.equipped && it.slotCursed) {  // R60
        out.result = SaleResult::EquippedCursed;
        return out;
    }
    if (!payeeCanReceive) {  // R62 (port)
        out.result = SaleResult::PayeeRefused;
        return out;
    }
    Currency price = storeSalePrice(static_cast<const Ctx&>(ctx), it);  // R60
    // R15.4 / Character.Receive (reference): if the receiving account would overflow,
    // the sale is refused with nothing moved. Checked before any mutation.
    if (price > 0 && seller.wallet.goldOnHand > std::numeric_limits<Currency>::max() - price) {
        out.result = SaleResult::PayeeRefused;
        return out;
    }
    if (price < 0 && seller.wallet.goldOnHand < std::numeric_limits<Currency>::min() - price) {
        out.result = SaleResult::PayeeRefused;
        return out;
    }
    bool partUsed = isPartUsed(it);  // R61
    bool shelved = (it.cursed == CurseKind::CurseNone) && storeShelvesItem(it, storeAlign) && !partUsed;
    // R62: UNKNOWN: town-income hook is not modelled; gold goes to the seller.
    // (R15.1 and R41: with no mod answer, the original pays the seller's hand.)
    seller.wallet.goldOnHand += price;
    out.gold = price;
    out.shelved = shelved;
    out.result = SaleResult::Sold;
    // CONFLICT: R61 says a part-used item is not shelved and stays in the pack. Here the
    // item stays in the pack and the gold is still paid (literal reading; UNKNOWN).
    if (!partUsed) {
        inv.pack[slot] = Item{};
    }
    return out;
}

// R61: UNKNOWN store acceptance rule. Assumed: unaligned, or matching alignment.
bool storeShelvesItem(const Item& item, Align storeAlign) {
    if (item.alignment == ItemAlign::Unaligned) return true;
    return static_cast<i32>(item.alignment) - 1 == static_cast<i32>(storeAlign);
}

// R61: part-used means charges below a whole item's charges.
bool isPartUsed(const Item& item) {
    return item.maxCharges > 0 && item.charges < item.maxCharges;
}

// R63: fee only for items not fully known.
bool needsIdentifyFee(const Item& item) {
    return item.idLevel != IdLevel::All;
}


// ---- R63: identify.
IdentifyOutcome identifyItem(Ctx& ctx, Character& payer, Item& item) {
    if (isEmptySlot(item)) return IdentifyOutcome::EmptySlot;
    if (!needsIdentifyFee(item)) return IdentifyOutcome::NotNeeded;
    Currency fee = identifyFee(static_cast<const Ctx&>(ctx), item);
    if (!chargePurse(payer, fee)) return IdentifyOutcome::CannotAfford;
    item.idLevel = IdLevel::All;
    return IdentifyOutcome::Identified;
}

// UNKNOWN: realign fee formula not in the dossier. Returns 0 pending data.
Currency realignFee(const Ctx& ctx, const Item& item) {
    (void)ctx;
    (void)item;
    return 0;
}

// ---- R66: realign an item. Equipped items refused; otherwise confirm, then charge.
RealignOutcome realignItem(Ctx& ctx, Character& payer, Inventory& inv, i32 slot,
                           ItemAlign target, bool confirmed) {
    if (!validSlot(slot) || isEmptySlot(inv.pack[slot])) return RealignOutcome::Refused;
    Item& it = inv.pack[slot];
    if (it.equipped) {  // R66
        ctx.say("items.realignEquipped", "Cannot realign Equipped item!", {}, {});
        return RealignOutcome::Refused;
    }
    if (!confirmed) {  // R66: Yes/No confirm
        ctx.say("items.realignConfirm", "Realign this item?", {}, {});
        return RealignOutcome::NeedsConfirm;
    }
    Currency fee = realignFee(static_cast<const Ctx&>(ctx), it);
    if (!chargePurse(payer, fee)) return RealignOutcome::CannotAfford;
    it.alignment = target;
    it.aligned = true;
    return RealignOutcome::Realigned;
}

// ---- R64: uncurse fee from list price. Factor and divisor are given (0.8, 0.5);
// UNKNOWN: how they combine. Rounding is half to even.
Currency uncurseFee(Currency listPrice) {
    double v = static_cast<double>(listPrice) * kUncurseFeeFactor / kUncurseFeeDivisor;
    return roundHalfEven(v);
}

// ---- R65: combine enabled once the list is nonempty.
bool combineEnabled(const Vec<i32>& slots) {
    return slots.size() > 0;
}

// R65: refusal checks. CONFLICT: pairing of messages (see file header).
CombineRefusal combineCheck(const Ctx& ctx, const Item& target, const Item& other) {
    (void)ctx;
    if (&target == &other) return CombineRefusal::SingleItem;
    if (target.type != other.type) return CombineRefusal::TypeDiffers;
    if (target.id != other.id) return CombineRefusal::ItemIdDiffers;
    if (target.alignment != other.alignment) return CombineRefusal::AlignmentTest;  // UNKNOWN text
    return CombineRefusal::NoRefusal;
}

// UNKNOWN: binding cost table not in the dossier. Returns 0 pending data.
Currency bindingCost(i32 bindingLevel) {
    (void)bindingLevel;
    return 0;
}

// ---- R67-R70: bind a companion. Cost quoted at current level, then level advances.
BindOutcome bindCompanion(Ctx& ctx, Character& payer, Companion& comp) {
    if (comp.bindingLevel < 0) {  // R67 free will refused
        ctx.say("items.freeWill", "This companion has free will.", {}, {});
        return BindOutcome::FreeWillRefused;
    }
    if (comp.bindingLevel >= kBindingMax) return BindOutcome::AlreadyBound;  // R68
    Currency cost = bindingCost(comp.bindingLevel);  // R69
    if (!chargePurse(payer, cost)) return BindOutcome::CannotAfford;
    i32 next = comp.bindingLevel + 1;  // R69: UNKNOWN step size
    comp.bindingLevel = next > kBindingMax ? kBindingMax : next;
    comp.bound = true;
    return BindOutcome::Bound;
}

// ---- R71: identify a companion. IdLevel lowered by 1 (towards Completely).
// UNKNOWN: direction of "lowered" (CONFLICT-free reading: improves knowledge).
bool identifyCompanion(Ctx& ctx, Character& payer, Companion& comp, const MonsterDef& def) {
    (void)ctx;
    Currency fee = priceToIdentify(def);
    if (!chargePurse(payer, fee)) return false;
    if (comp.idLevel != OwnerIdLevel::Completely) {
        comp.idLevel = static_cast<OwnerIdLevel>(static_cast<i32>(comp.idLevel) - 1);
    }
    return true;
}

// R72: realign only at IdLevel 0.
bool canOfferRealign(const Companion& comp) {
    return comp.idLevel == OwnerIdLevel::Completely;
}

// R72: the two alignments the companion does not have.
Arr<CompanionAlignment, 2> realignOptions(const Companion& comp) {
    Arr<CompanionAlignment, 2> out{};
    CompanionAlignment all[3] = {CompanionAlignment::Good, CompanionAlignment::Neutral,
                                 CompanionAlignment::Evil};
    i32 n = 0;
    for (i32 k = 0; k < 3; ++k) {
        if (all[k] != comp.alignment && n < 2) {
            out[n] = all[k];
            ++n;
        }
    }
    return out;
}

// ---- R72: realign a companion.
bool realignCompanion(Ctx& ctx, Character& payer, Companion& comp,
                      CompanionAlignment to, const MonsterDef& def) {
    (void)ctx;
    if (!canOfferRealign(comp)) return false;
    Arr<CompanionAlignment, 2> opts = realignOptions(comp);
    bool offered = (opts[0] == to) || (opts[1] == to);
    if (!offered) return false;
    Currency cost = priceToRealign(def, comp.alignment, to);
    if (!chargePurse(payer, cost)) return false;
    comp.alignment = to;
    return true;
}

// ---- R73: sell a companion. Hits == 0 refused.
// UNKNOWN: slot release after sale. Companion is marked unoccupied.
bool sellCompanion(Ctx& ctx, Character& payer, Companion& comp, const MonsterDef& def) {
    if (comp.hits == 0) return false;
    Currency price = priceToSell(def, comp.alignment, comp.idLevel);
    payer.wallet.goldOnHand += price;
    comp.occupied = false;
    comp.owner = -1;
    ctx.say("items.sellCompanion", "Companion sold.", {price}, {comp.name});
    return true;
}

// UNKNOWN: identify price table not in the dossier. Returns 0 pending data.
Currency priceToIdentify(const MonsterDef& def) {
    (void)def;
    return 0;
}

// UNKNOWN: realign price table not in the dossier. Returns 0 pending data.
Currency priceToRealign(const MonsterDef& def, CompanionAlignment from, CompanionAlignment to) {
    (void)def;
    (void)from;
    (void)to;
    return 0;
}

// UNKNOWN: sell price table not in the dossier. Returns 0 pending data.
Currency priceToSell(const MonsterDef& def, CompanionAlignment align, OwnerIdLevel idLevel) {
    (void)def;
    (void)align;
    (void)idLevel;
    return 0;
}

// ---- R74: Chars(0) at slots 0-39, Chars(1) at slots 40-79.
i32 exchangeSlotBase(i32 side) {
    return side * kPackSlots;
}

// ---- R75-R77: exchange window swap.
ExchangeOutcome exchangeSwap(Ctx& ctx, Inventory& from, i32 fromSlot,
                             Inventory& to, i32 toSlot) {
    if (&from == &to && fromSlot == toSlot) return ExchangeOutcome::SameSlot;  // R75
    if (fromSlot < 0 || fromSlot > kLastPackIndex || toSlot < 0 || toSlot > kLastPackIndex) {
        ctx.say("items.exchangeSorry", "Sorry!", {}, {});  // R76
        return ExchangeOutcome::OutOfRange;
    }
    if (fromSlot == 0 || toSlot == 0) return ExchangeOutcome::HandsSilent;  // R76 silent
    Item tmp = from.pack[fromSlot];
    from.pack[fromSlot] = to.pack[toSlot];
    to.pack[toSlot] = tmp;
    return ExchangeOutcome::Swapped;  // R77
}

// ---- R80-R83: gold charge. Purse is hand plus bank.
bool chargePurse(Character& c, Currency amount) {
    if (c.purse() < amount) return false;  // R81: nothing taken
    if (c.wallet.goldOnHand >= amount) {  // R82
        c.wallet.goldOnHand -= amount;
        return true;
    }
    Currency rest = amount - c.wallet.goldOnHand;  // R83
    c.wallet.goldOnHand = 0;
    c.wallet.goldInBank -= rest;
    return true;
}

// UNKNOWN: wear-cap rule not in the dossier. Assumed: worn count below cap.
bool wearCapAllows(i32 cap, i32 wornOfClass) {
    return wornOfClass < cap;
}

// UNKNOWN: record index rule not in the dossier. Assumed: the item's spell index.
i32 castSpellRecordIndex(const Item& item) {
    return item.spellIndex;
}

}  // namespace mordor

namespace mordor {
i32 exp_freeSlot_items2b_2(const Inventory& inv) { return freeSlot(inv); }
}
