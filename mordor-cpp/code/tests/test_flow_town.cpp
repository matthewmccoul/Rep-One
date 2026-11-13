// Town commands through the public game API: bank, gear, identify, morgue, pool. Written by the coordinator
// (behaviour checked by playing the console), not by the implementing ants, so the code cannot drift to its own tests.
#include "flow_support.h"

using namespace flowsup;

namespace {
constexpr Currency kGold = 10000;
size_t equippedCount(const Inventory& inv) {
  size_t n = 0;
  for (const Item& it : inv.pack) if (it.id > 0 && it.equipped) ++n;
  return n;
}
}

TEST(flow_town_bank_deposit_then_withdraw) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  Purse& p = s.game.party.members[0].wallet;
  const Currency hand0 = p.goldOnHand;
  CHECK(townAction(s, "bank deposit 100", reply));
  CHECK_EQ(p.goldInBank, 100 * kGold);
  CHECK_EQ(p.goldOnHand, hand0 - 100 * kGold);
  CHECK(townAction(s, "withdraw 30", reply));
  CHECK_EQ(p.goldInBank, 70 * kGold);
  CHECK_EQ(p.goldOnHand, hand0 - 70 * kGold);
  CHECK(townAction(s, "withdraw 5000", reply));          // more than the bank holds: refused, nothing moves
  CHECK_EQ(p.goldInBank, 70 * kGold);
  CHECK(townAction(s, "deposit 99999999", reply));       // more than is on hand: refused
  CHECK_EQ(p.goldInBank, 70 * kGold);
  CHECK_EQ(p.goldOnHand, hand0 - 70 * kGold);
}

TEST(flow_town_unequip_then_equip_restores_the_worn_item) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  const size_t worn0 = equippedCount(s.game.party.inventories[0]);
  CHECK(worn0 > 0);
  CHECK(townAction(s, "unequip 1 2", reply));
  CHECK_EQ(equippedCount(s.game.party.inventories[0]), worn0 - 1);
  CHECK(townAction(s, "equip 1 2", reply));
  CHECK_EQ(equippedCount(s.game.party.inventories[0]), worn0);
}

TEST(flow_town_gear_commands_without_arguments_are_usage_errors) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  // The town handler answers a malformed command with a "Usage:" reply (and true, like buy/sell/bank).
  townAction(s, "equip 1", reply);
  CHECK(reply.find("Usage: equip") != Str::npos);
  townAction(s, "unequip", reply);
  CHECK(reply.find("Usage: unequip") != Str::npos);
  townAction(s, "use 1", reply);
  CHECK(reply.find("Usage: use") != Str::npos);
}

TEST(flow_town_hands_and_empty_slots_are_refused_without_changing_gear) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  const size_t worn0 = equippedCount(s.game.party.inventories[0]);
  CHECK(townAction(s, "use 1 1", reply));
  CHECK(reply.find("hands") != Str::npos);
  CHECK(townAction(s, "equip 1 9", reply));
  CHECK(reply.find("empty") != Str::npos);
  CHECK_EQ(equippedCount(s.game.party.inventories[0]), worn0);
}

TEST(flow_town_identify_with_nothing_unknown_takes_no_gold) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  const Currency hand0 = s.game.party.members[0].wallet.goldOnHand;
  CHECK(townAction(s, "identify", reply));
  CHECK(reply.size() > 0);
  CHECK_EQ(s.game.party.members[0].wallet.goldOnHand, hand0);
}

TEST(flow_town_morgue_lists_the_dead) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  CHECK(townAction(s, "morgue", reply));
  CHECK(reply.find("nobody is dead") != Str::npos);
  s.game.party.members[1].deathState = DeathState::Dead;
  CHECK(townAction(s, "morgue", reply));
  CHECK(reply.find("Lyra") != Str::npos);
}

TEST(flow_town_pool_moves_all_gold_to_the_first_member_and_keeps_the_total) {
  Session s; CHECK(makeSession(s, 3));
  Str reply;
  Currency total = 0;
  for (i32 i = 0; i < s.game.party.size; ++i) total += s.game.party.members[static_cast<size_t>(i)].wallet.goldOnHand;
  CHECK(townAction(s, "pool", reply));
  CHECK_EQ(s.game.party.members[0].wallet.goldOnHand, total);
  for (i32 i = 1; i < s.game.party.size; ++i) CHECK_EQ(s.game.party.members[static_cast<size_t>(i)].wallet.goldOnHand, 0);
}
