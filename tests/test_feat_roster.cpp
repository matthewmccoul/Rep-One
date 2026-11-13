#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;

bool rosterUnload(mordor::Session& s, int member, mordor::Str& reply);
bool rosterLoad(mordor::Session& s, const mordor::Str& name, mordor::Str& reply);

TEST(feat_roster_unload_then_load_moves_the_party) {
  Session s;
  CHECK(makeSession(s, 1));
  CHECK(mordor::accPartySize(s) == 4);
  Str reply;
  // Borin is member 3 in the test session.
  CHECK(rosterUnload(s, 3, reply));
  CHECK(mordor::accPartySize(s) == 3);
  CHECK(mordor::accMemberByName(s, "Borin") == 0);
  CHECK(rosterLoad(s, "Borin", reply));
  CHECK(mordor::accPartySize(s) == 4);
  CHECK(mordor::accMemberByName(s, "Borin") == 4);
}
