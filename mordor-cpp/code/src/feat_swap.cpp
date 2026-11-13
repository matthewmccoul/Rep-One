#include "mordor/features.h"

#include <string>
#include <utility>

#include "mordor/access.h"
#include "mordor/state_common.h"
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {


// UNKNOWN: party member numbering is not in the evidence packet. Assumed 1-based, 1..kPartyMaxSize.
bool swapMemberInRange(i32 member) { return member >= 1 && member <= kPartyMaxSize; }

// int.TryParse analogue (reference SwapCommand.Parse): optional sign, digits, surrounding spaces ok.
// The original's five-character box limit does not apply: the words arrive already split.
bool swapParseInt(const Str& text, i32& out) {
  size_t i = 0;
  size_t n = text.size();
  while (i < n && text[i] == ' ') ++i;
  while (n > i && text[n - 1] == ' ') --n;
  if (i >= n) return false;
  bool neg = false;
  if (text[i] == '+' || text[i] == '-') {
    neg = (text[i] == '-');
    ++i;
  }
  if (i >= n) return false;
  i64 v = 0;
  for (; i < n; ++i) {
    const char c = text[i];
    if (c < '0' || c > '9') return false;
    v = v * 10 + (c - '0');
    if (v > 1000000000LL) return false;
  }
  out = static_cast<i32>(neg ? -v : v);
  return true;
}

// VB Integer range for Pack.Charges (FRMCTYS.BAS:677, reference Combining.Combine).
constexpr i64 kSwapChargeMax = 32767;
constexpr i64 kSwapChargeMin = -32768;

// Combinable kinds (FRMCTYS.BAS:663): Select Case Kind 23..28, 30, 31.
bool swapCombinableKind(i32 kind) {
  return kind == 23 || kind == 24 || kind == 25 || kind == 26 ||
         kind == 27 || kind == 28 || kind == 30 || kind == 31;
}

// NEED: ItemDef kind field name (reference Item.Type, looked up through accItemDef).
// Until the field is named, the kind is reported as unknown and combine refuses with the
// class message. Everything else in combine is implemented.
bool swapItemKind(const Session& s, const Item& it, i32& kind) {
  const ItemDef* def = accItemDef(s, it.id);
  if (def == nullptr) return false;
  kind = 0;
  return false;
}

enum class SwapCombineCheck : i32 {
  Ok = 0,
  NotIdentifiedFirst,
  NotIdentifiedSecond,
  ClassCannotCombine,
  NotCompatibleClass,
  NotSameType,
  NotCompatibleAlignment,
  Overflow
};

// FRMCTYS.BAS:647-694 check order. Identification is checked for both items first
// (FRMCTYS.BAS:647-653). The first item's kind must be combinable (FRMCTYS.BAS:662-668).
// Then: class, then id, then alignment, then the charge sum (FRMCTYS.BAS:669-694).
// CONFLICT: the reference swaps two messages (type mismatch reads "not compatible class",
// id mismatch reads "not of the same type"). The check order is kept; the text follows the
// reference wording in swapCombineText.
SwapCombineCheck swapCombineCheck(bool firstKnown, bool secondKnown,
                                  bool firstCombinableKind, bool sameClass,
                                  bool sameId, bool sameAlignment,
                                  i64 chargesFirst, i64 chargesSecond) {
  if (!firstKnown) return SwapCombineCheck::NotIdentifiedFirst;
  if (!secondKnown) return SwapCombineCheck::NotIdentifiedSecond;
  if (!firstCombinableKind) return SwapCombineCheck::ClassCannotCombine;
  if (!sameClass) return SwapCombineCheck::NotCompatibleClass;
  if (!sameId) return SwapCombineCheck::NotSameType;
  if (!sameAlignment) return SwapCombineCheck::NotCompatibleAlignment;
  // CONFLICT: the original does not check this sum and stops with VB run-time error 6
  // ("Overflow") past 32,767. This port refuses instead and changes nothing.
  const i64 sum = chargesFirst + chargesSecond;
  if (sum > kSwapChargeMax || sum < kSwapChargeMin) return SwapCombineCheck::Overflow;
  return SwapCombineCheck::Ok;
}

Str swapCombineText(SwapCombineCheck c, i32 slotFirst, i32 slotSecond) {
  const Str a = std::to_string(slotFirst);
  const Str b = std::to_string(slotSecond);
  switch (c) {
    case SwapCombineCheck::NotIdentifiedFirst:
      return "Item #" + a + " Not Completely Identified!";
    case SwapCombineCheck::NotIdentifiedSecond:
      return "Item #" + b + " Not Completely Identified!";
    case SwapCombineCheck::ClassCannotCombine:
      return "Item Class cannot be Combined!";
    case SwapCombineCheck::NotCompatibleClass:
      return "STORE: Item #" + b + " is not compatible class.";
    case SwapCombineCheck::NotSameType:
      return "STORE: Item #" + b + " is not of the same type.";
    case SwapCombineCheck::NotCompatibleAlignment:
      return "STORE: Item #" + b + " is not compatible Alignment.";
    case SwapCombineCheck::Overflow:
      return "STORE: Item #" + b + " not combined: the charges would pass 32767. Nothing changed.";
    case SwapCombineCheck::Ok:
      return "STORE: Item #" + b + " combined to Item #" + a + ".";
  }
  return "";
}

// Exchanges the contents of pack slots slotA and slotB for party member `member`, empty slots
// included (R45). Spell buffers are NOT remapped (original quirk): they keep naming the same
// slot numbers, so nothing else is touched.
bool swapApply(Session& s, i32 member, i32 slotA, i32 slotB, Str& reply) {
  if (accMember(s, member) == nullptr) {
    reply = "No such party member.";
    return false;
  }
  Item* a = accPackItem(s, member, slotA);
  Item* b = accPackItem(s, member, slotB);
  if (a == nullptr || b == nullptr) {
    reply = "Slots must be 2 to 40 (slot 1 is your hands and cannot be moved).";
    return false;
  }
  std::swap(*a, *b);
  reply = "Item swap completed!";
  return true;
}

// Pours the charges of slotB into slotA for party member `member` (FRMCTYS.BAS:616-697).
// Combine is a store action (Town only, see register_swap). On any refusal nothing changes.
bool combineApply(Session& s, i32 member, i32 slotA, i32 slotB, Str& reply) {
  if (accMember(s, member) == nullptr) {
    reply = "No such party member.";
    return false;
  }
  Item* first = accPackItem(s, member, slotA);
  Item* second = accPackItem(s, member, slotB);
  if (first == nullptr || second == nullptr) {
    reply = "Slots must be 2 to 40 (slot 1 is your hands and cannot be combined).";
    return false;
  }
  // UNKNOWN: an empty slot is not an item; the reference has no message for it. Refused here.
  if (first->id <= 0) {
    reply = "Item #" + std::to_string(slotA) + " is empty.";
    return false;
  }
  if (second->id <= 0) {
    reply = "Item #" + std::to_string(slotB) + " is empty.";
    return false;
  }

  const bool firstKnown =
      static_cast<i32>(first->idLevel) == static_cast<i32>(IdLevel::All);
  const bool secondKnown =
      static_cast<i32>(second->idLevel) == static_cast<i32>(IdLevel::All);

  i32 kindFirst = 0;
  i32 kindSecond = 0;
  const bool firstKindKnown = swapItemKind(s, *first, kindFirst);
  const bool secondKindKnown = swapItemKind(s, *second, kindSecond);
  const bool firstCombinable = firstKindKnown && swapCombinableKind(kindFirst);
  const bool sameClass = firstKindKnown && secondKindKnown && kindFirst == kindSecond;

  const SwapCombineCheck check = swapCombineCheck(
      firstKnown, secondKnown, firstCombinable, sameClass,
      first->id == second->id,
      first->alignment == second->alignment,
      static_cast<i64>(first->charges), static_cast<i64>(second->charges));
  reply = swapCombineText(check, slotA, slotB);
  if (check != SwapCombineCheck::Ok) return false;

  // UNKNOWN: an equipped copy is refused by the earlier port; the reference does not show it.
  // Kept as a conservative guard so an equipped item is never emptied by a combine.
  if (first->equipped || second->equipped) {
    reply = "Item #" + std::to_string(slotB) + " is equipped and cannot be combined.";
    return false;
  }

  const i64 sum = static_cast<i64>(first->charges) + static_cast<i64>(second->charges);
  first->charges = static_cast<decltype(first->charges)>(sum);
  *second = Item{};  // the poured-from slot is emptied
  return true;
}

// UNKNOWN: words[0] is assumed to be the verb; arguments are words[1..3].
bool cmdSwap(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() != 4) {
    reply = "Usage: swap <member> <slot1> <slot2>";
    return false;
  }
  i32 member = 0;
  i32 a = 0;
  i32 b = 0;
  if (!swapParseInt(w[1], member) || !swapMemberInRange(member)) {
    reply = "No such party member.";
    return false;
  }
  if (!swapParseInt(w[2], a) || !swapParseInt(w[3], b)) {
    reply = "Usage: swap <member> <slot1> <slot2>";
    return false;
  }
  // R45
  if (!swapSlotInRange(a) || !swapSlotInRange(b)) {
    reply = "Slots must be 2 to 40 (slot 1 is your hands and cannot be moved).";
    return false;
  }
  return swapApply(s, member, a, b, reply);
}

bool cmdCombine(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() != 4) {
    reply = "Usage: combine <member> <slot1> <slot2>";
    return false;
  }
  i32 member = 0;
  i32 a = 0;
  i32 b = 0;
  if (!swapParseInt(w[1], member) || !swapMemberInRange(member)) {
    reply = "No such party member.";
    return false;
  }
  if (!swapParseInt(w[2], a) || !swapParseInt(w[3], b)) {
    reply = "Usage: combine <member> <slot1> <slot2>";
    return false;
  }
  // The original silently does nothing for a repeated slot (FRMCTYS.BAS:654-657).
  // Here the same slot is refused with a reason.
  if (a == b) {
    reply = "Those are the same slot; nothing to combine.";
    return false;
  }
  if (!swapSlotInRange(a) || !swapSlotInRange(b)) {
    reply = "Slots must be 2 to 40 (slot 1 is your hands and cannot be combined).";
    return false;
  }
  return combineApply(s, member, a, b, reply);
}

}  // namespace

void register_swap() {
  registerFeatureCommand({"swap", Screen::Town, &cmdSwap,
                          "swap <member> <slot1> <slot2>  - swap two pack slots, empty ones included (slot 1 is your hands and cannot move)"});
  registerFeatureCommand({"swap", Screen::Dungeon, &cmdSwap,
                          "swap <member> <slot1> <slot2>  - swap two pack slots, empty ones included (slot 1 is your hands and cannot move)"});
  registerFeatureCommand({"combine", Screen::Town, &cmdCombine,
                          "combine <member> <slot1> <slot2>  - pour the charges of one identified item into another of the same kind"});
}

}  // namespace mordor
