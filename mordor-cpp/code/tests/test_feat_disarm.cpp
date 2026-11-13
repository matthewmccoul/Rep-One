#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;

namespace mordor {
bool disarmFinish(Session& s, i32 member, bool success, i32 monsterGuildLevel, Str& reply);
}

TEST(feat_disarm_success_clears_trap) {
  Session s;
  CHECK(makeSession(s, 1));
  s.game.floors.clear();
  s.game.party.dungeonLevel = 1;
  s.game.party.x = 1;
  s.game.party.y = 1;
  FloorState floor;
  floor.dungeonLevel = 1;
  floor.squares.resize(1);
  floor.squares[0].hasChest = true;
  floor.squares[0].chestTrap = ChestTrap::Fire;
  floor.squares[0].chestDisarm = 100;
  s.game.floors.push_back(floor);

  Str reply;
  CHECK(mordor::disarmFinish(s, 1, true, 0, reply));
  CHECK(s.game.floors[0].squares[0].chestTrap == ChestTrap::NotTrapped);
  CHECK(s.game.floors[0].squares[0].hasChest);
}
