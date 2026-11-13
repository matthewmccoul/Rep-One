#include "flow_support.h"
#include "mordor/access.h"

namespace mordor {
// Defined in src/feat_companions.cpp; declared here for the smoke test.
bool companionsReleaseSlot(Session& s, i32 member1, i32 slot1, Str& reply);
}

using namespace flowsup;

TEST(feat_companions_release_empties_slot) {
  Session s;
  CHECK(makeSession(s, 1));
  Arr<Companion, kCompanionSlots>* slots = accCompanions(s, 1);
  CHECK(slots != nullptr);
  (*slots)[0].occupied = true;
  (*slots)[0].ref = 0;
  (*slots)[0].hits = 5;
  Str reply;
  CHECK(mordor::companionsReleaseSlot(s, 1, 1, reply));
  CHECK(!(*accCompanions(s, 1))[0].occupied);
  CHECK((*accCompanions(s, 1))[0].ref == -1);
}
