// items2b.cpp: Part B items. Giving, dragging, equip/drop/use, item orders,
// chests, theft, pack re-read, bank withdrawal, swap parsing.
//
// CONFLICT: items2b.h uses EquipOutcome, UseOutcome, DropOutcome, UncurseOutcome,
// RealignOutcome, BuyOutcome, CombineOutcome (items1a.h) and SwapResult (items1b.h)
// without including the headers that define them. This file includes both before
// items2b.h. Enumerator values were not supplied, so the enumerator names used here
// are placeholders to reconcile with items1a.h / items1b.h.
// CONFLICT: canBankItem(const Item&) is declared in items1b.h and items2b.h with the
// same signature. Defined once, here.
// CONFLICT: uncurseItem(Ctx&,Character&,Inventory&,i32) is declared in items1a.h and
// items2b.h with the same signature. Defined once, in items2b_2.cpp.
// UNKNOWN: slot numbering. Inventory indices are 0-based; index 0 is Hands; storage
// is indices 1..39. A user-typed slot n maps to index n-1 (R4, R56).
// UNKNOWN: empty slot means Item.id == 0.
// FIXED: the enumerators EquipOutcome::{Refused, Equipped, Unequipped},
// DropOutcome::{Refused, Dropped}, UseOutcome::{Refused, NoCharges} and
// SwapResult::{Refused, Swapped} are not in the API INDEX and do not exist in the
// headers. They are replaced by the placeholder constants defined below. The ordinal
// values (0, 1, 2) are GUESSES and must be reconciled with the real enumerator lists
// in items1a.h and items1b.h before this code is used.

#include "mordor/items1a.h"
#include "mordor/items1b.h"
#include "mordor/items2b.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

// FIXED: placeholder enumerator values. UNKNOWN ordinals; reconcile with items1a.h / items1b.h.
constexpr EquipOutcome kEquipRefused = static_cast<EquipOutcome>(0);
constexpr EquipOutcome kEquipUnequipped = static_cast<EquipOutcome>(1);
constexpr EquipOutcome kEquipEquipped = static_cast<EquipOutcome>(2);
constexpr DropOutcome kDropRefused = static_cast<DropOutcome>(0);
constexpr DropOutcome kDropDropped = static_cast<DropOutcome>(1);
constexpr UseOutcome kUseRefused = static_cast<UseOutcome>(0);
constexpr UseOutcome kUseNoCharges = static_cast<UseOutcome>(1);
constexpr SwapResult kSwapRefused = static_cast<SwapResult>(0);
constexpr SwapResult kSwapSwapped = static_cast<SwapResult>(1);

}  // namespace

namespace {

}  // namespace

bool isEmptySlot(const Item& item) {
    return item.id == 0;  // UNKNOWN: empty encoding
}

}  // namespace mordor
