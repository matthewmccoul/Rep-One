#include "flow_support.h"
#include "mordor/access.h"

namespace mordor {
// Defined in src/feat_give.cpp.
bool giveRun(Session& s, const Vec<Str>& words, Str& reply);
}  // namespace mordor

using namespace flowsup;

TEST(feat_give_gold_moves) {
  mordor::Session s;
  s.game.party.size = 2;
  mordor::Character* a = mordor::accMember(s, 1);
  mordor::Character* b = mordor::accMember(s, 2);
  mordor::accGoldOnHand(*a) = 50 * 10000;
  mordor::accGoldOnHand(*b) = 0;
  mordor::Vec<mordor::Str> w = {"give", "gold", "1", "2", "10"};
  mordor::Str reply;
  mordor::giveRun(s, w, reply);
  CHECK(mordor::accGoldOnHand(*a) == 40 * 10000);
  CHECK(mordor::accGoldOnHand(*b) == 10 * 10000);
}
