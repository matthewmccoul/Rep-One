#include "mordor/features.h"
#include "mordor/access.h"
#include "mordor/math_util.h"
#include "mordor/shared_helpers.h"
#include "mordor/items2b.h"
#include "mordor/parse.h"

namespace mordor {
namespace {

// R61 / ch13 Uncurse. Refusals are checked in reference order: the slot's curse flag, then the
// equipped bit (Uncursing.Refusal). Texts are copied from the reference.
const char* kUncurseNotCursed = "Item is not cursed!";
const char* kUncurseNotEquipped = "Item isn't equipped!";
const char* kUncurseUsage = "Usage: uncurse <member> <slot>";
const char* kUncurseNoSlot = "No such member or item slot.";
// UNKNOWN: the reference has no text for a fee the payer cannot cover.
const char* kUncurseNoFunds = "You cannot afford the uncurse fee.";
// UNKNOWN: the reference has no text for an item whose base price is not in the tables.
const char* kUncurseNoPrice = "That item cannot be uncursed.";
// UNKNOWN: the reference has no text for a successful uncurse; this is the message used here.
const char* kUncurseDone = "The curse is lifted.";

// Currency is gold x 10000.
constexpr i64 kCurrencyPerGold = 10000;


// Returns the clerk's refusal, or nullptr when the uncurse goes ahead (R61).
const char* uncurseRefusal(bool slotCursed, bool equipped) {
  if (!slotCursed) return kUncurseNotCursed;
  if (!equipped) return kUncurseNotEquipped;
  return nullptr;
}

// Non-negative decimal integer only. Returns false on anything else. No exceptions.
bool parseIndex(const Str& text, i32& out) {
  i64 v = 0;
  if (!parseDecimal(text, v, false, 4, -1)) return false;
  out = static_cast<i32>(v);
  return true;
}

// R64 / ch13 [c52]: uncurse fee = round_half_even(0.8 * Price / 0.5), Price in whole gold
// (ItemDef.price). The fee is returned in Currency units (gold x 10000).
Currency uncurseFeeForPriceGold(i32 priceGold) {
  const double fee = 0.8 * static_cast<double>(priceGold) / 0.5;
  return static_cast<Currency>(roundHalfEven(fee)) * kCurrencyPerGold;
}

bool cmdUncurse(Session& s, const Vec<Str>& w, Str& reply) {
  // w[0] is the verb "uncurse"; the arguments are w[1] (member) and w[2] (slot).
  if (w.size() != 3) {
    reply = kUncurseUsage;
    return true;
  }
  i32 member = 0;
  i32 slot = 0;
  if (!parseIndex(w[1], member) || !parseIndex(w[2], slot)) {
    reply = kUncurseUsage;
    return true;
  }
  Character* who = accMember(s, member);
  Item* item = accPackItem(s, member, slot);
  if (who == nullptr || !swapSlotInRange(slot) || item == nullptr) {
    reply = kUncurseNoSlot;
    return true;
  }

  // R61: refusals read the slot curse flag, then the equipped bit. An empty slot has a clear flag,
  // so it gets "Item is not cursed!" as in the reference.
  const bool slotCursed = item->slotCursed;
  const bool equipped = static_cast<bool>(item->equipped);
  if (const char* refusal = uncurseRefusal(slotCursed, equipped)) {
    reply = refusal;
    return true;
  }

  // The uncurse proceeds (R61). Read the price and the fee, check funds with no writes, and only
  // then clear the flag and charge, so a refused step never costs gold.
  const ItemDef* def = accItemDef(s, item->id);
  if (def == nullptr) {
    reply = kUncurseNoPrice;
    return true;
  }
  const Currency fee = uncurseFeeForPriceGold(def->price);
  if (who->wallet.goldOnHand + who->wallet.goldInBank < fee) {
    reply = kUncurseNoFunds;
    return true;
  }
  // UNKNOWN: the reference does not say whether the item's own cursed field is cleared by an
  // uncurse; only the slot flag is cleared here (R61 wording: "the curse").
  item->slotCursed = false;
  // The funds check above guarantees the charge succeeds.
  chargePurse(*who, fee);
  reply = kUncurseDone;
  return true;
}

}  // namespace

void register_uncurse() {
  registerFeatureCommand({"uncurse", Screen::Town, &cmdUncurse, "uncurse <member> <slot>  - remove a curse from an equipped item for a fee"});
}

}  // namespace mordor
