#include "mordor/features.h"

#include "mordor/access.h"
#include "mordor/store_prices.h"   // storeRealignFee: 0.72 x price, doubled across more than one step (reference RealignCost)
#include "mordor/math_util.h"
#include "mordor/state_common.h"

namespace mordor {
namespace {

// Item alignment codes in the reference's ItemAlignments.All order (U, G, N, E).
// UNKNOWN: the ItemAlign enumerator names are not in the evidence; the codes are cast directly.
constexpr i32 kRealignUnaligned = 0;
constexpr i32 kRealignGood = 1;
constexpr i32 kRealignNeutral = 2;
constexpr i32 kRealignEvil = 3;

// Maps the player's word to an item alignment code; -1 means not recognised.
i32 realignAlignCode(const Str& word) {
  const Str w = accLower(word);
  if (w == "good") return kRealignGood;
  if (w == "neutral") return kRealignNeutral;
  if (w == "evil") return kRealignEvil;
  return -1;
}

// Plain decimal parse without exceptions. Returns false on empty or non-digit input.
bool realignParseNumber(const Str& word, i32& out) {
  i64 v = 0;
  if (!accParseInt(word, v) || v < 0 || v > 1000000) return false;
  out = static_cast<i32>(v);
  return true;
}

// The pack item at (member, slot), or nullptr when the slot is empty or out of range.
// Slot 1 is the hands and never an item; accPackItem's range check covers the rest.
Item* realignItemAt(Session& s, i32 member1, i32 slot1) {
  if (slot1 < 2) return nullptr;
  Item* it = accPackItem(s, member1, slot1);
  if (it == nullptr || it->id <= 0) return nullptr;
  return it;
}

// Base Price of the item (R60 input). The ItemDef price field is not named in the evidence.
// NEED: the ItemDef field that holds the base Price (the data file's Price column).
// Until it is named, the base price reads as 0 and the fee is 0.
// Takes the fee from the party's gold on hand, drawing from members in roster order.
// Returns false, with nothing taken, when the party cannot pay.
// UNKNOWN: the SRD does not say which member pays; the party pool is used.
bool realignPayFee(Session& s, Currency fee) {
  const i32 n = accPartySize(s);
  Currency total = 0;
  for (i32 m = 1; m <= n; ++m) total += accGoldOnHand(*accMember(s, m));
  if (total < fee) return false;
  Currency left = fee;
  for (i32 m = 1; m <= n && left > 0; ++m) {
    Currency& gold = accGoldOnHand(*accMember(s, m));
    const Currency take = gold < left ? gold : left;
    gold -= take;
    left -= take;
  }
  return true;
}

// realign <member> <slot> <good|neutral|evil>
bool cmdRealign(Session& s, const Vec<Str>& w, Str& reply) {
  // UNKNOWN: assumes w[0] is the verb and the arguments follow.
  if (w.size() != 4) {
    reply = "Usage: realign <member> <slot> <good|neutral|evil>";
    return true;
  }

  i32 member1 = 0;
  if (!realignParseNumber(w[1], member1) || member1 < 1 || member1 > accPartySize(s)) {
    reply = "No such member.";
    return true;
  }

  // Pack slots 2..kPackSlots hold items; slot 1 is the hands.
  i32 slot1 = 0;
  if (!realignParseNumber(w[2], slot1) || slot1 < 2 || slot1 > kPackSlots) {
    reply = "No such slot.";
    return true;
  }

  const i32 target = realignAlignCode(w[3]);
  if (target < 0) {
    reply = "Realign to good, neutral or evil.";
    return true;
  }

  Item* it = realignItemAt(s, member1, slot1);
  if (it == nullptr) {
    reply = "Nothing in that slot.";
    return true;
  }

  // R59: an equipped item cannot be realigned.
  if (it->equipped) {
    reply = "Cannot realign Equipped item!";
    return true;
  }

  const i32 current = static_cast<i32>(it->alignment);
  // UNKNOWN: the SRD does not say what happens when the target equals the current
  // alignment. Refused as a no-op.
  if (current == target) {
    reply = "That item is already that alignment.";
    return true;
  }

  // R60: round_half_even(0.72 x Price x mult); storeRealignFee owns the multiplier
  // (2 when the step is more than one place away, else 1).
  const i32 steps = current > target ? current - target : target - current;
  const Currency fee = storeRealignFee(s.data, *it, steps);

  // UNKNOWN: the SRD's R66 shows a Yes/No confirm before the change. A single command
  // cannot ask, so the change is applied directly once the fee is paid.
  if (!realignPayFee(s, fee)) {
    reply = "Not enough gold for the realign fee.";
    return true;
  }

  // The pointer stays valid: no pack or roster resize happens between lookup and here.
  it->alignment = static_cast<decltype(it->alignment)>(target);

  Ctx ctx = s.ctx();
  ctx.say("realign.done", "The item has been realigned.",
          {static_cast<i64>(member1), static_cast<i64>(slot1), static_cast<i64>(fee)}, {});
  reply = "The item has been realigned.";
  return true;
}

}  // namespace

void register_realign() {
  registerFeatureCommand({"realign", Screen::Town, &cmdRealign,
                          "realign <member> <slot> <good|neutral|evil>  - change an item's alignment for a fee"});
}

}  // namespace mordor
