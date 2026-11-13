#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;
// FIXED: newSession is not declared in flowsup, in flow_support.h, or in any header in the API index. The state headers live in namespace mordor, so bring that namespace in too.
using namespace mordor;

// FIXED: local stand-in for the missing newSession. It takes whatever readData() returns and builds a default Session. Replace it with the real declaration once its header is known; the data argument is currently unused.
static Session newSession(decltype(readData()) data) {
  (void)data;
  return Session{};
}

// ASSUMED (match flow_support.h / features.h if named differently):
//   newSession(readData()) builds a Session; townAction(s, line, reply) returns false on refusal.
TEST(feat_swap_exchanges_pack_slots) {
  Session s;
  CHECK(makeSession(s, 1));

  accPackItem(s, 1, 2)->id = 101;
  accPackItem(s, 1, 3)->id = 202;

  Str reply;
  const bool ok = townAction(s, "swap 1 2 3", reply);
  CHECK(ok);
  CHECK(accPackItem(s, 1, 2)->id == 202);
  CHECK(accPackItem(s, 1, 3)->id == 101);

  // Slot 1 (hands) must never move: a refused swap leaves the pack alone.
  const bool refused = townAction(s, "swap 1 1 3", reply);
  CHECK(!refused);
  CHECK(accPackItem(s, 1, 3)->id == 101);
}
