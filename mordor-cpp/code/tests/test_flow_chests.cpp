// Opening a chest through the public game API. Written by the coordinator; the implementing ants may not change it.
// Reference behaviour (loot modules / ChestTraps): the trap is resolved first, then gold is shared and the items are placed
// with the party; opening takes several `open` commands (the opening countdown); an opened chest is gone.
#include "flow_support.h"
#include "mordor/state_dungeon.h"

using namespace flowsup;

namespace {
constexpr Currency kGold = 10000;
SquareState* squareHere(Session& s) {
  for (FloorState& f : s.game.floors) {
    if (f.dungeonLevel != s.game.party.dungeonLevel) continue;
    const size_t idx = static_cast<size_t>((s.game.party.y - 1) * 30 + (s.game.party.x - 1));
    if (idx < f.squares.size()) return &f.squares[idx];
  }
  return nullptr;
}
Currency partyGold(const Session& s) {
  Currency t = 0;
  for (i32 i = 0; i < s.game.party.size; ++i) t += s.game.party.members[static_cast<size_t>(i)].wallet.goldOnHand;
  return t;
}
i32 packItems(const Session& s) {
  i32 n = 0;
  for (i32 i = 0; i < s.game.party.size; ++i)
    for (const Item& it : s.game.party.inventories[static_cast<size_t>(i)].pack) if (it.id > 0 && !it.equipped) ++n;
  return n;
}
// A session on dungeon level 1 with a plain chest on the party's square.
bool chestSession(Session& s, Currency gold, i32 items, ChestTrap trap) {
  if (!makeSession(s, 3)) return false;
  Str reply;
  townAction(s, "descend", reply);
  SquareState* sq = squareHere(s);
  if (sq == nullptr) return false;
  sq->hasChest = true; sq->chestGold = gold; sq->chestTrap = trap; sq->chestLock = ChestLock::UNLOCKED;
  sq->chestItems.clear();
  for (i32 k = 0; k < items; ++k) { Item it; it.id = 1; sq->chestItems.push_back(it); }
  return true;
}
// Issue `open` until the chest is gone (the opening countdown takes a few commands); returns the last reply.
Str openUntilGone(Session& s) {
  Str reply, all;
  for (int i = 0; i < 8; ++i) {
    dungeonAction(s, "open", reply); all += reply + "\n";
    const SquareState* sq = squareHere(s);
    if (sq == nullptr || !sq->hasChest) break;
  }
  return all;
}
}  // namespace

TEST(flow_chests_opening_pays_the_gold_to_the_party_and_removes_the_chest) {
  Session s; CHECK(chestSession(s, 1000 * kGold, 0, ChestTrap::NotTrapped));
  const Currency before = partyGold(s);
  const Str said = openUntilGone(s);
  const SquareState* sq = squareHere(s);
  CHECK(sq != nullptr && !sq->hasChest);
  const Currency got = partyGold(s) - before;
  CHECK(got > 990 * kGold && got <= 1000 * kGold);       // shared out (any rounding remainder stays unpaid)
  CHECK(said.find("gold") != Str::npos);                  // and the player is told
  Str reply;
  dungeonAction(s, "open", reply);
  CHECK(reply.find("no chest") != Str::npos);
}

TEST(flow_chests_items_in_the_chest_end_up_in_the_party_packs) {
  Session s; CHECK(chestSession(s, 0, 2, ChestTrap::NotTrapped));
  const i32 before = packItems(s);
  const Str said = openUntilGone(s);
  CHECK_EQ(packItems(s), before + 2);
  CHECK(said.size() > 0);
}

TEST(flow_chests_a_trapped_chest_springs_its_trap_and_says_so) {
  Session s; CHECK(chestSession(s, 100 * kGold, 0, ChestTrap::Fire));
  i32 hits0 = 0;
  for (i32 i = 0; i < s.game.party.size; ++i) hits0 += s.game.party.members[static_cast<size_t>(i)].vitals.curHits;
  const Str said = openUntilGone(s);
  i32 hits1 = 0;
  for (i32 i = 0; i < s.game.party.size; ++i) hits1 += s.game.party.members[static_cast<size_t>(i)].vitals.curHits;
  CHECK(hits1 < hits0);                                   // a fire trap hurts
  CHECK(said.find("trap") != Str::npos || said.find("Trap") != Str::npos);
}
