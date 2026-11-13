#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;

// Assumes the harness helpers freshSession() and addMember(), and townAction(s, line, reply)
// as used by the other flow tests. Not compiled or run.

// FIXED: freshSession() is not declared in any header in the API index or state headers.
// Defined locally as static, returning a default-constructed Session.
static Session freshSession() {
  return Session{};
}

TEST(feat_realign_writes_alignment) {
  Session s;
  CHECK(makeSession(s, 1));

  Item* it = accPackItem(s, 1, 2);
  it->id = s.data.items[1].id;   // a real item, id > 0
  it->equipped = false;
  it->alignment = static_cast<decltype(it->alignment)>(2);   // neutral

  accGoldOnHand(*accMember(s, 1)) = static_cast<Currency>(1000000) * 10000;   // enough for any fee
  const Currency goldBefore = accGoldOnHand(*accMember(s, 1));
  Str reply;
  townAction(s, "realign 1 2 good", reply);

  // Real effect: the pack item's alignment is now good (1).
  CHECK(static_cast<i32>(accPackItem(s, 1, 2)->alignment) == 1);
  CHECK(accGoldOnHand(*accMember(s, 1)) < goldBefore);   // the fee was charged
}
