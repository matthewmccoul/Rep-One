#include "flow_support.h"
#include "mordor/access.h"
#include "mordor/guilds2.h"
using namespace flowsup;

TEST(feat_quests_forfeit_drops_membership) {
  Session s;
  CHECK(makeSession(s, 1));
  Character* aldric = accMember(s, 1);
  CHECK(aldric != nullptr);
  const size_t g = static_cast<size_t>(Guild::Warrior);
  aldric->guilds[g].joined = true;
  aldric->guilds[g].level = 3;
  Ctx ctx = s.ctx();
  CHECK(Forfeit(ctx, *aldric, Guild::Warrior));
  CHECK(!aldric->guilds[g].joined);
}
