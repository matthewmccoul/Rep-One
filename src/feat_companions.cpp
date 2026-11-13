#include "mordor/features.h"

#include <string>
#include <utility>

#include "mordor/access.h"
#include "mordor/game.h"
#include "mordor/state_common.h"

namespace mordor {
namespace {

// UNKNOWN: words[0] is the verb; arguments start at words[1].
// UNKNOWN: member and slot numbers are 1-based on screen; the access helpers are 1-based too.

// R34 / CompanionMood: a companion bound for life never drifts again.
// UNKNOWN: ForLife is 100 (c21: effect 150 sets the binding to 100 and ForLife); confirm against Binding.ForLife.
constexpr i32 kBindingForLife = 100;

Str companionsNum(i32 n) { return Str(std::to_string(n)); }

bool companionsParseNumber(const Str& text, i32& out) {
  if (text.empty() || text.size() > 2) return false;
  i32 v = 0;
  for (char ch : text) {
    if (ch < '0' || ch > '9') return false;
    v = v * 10 + (ch - '0');
  }
  out = v;
  return true;
}

// 1-based party member, 1..kPartyMaxSize.
bool companionsParseMember(const Str& text, i32& member1) {
  i32 n = 0;
  if (!companionsParseNumber(text, n) || n < 1 || n > kPartyMaxSize) return false;
  member1 = n;
  return true;
}

// 1-based companion slot, 1..kCompanionSlots.
bool companionsParseSlot(const Str& text, i32& slot1) {
  i32 n = 0;
  if (!companionsParseNumber(text, n) || n < 1 || n > kCompanionSlots) return false;
  slot1 = n;
  return true;
}

// R67 / CompanionSlots: a slot holds a companion when it is occupied and names a creature.
bool companionsFilled(const Companion& c) { return c.occupied && c.ref >= 0; }

// CompanionCarry (MAINMOD2.BAS:2027): a fallen companion is a filled slot at zero or fewer hits.
bool companionsFallen(const Companion& c) { return companionsFilled(c) && c.hits <= 0; }

// R37.1: the owner chooses a body only while carrying nothing or a companion.
// UNKNOWN: carryIndex is the 0-based companion slot, -1 for nothing, 4 and up for a morgue body.
bool companionsCarryOpen(i32 carryIndex) { return carryIndex < kCompanionSlots; }

bool companionsCarryingSlot(i32 carryIndex, i32 slot0) { return carryIndex == slot0; }

// The creature's name, read from the monster table through the slot's creature index.
// UNKNOWN: the companion record carries no name field in the types shown, so the table name is used.
Str companionsName(const Session& s, const Companion& c) {
  const MonsterDef* def = accMonster(s, c.ref);
  return def != nullptr ? def->name : Str("creature");
}

struct CompanionsTarget {
  Character* who = nullptr;
  Inventory* inv = nullptr;
  Arr<Companion, kCompanionSlots>* slots = nullptr;
};

bool companionsTarget(Session& s, i32 member1, CompanionsTarget& t) {
  t.who = accMember(s, member1);
  t.inv = accInventory(s, member1);
  t.slots = accCompanions(s, member1);
  return t.who != nullptr && t.inv != nullptr && t.slots != nullptr;
}

// Trailing spaces are trimmed from names in mood lines (CompanionMood.Round uses TrimEnd).
Str companionsTrimEnd(const Str& text) {
  size_t end = text.size();
  while (end > 0 && text[end - 1] == ' ') --end;
  return text.substr(0, end);
}

}  // namespace

// R67: release a companion. Sets the creature index to -1; hits and every other field stay in the record.
// The carry is not touched, so a carry on this slot is left pointing at the hole (CompanionCarry fault, kept).
// UNKNOWN: the release line text; the reference's wording is not in the evidence.
bool companionsReleaseSlot(Session& s, i32 member1, i32 slot1, Str& reply) {
  CompanionsTarget t;
  if (!companionsTarget(s, member1, t)) {
    reply = "no such member.";
    return true;
  }
  if (slot1 < 1 || slot1 > kCompanionSlots) {
    reply = "no such companion slot.";
    return true;
  }
  Companion& c = (*t.slots)[static_cast<size_t>(slot1 - 1)];
  if (!companionsFilled(c)) {
    reply = "companion " + companionsNum(slot1) + " is empty.";
    return true;
  }
  reply = companionsName(s, c) + " is let go.";
  c.occupied = false;
  c.ref = -1;
  return true;
}

namespace {

bool cmdCompanions(Session& s, const Vec<Str>& w, Str& reply) {
  i32 member = 0;
  if (w.size() != 2 || !companionsParseMember(w[1], member)) {
    reply = "usage: companions <member 1-4>";
    return true;
  }
  CompanionsTarget t;
  if (!companionsTarget(s, member, t)) {
    reply = "no such member.";
    return true;
  }
  Str out = t.who->name + "'s companions:";
  for (i32 i = 0; i < kCompanionSlots; ++i) {
    const Companion& c = (*t.slots)[static_cast<size_t>(i)];
    out += "\n" + companionsNum(i + 1) + ". ";
    if (!companionsFilled(c)) {
      out += "empty";
      continue;
    }
    out += companionsName(s, c) + ", hits " + companionsNum(c.hits);
    if (companionsCarryingSlot(t.inv->carryIndex, i)) out += " (carried)";
  }
  reply = out;
  return true;
}

bool cmdRearrange(Session& s, const Vec<Str>& w, Str& reply) {
  i32 member = 0, from = 0, to = 0;
  if (w.size() != 4 || !companionsParseMember(w[1], member) || !companionsParseSlot(w[2], from) ||
      !companionsParseSlot(w[3], to)) {
    reply = "usage: rearrange <member 1-4> <from slot 1-4> <to slot 1-4>";
    return true;
  }
  // Onto itself nothing happens (CompanionSlots.Rearrange, FRMCHAR3.BAS:763-779).
  if (from == to) {
    reply.clear();
    return true;
  }
  CompanionsTarget t;
  if (!companionsTarget(s, member, t)) {
    reply = "no such member.";
    return true;
  }
  const i32 from0 = from - 1;
  const i32 to0 = to - 1;
  // Swap the whole records, empty slots included: a slot keeps what its last occupant left in it.
  std::swap((*t.slots)[static_cast<size_t>(from0)], (*t.slots)[static_cast<size_t>(to0)]);
  // UNKNOWN: the carry follows the companion it holds when the two slots swap.
  const i32 carry = t.inv->carryIndex;
  if (carry == from0) {
    t.inv->carryIndex = to0;
  } else if (carry == to0) {
    t.inv->carryIndex = from0;
  }
  // UNKNOWN: a dead companion's refusal (NoRearrangingTheDead) has no wording in the evidence, so no refusal is made.
  reply.clear();
  return true;
}

bool cmdRelease(Session& s, const Vec<Str>& w, Str& reply) {
  i32 member = 0, slot = 0;
  if (w.size() != 3 || !companionsParseMember(w[1], member) || !companionsParseSlot(w[2], slot)) {
    reply = "usage: release <member 1-4> <slot 1-4>";
    return true;
  }
  return companionsReleaseSlot(s, member, slot, reply);
}

bool cmdCarry(Session& s, const Vec<Str>& w, Str& reply) {
  i32 member = 0, slot = 0;
  if (w.size() != 3 || !companionsParseMember(w[1], member) || !companionsParseSlot(w[2], slot)) {
    reply = "usage: carry <member 1-4> <slot 1-4>";
    return true;
  }
  CompanionsTarget t;
  if (!companionsTarget(s, member, t)) {
    reply = "no such member.";
    return true;
  }
  const i32 slot0 = slot - 1;
  const Companion& c = (*t.slots)[static_cast<size_t>(slot0)];
  if (!companionsFilled(c)) {
    reply = "companion " + companionsNum(slot) + " is empty.";
    return true;
  }
  // UNKNOWN: refusal wording for carrying a morgue body is not in the evidence.
  if (!companionsCarryOpen(t.inv->carryIndex)) {
    reply = t.who->name + " is carrying a body.";
    return true;
  }
  // UNKNOWN: refusal wording for a companion already carried is not in the evidence.
  if (companionsCarryingSlot(t.inv->carryIndex, slot0)) {
    reply = companionsName(s, c) + " is already being carried.";
    return true;
  }
  // UNKNOWN: refusal wording for a companion that is not fallen is not in the evidence.
  if (!companionsFallen(c)) {
    reply = companionsName(s, c) + " is not fallen.";
    return true;
  }
  // UNKNOWN: the size gate (monster size less owner race size at most one, else "too big to carry") is not run:
  // neither size field is in the types shown, so every fallen companion is picked up.
  const Str name = companionsName(s, c);
  Str out;
  const i32 old = t.inv->carryIndex;
  if (old >= 0 && old < kCompanionSlots) {
    // CompanionCarry: the "dropped" line names the slot being replaced (MAINMOD2.BAS:2027-2084).
    // UNKNOWN: wording of the dropped line.
    out = "\"" + companionsTrimEnd(t.who->name) + "\" dropped companion " + companionsNum(old + 1) +
          " to carry " + name + ".";
    out += "\n";
  }
  t.inv->carryIndex = slot0;
  out += "You are now carrying " + name + ".";
  reply = out;
  return true;
}

}  // namespace

void register_companions() {
  registerFeatureCommand({"companions", Screen::Town, &cmdCompanions, "companions <member>  - list a member's companions"});
  registerFeatureCommand({"companions", Screen::Dungeon, &cmdCompanions, "companions <member>  - list a member's companions"});
  registerFeatureCommand({"rearrange", Screen::Town, &cmdRearrange, "rearrange <member> <from> <to>  - swap companion slots"});
  registerFeatureCommand({"rearrange", Screen::Dungeon, &cmdRearrange, "rearrange <member> <from> <to>  - swap companion slots"});
  registerFeatureCommand({"release", Screen::Town, &cmdRelease, "release <member> <slot>  - let a companion go"});
  registerFeatureCommand({"release", Screen::Dungeon, &cmdRelease, "release <member> <slot>  - let a companion go"});
  registerFeatureCommand({"carry", Screen::Town, &cmdCarry, "carry <member> <slot>  - carry a fallen companion"});
  registerFeatureCommand({"carry", Screen::Dungeon, &cmdCarry, "carry <member> <slot>  - carry a fallen companion"});
}

}  // namespace mordor
