#include "flow_support.h"
using namespace flowsup;

TEST(feat_options_set_game_bool) {
  Session s;
  CHECK(makeSession(s, 3));
  Str reply;
  CHECK(townAction(s, "options NoMusic on", reply));
  CHECK(townAction(s, "options nomusic", reply));
  CHECK(reply.find("on") != Str::npos);
}

TEST(feat_options_rejects_bad_value) {
  Session s;
  CHECK(makeSession(s, 3));
  Str reply;
  CHECK(townAction(s, "options Brightness 200", reply));
  CHECK(s.featureState.find("option.Brightness") == s.featureState.end());
}
