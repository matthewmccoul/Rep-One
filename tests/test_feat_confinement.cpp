#include "flow_support.h"
#include "mordor/access.h"
using namespace flowsup;

namespace mordor {
// Defined in src/feat_confinement.cpp: the body of `pen buy`, after the words are parsed.
bool confinementBuyRow(Session& s, i32 row, i32 member, Str& reply);
}

TEST(feat_confinement_buy_moves_gold) {
  Session s;
  CHECK(makeSession(s, 1));
  Character* ann = accMember(s, 1);
  CHECK(ann != nullptr);
  ann->wallet.goldOnHand = static_cast<Currency>(1000000) * 10000;  // a million gold: pen creatures cost tens of thousands (R22)
  const i64 before = static_cast<i64>(ann->wallet.goldOnHand);
  Str reply;
  const bool ok = confinementBuyRow(s, 1, 1, reply);
  CHECK(ok);
  CHECK(static_cast<i64>(accMember(s, 1)->wallet.goldOnHand) < before);
}
