#include "mordor/features.h"

#include <string>

#include "mordor/access.h"
#include "mordor/state_common.h"

namespace mordor {
namespace {

// Reference: GiveCommand.cs and fn1521 case 71 (MAINMOD2.BAS:3705-3730).
// Commands:  give gold <from> <to> <amount>
//            give item <from> <slot> <to>
// ASSUMPTION: words[0] is the verb "give" and words[1] is "gold" or "item".
// ASSUMPTION: the dispatcher passes the full word list (words[0] = "give") to the registered function.
// ASSUMPTION: Currency is i64 in 1/10000 gold.
// ASSUMPTION: Item has id (<= 0 means empty), equipped and guildIssued (truthy when set), and is
//             default-constructible; an emptied pack slot is Item().

constexpr const char* kGiveNotEnoughGold = "Not enough gold!";                 // GiveCommand.NotEnoughGold
constexpr const char* kGiveNoItem = "No Item to give!";                       // GiveCommand.NoItem
constexpr const char* kGiveItemEquipped = "Item is Equipped!";                // GiveCommand.ItemEquipped
constexpr const char* kGiveGuildIssued = "Cannot give Guild-Issued Items!";   // Party.cs GiveItem
constexpr const char* kGiveUsage =
    "give gold <from> <to> <amount>  |  give item <from> <slot> <to>";
constexpr const char* kGiveNoMember = "No such party member.";                // UNKNOWN: port wording
constexpr i64 kGiveGoldUnit = 10000;  // currency is i64 in 1/10000 gold

enum class GiveGoldStep : i32 { Nothing = 0, Refused, Give };

bool giveIsBlank(char c) { return c == ' ' || c == '\t'; }

// VB Val: leading blanks, optional sign, digits and one point; stops at the first thing it cannot use.
// UNKNOWN: exponent notation is not read here.
double giveVal(const Str& t) {
  size_t i = 0;
  while (i < t.size() && giveIsBlank(t[i])) ++i;
  double sign = 1.0;
  if (i < t.size() && (t[i] == '+' || t[i] == '-')) {
    if (t[i] == '-') sign = -1.0;
    ++i;
  }
  double v = 0.0;
  double scale = 1.0;
  bool point = false;
  for (; i < t.size(); ++i) {
    char c = t[i];
    if (c >= '0' && c <= '9') {
      if (!point) {
        v = v * 10.0 + (c - '0');
      } else {
        scale /= 10.0;
        v += (c - '0') * scale;
      }
    } else if (c == '.' && !point) {
      point = true;
    } else {
      break;
    }
  }
  return sign * v;
}

// VB CLng of the whole string: blanks at either end, optional sign, digits with at most one point.
// Rounds half to even. Anything else, thousands separators included, is a Type mismatch: false.
// PORT DEVIATION: the original's CLng reads thousands separators from the Control Panel; this port refuses them.
bool giveCLng(const Str& t, i64& out) {
  size_t b = 0;
  size_t e = t.size();
  while (b < e && giveIsBlank(t[b])) ++b;
  while (e > b && giveIsBlank(t[e - 1])) --e;
  if (b == e) return false;
  bool neg = false;
  if (t[b] == '+' || t[b] == '-') {
    neg = (t[b] == '-');
    ++b;
  }
  i64 whole = 0;
  bool sawDigit = false;
  bool point = false;
  int firstFrac = -1;
  bool restNonzero = false;
  int fracIdx = 0;
  for (size_t i = b; i < e; ++i) {
    char c = t[i];
    if (c >= '0' && c <= '9') {
      sawDigit = true;
      if (!point) {
        if (whole > 100000000000LL) return false;
        whole = whole * 10 + (c - '0');
      } else {
        if (fracIdx == 0) firstFrac = c - '0';
        else if (c != '0') restNonzero = true;
        ++fracIdx;
      }
    } else if (c == '.' && !point) {
      point = true;
    } else {
      return false;
    }
  }
  if (!sawDigit) return false;
  bool up = firstFrac > 5 || (firstFrac == 5 && restNonzero) ||
            (firstFrac == 5 && !restNonzero && (whole % 2 == 1));
  if (up) ++whole;
  out = neg ? -whole : whole;
  return true;
}

// Member numbers are 1..kPartyMaxSize; accMember then checks them against the party size.
bool giveMember(const Str& text, i32& member) {
  i64 n = 0;
  if (!giveCLng(text, n)) return false;
  if (n < 1 || n > kPartyMaxSize) return false;
  member = static_cast<i32>(n);
  return true;
}

// Reference GiveCommand.GoldAnswer: "" is 0; Val below 1 is 0; Val above what is held is refused;
// otherwise the amount is CLng of the text. A text that is not a number throughout hands over nothing.
GiveGoldStep giveGoldAmount(const Str& text, i64 heldUnits, i64& amountUnits, Str& refusal) {
  if (text.empty()) return GiveGoldStep::Nothing;
  double v = giveVal(text);
  if (v < 1.0) return GiveGoldStep::Nothing;
  double held = static_cast<double>(heldUnits) / static_cast<double>(kGiveGoldUnit);
  if (v > held) {
    refusal = kGiveNotEnoughGold;
    return GiveGoldStep::Refused;
  }
  i64 gold = 0;
  if (!giveCLng(text, gold)) return GiveGoldStep::Nothing;
  if (gold < 1) return GiveGoldStep::Nothing;
  i64 units = gold * kGiveGoldUnit;
  // UNKNOWN: CLng can round up past what is held (1.5 held, "1.5" typed -> 2). Refused, as for Val.
  if (units > heldUnits) {
    refusal = kGiveNotEnoughGold;
    return GiveGoldStep::Refused;
  }
  amountUnits = units;
  return GiveGoldStep::Give;
}

// Reference fn131A: a pack slot 2..kPackSlots holds an item; slot 1 is the Hands and never an item.
// Returns the player slot, or -1 for "No Item to give!".
i32 giveSlot(const Str& text) {
  i64 n = 0;
  if (!giveCLng(text, n)) return -1;
  if (n < 2 || n > kPackSlots) return -1;
  return static_cast<i32>(n);
}

// First empty pack slot of a member (slots 2..kPackSlots; slot 1 is the Hands). nullptr when the pack is full.
// UNKNOWN: the reference's placement rule for a received item (Party.GiveItem) is not in the packet; first free slot used.
Item* giveFreeSlot(Session& s, i32 member1) {
  for (i32 slot = 2; slot <= kPackSlots; ++slot) {
    Item* it = accPackItem(s, member1, slot);
    if (it != nullptr && it->id <= 0) return it;
  }
  return nullptr;
}

bool giveGold(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() != 5) {
    reply = kGiveUsage;
    return true;
  }
  i32 from = 0;
  i32 to = 0;
  if (!giveMember(w[2], from) || !giveMember(w[3], to)) {
    reply = kGiveUsage;
    return true;
  }
  Character* giver = accMember(s, from);
  Character* receiver = accMember(s, to);
  if (giver == nullptr || receiver == nullptr) {
    reply = kGiveNoMember;
    return true;
  }
  if (from == to) {
    // UNKNOWN: the reference offers everyone but the giver; the port's refusal wording is its own.
    reply = "Cannot give to yourself.";
    return true;
  }
  Currency& held = accGoldOnHand(*giver);
  i64 units = 0;
  Str refusal;
  switch (giveGoldAmount(w[4], held, units, refusal)) {
    case GiveGoldStep::Refused:
      reply = refusal;
      return true;
    case GiveGoldStep::Nothing:
      reply = "Nothing given.";  // UNKNOWN: the reference prints nothing here
      return true;
    case GiveGoldStep::Give:
      break;
  }
  held -= units;
  accGoldOnHand(*receiver) += units;
  // UNKNOWN: the reference's success line for gold is not in the packet.
  reply = "Gave " + std::to_string(units / kGiveGoldUnit) + " gold.";
  return true;
}

bool giveItem(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() != 5) {
    reply = kGiveUsage;
    return true;
  }
  i32 from = 0;
  i32 to = 0;
  if (!giveMember(w[2], from) || !giveMember(w[4], to)) {
    reply = kGiveUsage;
    return true;
  }
  Character* giver = accMember(s, from);
  Character* receiver = accMember(s, to);
  if (giver == nullptr || receiver == nullptr) {
    reply = kGiveNoMember;
    return true;
  }
  if (from == to) {
    reply = "Cannot give to yourself.";  // UNKNOWN, as for gold
    return true;
  }
  // The reference treats "" and "0" as no item: nothing is handed over.
  if (w[3].empty() || w[3] == "0") {
    reply = "Nothing given.";  // UNKNOWN
    return true;
  }
  const i32 slot = giveSlot(w[3]);
  if (slot < 0) {
    reply = kGiveNoItem;
    return true;
  }
  Item* src = accPackItem(s, from, slot);
  if (src == nullptr || src->id <= 0) {
    reply = kGiveNoItem;
    return true;
  }
  if (src->equipped) {
    reply = kGiveItemEquipped;
    return true;
  }
  if (src->guildIssued) {
    reply = kGiveGuildIssued;
    return true;
  }
  // The receiver's room is checked before anything leaves the giver.
  Item* dst = giveFreeSlot(s, to);
  if (dst == nullptr) {
    reply = "\"" + receiver->name + "\" has no free slots!";  // UNKNOWN: port wording
    return true;
  }
  *dst = *src;
  *src = Item();
  reply = "Gave item " + w[3] + ".";  // UNKNOWN: success line not in packet
  return true;
}

bool cmdGive(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = kGiveUsage;
    return true;
  }
  if (w[1] == "gold") return giveGold(s, w, reply);
  if (w[1] == "item") return giveItem(s, w, reply);
  reply = kGiveUsage;
  return true;
}

}  // namespace

// Runs `give gold ...` or `give item ...` on the session. words[0] is "give". Returns true when a reply was produced.
bool giveRun(Session& s, const Vec<Str>& words, Str& reply) { return cmdGive(s, words, reply); }

void register_give() {
  registerFeatureCommand({"give", Screen::Town, &cmdGive, "give gold <from> <to> <amount> | give item <from> <slot> <to>  - hand gold or a pack item to another member"});
  registerFeatureCommand({"give", Screen::Dungeon, &cmdGive, "give gold <from> <to> <amount> | give item <from> <slot> <to>  - hand gold or a pack item to another member"});
}

}  // namespace mordor
