#include "flow_support.h"
#include "mordor/access.h"

#include <cmath>

namespace mordor {
bool seerCommand(Session& s, const Vec<Str>& w, Str& reply);  // defined in src/feat_seer.cpp
}

using namespace flowsup;

TEST(feat_seer_person_fee_moves_gold) {
  Session s;
  CHECK(makeSession(s, 1));
  mordor::Character* lyra = mordor::accMember(s, 2);
  CHECK(lyra != nullptr);
  lyra->location.dungeonLevel = 2;

  Vec<Str> ask = {"seer", "person", "Lyra"};
  Str offer;
  CHECK(mordor::seerCommand(s, ask, offer));
  CHECK(offer.find("Is this acceptable?") != Str::npos);

  i64 before = 0;
  for (mordor::i32 i = 1; i <= mordor::accPartySize(s); ++i) before += mordor::accMember(s, i)->wallet.goldOnHand;

  Vec<Str> yes = {"seer", "yes"};
  Str answer;
  CHECK(mordor::seerCommand(s, yes, answer));
  CHECK(answer.find("Not enough gold") == Str::npos);

  i64 after = 0;
  for (mordor::i32 i = 1; i <= mordor::accPartySize(s); ++i) after += mordor::accMember(s, i)->wallet.goldOnHand;

  const i64 expectedFee = static_cast<i64>(132567.52 * std::log(3.0) * 2.0);
  CHECK(before - after == expectedFee);
}
