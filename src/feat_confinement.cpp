#include "mordor/features.h"
#include "mordor/state_common.h"
#include "mordor/access.h"
#include "mordor/city1.h"

namespace mordor {
namespace {

// Parses a plain decimal word without exceptions. Returns false on anything else.
bool confParseIndex(const Str& w, i32& out) {
  if (w.empty() || w.size() > 6) return false;
  i32 v = 0;
  for (char c : w) {
    if (c < '0' || c > '9') return false;
    v = v * 10 + static_cast<i32>(c - '0');
  }
  out = v;
  return true;
}

// Members are numbered 1..kPartyMaxSize on screen (same numbering as the access helpers).
bool confParseMember(const Str& w, i32& member) {
  return confParseIndex(w, member) && member >= 1 && member <= kPartyMaxSize;
}

// Companion slots are numbered 1..kCompanionSlots on screen (R67: fixed slots, never compacted).
bool confParseSlot(const Str& w, i32& slot) {
  return confParseIndex(w, slot) && slot >= 1 && slot <= kCompanionSlots;
}

// Plain integer text, no library dependency.
Str confNumText(i64 v) {
  if (v == 0) return "0";
  const bool neg = v < 0;
  if (neg) v = -v;
  Str out;
  while (v > 0) {
    out.insert(out.begin(), static_cast<char>('0' + static_cast<int>(v % 10)));
    v /= 10;
  }
  if (neg) out.insert(out.begin(), '-');
  return out;
}

// Whole gold from a currency value (i64 in 1/10000 gold).
Str confGoldText(i64 v) { return confNumText(v / 10000); }

Str confAlignName(i32 align) {
  if (align == 0) return "Good";      // pen index: 0 Good, 1 Neutral, 2 Evil (R30)
  if (align == 1) return "Neutral";
  return "Evil";
}

// One pen row: a creature table index (its Ref) and the item alignment (1 Good, 2 Neutral, 3 Evil).
struct ConfRow {
  i32 ref;
  i32 align;
};

// Rows are numbered in table order, then alignment 1..3, and only where penTakeAllowed says a
// creature can be taken from the pen.
Vec<ConfRow> confRows(const Session& s) {
  Vec<ConfRow> rows;
  const i32 n = static_cast<i32>(s.data.monsters.size());
  for (i32 ref = 0; ref < n; ++ref) {
    for (i32 align = 0; align < kPenAlignCount; ++align) {
      if (penTakeAllowed(s.town.pens, ref, align)) rows.push_back(ConfRow{ref, align});
    }
  }
  return rows;
}

// UNKNOWN: the guild level creatureBaseWorth takes is not given for the pen listing.
// The first member's character level is used; with no party, 1.
i32 confBaseLevel(const Session& s) {
  if (s.game.party.size < 1) return 1;
  return static_cast<i32>(s.game.party.members[0].level);
}

}  // namespace

// Buys pen row `row` (1-based, as listed by `pens`) for party member `member` (1-based).
// The payment and the pen count are handled by confinementBuy. The bought companion goes into
// the member's first free companion slot (R67); if none is free, nothing is charged.
bool confinementBuyRow(Session& s, i32 row, i32 member, Str& reply) {
  const Vec<ConfRow> rows = confRows(s);
  if (row < 1 || static_cast<size_t>(row) > rows.size()) {
    reply = "pen buy: there is no pen row with that number. See pens.";
    return false;
  }
  Character* payer = accMember(s, member);
  Arr<Companion, kCompanionSlots>* slots = accCompanions(s, member);
  if (payer == nullptr || slots == nullptr) {
    reply = "pen buy: no such party member.";
    return false;
  }
  i32 freeSlot = -1;
  for (i32 i = 0; i < kCompanionSlots; ++i) {
    if (!(*slots)[static_cast<size_t>(i)].occupied) {
      freeSlot = i;
      break;
    }
  }
  if (freeSlot < 0) {
    reply = "pen buy: that member has no free companion slot. Nothing was charged.";
    return false;
  }
  const ConfRow pick = rows[static_cast<size_t>(row - 1)];
  const MonsterDef* def = accMonster(s, pick.ref);
  const i64 before = static_cast<i64>(payer->wallet.goldOnHand);

  Ctx ctx = s.ctx();
  TownModAnswers mods{};  // UNKNOWN: default value of TownModAnswers is not in the packet.
  if (!confinementBuy(ctx, s.game.party, member - 1, s.town.pens, pick.ref, pick.align, mods)) {
    reply = "pen buy: the purchase was refused. Check your gold and the pen stock.";
    return false;
  }

  // R67: fill the first hole. The name the slot kept is left as it is.
  Companion& slot = (*slots)[static_cast<size_t>(freeSlot)];
  slot.ref = pick.ref;
  slot.occupied = true;
  slot.monsterId = def != nullptr ? def->id : 0;
  slot.alignment = static_cast<CompanionAlignment>(pick.align + 1);   // pen index 0..2 -> Good/Neutral/Evil 1..3
  slot.hits = def != nullptr ? def->hits : 0;

  const i64 paid = before - static_cast<i64>(payer->wallet.goldOnHand);
  const Str monsterName = (def != nullptr) ? def->name : Str("a creature");
  reply = "Bought " + monsterName + " (" + confAlignName(pick.align) + ") for " +
          confGoldText(paid) + " gold. " + payer->name + " has " +
          confGoldText(static_cast<i64>(payer->wallet.goldOnHand)) + " gold left.";
  return true;
}

// Sells the companion in slot `slot` (1-based) of party member `member` (1-based) back to the pens.
// R67: the slot is left empty but keeps its name and ref.
bool confinementSellSlot(Session& s, i32 slot, i32 member, Str& reply) {
  Character* seller = accMember(s, member);
  Arr<Companion, kCompanionSlots>* slots = accCompanions(s, member);
  if (seller == nullptr || slots == nullptr) {
    reply = "pen sell: no such party member.";
    return false;
  }
  Companion& comp = (*slots)[static_cast<size_t>(slot - 1)];
  if (!comp.occupied) {
    reply = "pen sell: that companion slot is empty.";
    return false;
  }
  const Companion copy = comp;
  const MonsterDef* def = accMonster(s, static_cast<i32>(copy.ref));
  const i64 before = static_cast<i64>(seller->wallet.goldOnHand);

  Ctx ctx = s.ctx();
  TownModAnswers mods{};  // UNKNOWN: default value of TownModAnswers is not in the packet.
  if (!confinementSell(ctx, s.game.party, member - 1, s.town.pens, copy, mods)) {
    reply = "pen sell: the pens would not take that companion.";
    return false;
  }

  comp.occupied = false;  // R67: the hole keeps the name and ref of its last occupant.
  const i64 got = static_cast<i64>(seller->wallet.goldOnHand) - before;
  const Str monsterName = (def != nullptr) ? def->name : Str("a creature");
  reply = "Sold " + monsterName + " to the Confinement pens for " + confGoldText(got) +
          " gold. Slot " + confNumText(slot) + " is empty.";
  return true;
}

namespace {

bool cmdPens(Session& s, const Vec<Str>& w, Str& reply) {
  (void)w;
  const Vec<ConfRow> rows = confRows(s);
  if (rows.empty()) {
    reply = "The Confinement pens are empty.";
    return true;
  }
  const i32 level = confBaseLevel(s);
  Str text = "Confinement pens:";
  for (size_t i = 0; i < rows.size(); ++i) {
    const MonsterDef* def = accMonster(s, rows[i].ref);
    if (def == nullptr) continue;
    const i32 penned = (static_cast<size_t>(rows[i].ref) < s.town.pens.byRef.size())
                           ? s.town.pens.byRef[static_cast<size_t>(rows[i].ref)][static_cast<size_t>(rows[i].align)] : 0;
    const Currency price = penBuyPrice(creatureBaseWorth(*def, level), penned);   // R22: the price falls as the pen fills
    text += "\n " + confNumText(static_cast<i64>(i + 1)) + ". " + def->name + " (" +
            confAlignName(rows[i].align) + ") x" + confNumText(penned) + ", " +
            confGoldText(static_cast<i64>(price)) + " gold";
  }
  reply = text;
  return true;
}

bool cmdPenBuy(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() != 4) {
    reply = "usage: pen buy <row> <member>";
    return false;
  }
  i32 row = 0;
  i32 member = 0;
  if (!confParseIndex(w[2], row) || row < 1) {
    reply = "pen buy: the row must be a number from the pens list.";
    return false;
  }
  if (!confParseMember(w[3], member)) {
    reply = "pen buy: the member must be a party position from 1 to 4.";
    return false;
  }
  return confinementBuyRow(s, row, member, reply);
}

bool cmdPenSell(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() != 4) {
    reply = "usage: pen sell <slot> <member>";
    return false;
  }
  i32 slot = 0;
  i32 member = 0;
  if (!confParseSlot(w[2], slot)) {
    reply = "pen sell: the slot must be a companion slot from 1 to 4.";
    return false;
  }
  if (!confParseMember(w[3], member)) {
    reply = "pen sell: the member must be a party position from 1 to 4.";
    return false;
  }
  return confinementSellSlot(s, slot, member, reply);
}

bool cmdPen(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "usage: pen buy <row> <member> | pen sell <slot> <member>";
    return false;
  }
  if (w[1] == "buy") return cmdPenBuy(s, w, reply);
  if (w[1] == "sell") return cmdPenSell(s, w, reply);
  reply = "usage: pen buy <row> <member> | pen sell <slot> <member>";
  return false;
}

}  // namespace

void register_confinement() {
  registerFeatureCommand({"pens", Screen::Town, &cmdPens,
                          "pens  - list creatures in the Confinement pens and their buy price"});
  registerFeatureCommand({"pen", Screen::Town, &cmdPen,
                          "pen buy <row> <member> | pen sell <slot> <member>  - buy or sell a companion"});
}

}  // namespace mordor
