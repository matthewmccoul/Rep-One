#include "mordor/features.h"

#include "mordor/access.h"
#include "mordor/casting1.h"
#include "mordor/game.h"
#include "mordor/state_common.h"
#include "mordor/types.h"
#include "mordor/items1b.h"

namespace mordor {

// Forward declaration with the signature features.h uses (Session&). Defined below.
i32 buffersRead(Session& s, i32 member, i32 slot);

namespace {

// SpellBuffers.cs: a buffer holds -1 (empty), a player pack slot (an item) or kPackSlots + spellIndex (a spell).
// Player slot 1 is the hands, never an item, so item buffers start at slot 2.
// UNKNOWN: the reference has ten buffers (SpellBuffers.Count = 10); this build exposes slots 1-4 only.
constexpr i32 kBufferEmpty = -1;
constexpr i32 kBufferSlotsExposed = 4;
constexpr i32 kFirstBufferablePackSlot = 2;
// CombatSetting 3 is Buffers[0], so buffer slot = CombatSetting - 2 (slot 1 = Buffers[0]).
constexpr i32 kCombatSettingBufferBase = 2;
const char* const kBufferUsage = "Usage: buffer <member> | buffer <member> <slot 1-4> <spell>";

Str bufferInt(i64 v) {
  if (v == 0) return "0";
  bool neg = v < 0;
  if (neg) v = -v;
  Str out;
  while (v > 0) {
    out.insert(out.begin(), static_cast<char>('0' + (v % 10)));
    v /= 10;
  }
  if (neg) out.insert(out.begin(), '-');
  return out;
}

bool bufferParseInt(const Str& t, i32& out) {
  if (t.empty() || t.size() > 6) return false;
  i32 v = 0;
  for (char c : t) {
    if (c < '0' || c > '9') return false;
    v = v * 10 + (c - '0');
  }
  out = v;
  return true;
}

Str bufferJoin(const Vec<Str>& w, size_t from) {
  Str out;
  for (size_t i = from; i < w.size(); ++i) {
    if (i > from) out += ' ';
    out += w[i];
  }
  return out;
}

bool bufferIsItem(i32 b) { return b >= 0 && b < kPackSlots; }
i32 bufferSpellIndex(i32 b) { return b - kPackSlots; }

// Spell table position through the data table. Past the end returns nullptr.
const SpellDef* bufferSpellDef(const Session& s, i32 spellIndex) {
  if (spellIndex < 0 || spellIndex >= static_cast<i32>(s.data.spells.size())) return nullptr;
  return &s.data.spells[static_cast<size_t>(spellIndex)];
}

// Find a spell by name or numeric id (accSpellByName), then report its table position.
// A spell buffer stores that position, not the spell id.
bool bufferFindSpell(const Session& s, const Str& name, i32& spellIndex) {
  const SpellDef* hit = accSpellByName(s, name);
  if (hit == nullptr) return false;
  for (size_t i = 0; i < s.data.spells.size(); ++i) {
    if (&s.data.spells[i] == hit) {
      spellIndex = static_cast<i32>(i);
      return true;
    }
  }
  return false;
}

// R18 / R28: the member's spell point cost for the spell, from castCostSp with item level 0
// (an item-free cast). False when the member or spell does not exist.
bool bufferSpellCost(Session& s, i32 member, i32 spellIndex, i64& out) {
  Character* c = accMember(s, member);
  const SpellDef* sp = bufferSpellDef(s, spellIndex);
  if (c == nullptr || sp == nullptr) return false;
  Ctx ctx = s.ctx();
  out = castCostSp(ctx, *c, *sp, 0);
  return true;
}

// Party membership through the roster (access helper), 1..party.size.
bool bufferMemberExists(Session& s, i32 member) {
  return accMember(s, member) != nullptr;
}

Str bufferMemberName(Session& s, i32 member) {
  Character* c = accMember(s, member);
  return c != nullptr ? c->name : Str();
}

// ---- storage ----
// featureState["buffers.<member>.<slot>"] = encoded buffer; absent = empty.
// featureState["order.<member>"] = standing order "fight" | "defend" | "spell:<id>"; absent = none.
// UNKNOWN: keyed by member number, not by character; a reordered party moves buffers with the number.

Str bufferKey(i32 member, i32 slot) {
  return "buffers." + bufferInt(member) + "." + bufferInt(slot);
}

// SpellBuffers.Describe: a pack-slot buffer whose slot is now empty resets to nothing; a spell past the
// end of the table shows "(Nothing)" but is not cleared.
Str buffersDescribe(Session& s, i32 member, i32 slot) {
  const i32 b = buffersRead(s, member, slot);
  if (b < 0) return "(Nothing)";
  if (bufferIsItem(b)) {
    if (b < kFirstBufferablePackSlot) {
      s.featureState.erase(bufferKey(member, slot));
      return "(Nothing)";
    }
    Item* it = accPackItem(s, member, b);
    if (it == nullptr || it->id <= 0) {
      s.featureState.erase(bufferKey(member, slot));
      return "(Nothing)";
    }
    const ItemDef* def = accItemDef(s, it->id);
    return def != nullptr ? "\"" + def->name + "\"" : Str("(unknown item)");
  }
  const i32 idx = bufferSpellIndex(b);
  const SpellDef* sp = bufferSpellDef(s, idx);
  if (sp == nullptr) return "(Nothing)";
  i64 c = 0;
  const Str cost = bufferSpellCost(s, member, idx, c)
                       ? (c == 0 ? Str("?") : bufferInt(c))  // lblBufferStat: "?" when the cost is nought
                       : Str("(cost unknown)");
  return "\"" + sp->name + "\"  " + cost;
}

Str buffersListText(Session& s, i32 member) {
  Str out = "Buffers for member " + bufferInt(member) + " (" + bufferMemberName(s, member) + "):";
  for (i32 slot = 1; slot <= kBufferSlotsExposed; ++slot) {
    out += "\n" + bufferInt(slot) + ". " + buffersDescribe(s, member, slot);
  }
  return out;
}

}  // namespace

// ---- command ----
// buffer <member>                       lists the buffers with their cost text
// buffer <member> <slot 1-4> <spell>    sets a spell buffer (name or numeric id)
// buffer <member> <slot 1-4> clear      empties the buffer
// The verb may be the first word or left out.
bool bufferCommand(Session& s, const Vec<Str>& w, Str& reply) {
  const size_t first = (!w.empty() && w[0] == "buffer") ? 1 : 0;
  if (w.size() <= first) {
    reply = kBufferUsage;
    return true;
  }
  i32 member = 0;
  if (!bufferParseInt(w[first], member) || member < 1) {
    reply = kBufferUsage;
    return true;
  }
  if (!bufferMemberExists(s, member)) {
    reply = "No party member " + bufferInt(member) + ".";
    return true;
  }
  const size_t argc = w.size() - first - 1;
  if (argc == 0) {
    reply = buffersListText(s, member);
    return true;
  }
  if (argc < 2) {
    reply = kBufferUsage;
    return true;
  }
  i32 slot = 0;
  if (!bufferParseInt(w[first + 1], slot) || slot < 1 || slot > kBufferSlotsExposed) {
    reply = "Slot must be 1 to 4.";
    return true;
  }
  const Str spellName = bufferJoin(w, first + 2);
  if (spellName.empty()) {
    reply = kBufferUsage;
    return true;
  }
  if (accLower(spellName) == "clear") {
    s.featureState.erase(bufferKey(member, slot));
    reply = "Buffer " + bufferInt(slot) + " of member " + bufferInt(member) + " is now empty.";
    return true;
  }
  i32 spellIndex = 0;
  if (!bufferFindSpell(s, spellName, spellIndex)) {
    reply = "No spell called \"" + spellName + "\".";
    return true;
  }
  s.featureState[bufferKey(member, slot)] = bufferInt(bufferForSpell(spellIndex));
  reply = "Buffer " + bufferInt(slot) + " of member " + bufferInt(member) +
          " now holds \"" + spellName + "\".";
  return true;
}

// ---- public helpers ----

// Read a buffer value: -1 when empty or unreadable.
i32 buffersRead(Session& s, i32 member, i32 slot) {
  auto it = s.featureState.find(bufferKey(member, slot));
  if (it == s.featureState.end()) return kBufferEmpty;
  i32 v = 0;
  if (!bufferParseInt(it->second, v)) return kBufferEmpty;
  return v;
}

void register_buffers() {
  registerFeatureCommand({"buffer", Screen::Town, &bufferCommand,
                          "buffer <member> [<slot 1-4> <spell>]  - set or list spell buffers"});
  registerFeatureCommand({"buffer", Screen::Combat, &bufferCommand,
                          "buffer <member> [<slot 1-4> <spell>]  - set or list spell buffers"});
}

}  // namespace mordor
