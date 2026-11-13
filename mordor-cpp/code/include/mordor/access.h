// Access helpers: the one place that says how to reach each piece of game state from a Session. Features use these instead of guessing.
// Conventions: party members are numbered 1..4 as in the commands; pack slots are PLAYER slots 1..40 (slot 1 = the hands, never an item);
// Currency is gold x 10000; Party.size is the member count; a member's data is spread over parallel arrays indexed by (member - 1):
//   s.game.party.members[i]      Character (name, race, alignment, activeGuild, guilds[g] {joined, level, experience, att, def}, abilities.stats[6],
//                                vitals {curHits, maxHits, curSpellPoints, maxSpellPoints}, wallet {goldOnHand, goldInBank}, ailments, resistances,
//                                deathState, ageDays, level, experience, location)
//   s.game.party.inventories[i]  Inventory (pack[40] of Item {id, equipped, charges, idLevel, alignment, guildIssued, ...}, bank[], freeHands, carryIndex)
//   s.game.memberCompanions[i]   Arr<Companion, 4>  (ref = creature table index, occupied, owner, present, hits via Companion fields)
//   s.game.party.x / y (1-based square), dungeonLevel (0 = town), facing; s.game.floors[]: FloorState {dungeonLevel, squares[]}
//   s.town.store (Vec<Item> shelf), s.town.pens (PenBook), s.town.deepestReached; s.featureState (std::map<Str,Str>) for any other state
//   s.data: GameData tables (races, guilds, spells, items, monsters, floors, itemTypes); an ItemDef's id is NOT its table index.
#pragma once
#include "mordor/game.h"
#include "mordor/parse.h"

namespace mordor {

inline Str accLower(const Str& w) {
  Str o = w;
  for (char& c : o) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  return o;
}
inline bool accParseInt(const Str& w, i64& out) {
  return parseDecimal(w, out, true, 0, 4000000000000000000LL);
}

// ---- party ----
inline i32 accPartySize(const Session& s) { return s.game.party.size; }
inline Character* accMember(Session& s, i32 member1) {
  return (member1 >= 1 && member1 <= s.game.party.size) ? &s.game.party.members[static_cast<size_t>(member1 - 1)] : nullptr;
}
inline Inventory* accInventory(Session& s, i32 member1) {
  return (member1 >= 1 && member1 <= s.game.party.size) ? &s.game.party.inventories[static_cast<size_t>(member1 - 1)] : nullptr;
}
// Pack item at a PLAYER slot (2..40 hold items; slot 1 is the hands). nullptr when out of range; check ->id > 0 for "something is there".
inline Item* accPackItem(Session& s, i32 member1, i32 slot1) {
  Inventory* inv = accInventory(s, member1);
  return (inv != nullptr && slot1 >= 1 && slot1 <= kPackSlots) ? &inv->pack[static_cast<size_t>(slot1 - 1)] : nullptr;
}
inline Arr<Companion, kCompanionSlots>* accCompanions(Session& s, i32 member1) {
  return (member1 >= 1 && member1 <= s.game.party.size) ? &s.game.memberCompanions[static_cast<size_t>(member1 - 1)] : nullptr;
}
inline Currency& accGoldOnHand(Character& c) { return c.wallet.goldOnHand; }
inline i32 accMemberByName(const Session& s, const Str& name) {   // 1-based, 0 when absent (case-insensitive)
  for (i32 i = 0; i < s.game.party.size; ++i) if (accLower(s.game.party.members[static_cast<size_t>(i)].name) == accLower(name)) return i + 1;
  return 0;
}

// ---- tables ----
inline const ItemDef* accItemDef(const Session& s, i32 itemId) {
  for (const ItemDef& d : s.data.items) if (d.id == itemId) return &d;
  return nullptr;
}
inline const SpellDef* accSpellById(const Session& s, i32 spellId) {
  for (const SpellDef& d : s.data.spells) if (d.id == spellId) return &d;
  return nullptr;
}
inline const SpellDef* accSpellByName(const Session& s, const Str& name) {   // case-insensitive; also accepts the numeric id
  i64 id = 0;
  if (accParseInt(name, id)) return accSpellById(s, static_cast<i32>(id));
  const Str want = accLower(name);
  for (const SpellDef& d : s.data.spells) if (accLower(d.name) == want) return &d;
  return nullptr;
}
inline const MonsterDef* accMonster(const Session& s, i32 tableIndex) {
  return (tableIndex >= 0 && tableIndex < static_cast<i32>(s.data.monsters.size())) ? &s.data.monsters[static_cast<size_t>(tableIndex)] : nullptr;
}
inline const MonsterDef* accMonsterByName(const Session& s, const Str& name) {
  const Str want = accLower(name);
  for (const MonsterDef& d : s.data.monsters) if (accLower(d.name) == want) return &d;
  return nullptr;
}
inline const ItemDef* accItemByName(const Session& s, const Str& name) {
  const Str want = accLower(name);
  for (const ItemDef& d : s.data.items) if (accLower(d.name) == want) return &d;
  return nullptr;
}

// ---- the square under the party (nullptr in town); chests live on SquareState (hasChest, chestGold, chestItems, chestTrap, chestLock, chestDisarm) ----
inline SquareState* accSquareHere(Session& s) {
  const Party& p = s.game.party;
  if (p.dungeonLevel < 1) return nullptr;
  i32 width = 30;
  for (const FloorDef& d : s.data.floors) if (d.levelNum == p.dungeonLevel) { width = d.width; break; }
  for (FloorState& f : s.game.floors) {
    if (f.dungeonLevel != p.dungeonLevel) continue;
    const size_t idx = static_cast<size_t>((p.y - 1) * width + (p.x - 1));
    return idx < f.squares.size() ? &f.squares[idx] : nullptr;
  }
  return nullptr;
}

}  // namespace mordor
