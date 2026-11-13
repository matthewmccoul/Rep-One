#include "flow_support.h"
#include "mordor/access.h"

using namespace flowsup;

// Until features.h declares the arrival entry point, the test declares it here.
namespace mordor {
Vec<Str> hallArrivalUpdate(Session& s, i64 daySerial);
}

TEST(feat_hall_arrival_sets_strength_record) {
  Session s;
  CHECK(makeSession(s, 1));
  s.game.party.members[0].abilities.stats[0] = 17;
  mordor::hallArrivalUpdate(s, 100);
  CHECK(s.featureState["hall.0.holder"] == accMember(s, 1)->name);
  CHECK(s.featureState["hall.0.value"] == "17");
}
