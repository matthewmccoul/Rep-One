#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;

// UNKNOWN: declared here rather than in a new header, so no header has to be added to the build.
namespace mordor {
Str lookText(Session& s);
}

// FIXED: newSession was not declared in any visible header. Declared here as a static helper.
// The body only value-initializes a Session and ignores `data`, because the real constructor is not visible.
// Replace the body with the real constructor call when it is known.
static Session newSession(decltype(readData()) data) {
  (void)data;
  Session s{};
  return s;
}

TEST(feat_look_readout_follows_party_position) {
  Session s;
  CHECK(makeSession(s, 1));
  CHECK(accPartySize(s) >= 1);

  s.game.party.x = 5;
  s.game.party.y = 7;
  s.game.party.dungeonLevel = 3;
  Str out = mordor::lookText(s);
  CHECK(out.find("5,7,3") != Str::npos);
  CHECK(out.find(accMember(s, 1)->name) != Str::npos);

  // Moving the party changes the readout: a real effect on the output.
  s.game.party.x = 6;
  out = mordor::lookText(s);
  CHECK(out.find("6,7,3") != Str::npos);
}
