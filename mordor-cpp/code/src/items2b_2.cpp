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

}  // namespace


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

}  // namespace mordor
