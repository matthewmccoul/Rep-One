#include "flow_support.h"
using namespace flowsup;

TEST(feat_library_list_and_search) {
  Session s;
  CHECK(makeSession(s, 3));
  s.featureState["library.monster.Giant Rat"] = "2";
  Str reply;
  CHECK(townAction(s, "library", reply));
  CHECK(reply.find("Giant Rat") != Str::npos);
  CHECK(townAction(s, "library search zz", reply));
  CHECK(reply == "No match was found.");
}

TEST(feat_library_monster_grade1_size) {
  Session s;
  CHECK(makeSession(s, 3));
  s.featureState["library.monster.Giant Rat"] = "1";
  s.featureState["library.monster.Giant Rat.size"] = "1";
  Str reply;
  CHECK(townAction(s, "library monster Giant Rat", reply));
  CHECK(reply.find("Size: Small") != Str::npos);
  CHECK(reply.find("Class:") == Str::npos);
}
