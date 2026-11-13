#include <fstream>
#include <sstream>
#include "harness.h"
#include "mordor/ctx.h"
#include "mordor/json_lite.h"

using namespace mordor;

static bool loadData(GameData& gd, Str& err) {
  std::ifstream f(MORDOR_DATA_FILE);
  if (!f) { err = "cannot open data file"; return false; }
  std::stringstream ss; ss << f.rdbuf();
  return loadGameDataFromJson(ss.str(), gd, err);
}

TEST(math_round_half_even) {
  CHECK_EQ(roundHalfEven(2.5), 2);
  CHECK_EQ(roundHalfEven(3.5), 4);
  CHECK_EQ(roundHalfEven(2.4999), 2);
  CHECK_EQ(roundHalfEven(-0.5), 0);
  CHECK_EQ(roundHalfEven(250.25), 250);
  CHECK_EQ(vbInt(-1.5), -2);
  CHECK_EQ(vbFix(-1.5), -1);
}

TEST(rng_deterministic_and_in_range) {
  Rng a(42), b(42);
  for (int i = 0; i < 100; ++i) CHECK_EQ(a.nextU32(), b.nextU32());
  Rng r(7);
  for (int i = 0; i < 1000; ++i) { double d = r.nextDouble(); CHECK(d >= 0.0 && d < 1.0); i32 k = r.below(10); CHECK(k >= 0 && k < 10); }
  ScriptedRng s({0.5, 0.9});
  CHECK(s.nextDouble() == 0.5); CHECK(s.nextDouble() == 0.9); CHECK(s.nextDouble() == 0.9);
}

TEST(json_parses_basic_values) {
  JsonValue v; Str err;
  CHECK(parseJson("{\"a\":[1,2.5,true,null],\"b\":\"x\\ny\"}", v, err));
  CHECK(v.find("a") != nullptr);
  CHECK_EQ(v.find("a")->arr.size(), 4u);
  CHECK_EQ(v.find("b")->str, Str("x\ny"));
  JsonValue bad;
  CHECK(!parseJson("{\"a\":", bad, err));
}

TEST(data_file_loads_with_expected_counts) {
  GameData gd; Str err;
  CHECK(loadData(gd, err));
  CHECK_EQ(gd.races.size(), 9u);
  CHECK_EQ(gd.guilds.size(), 12u);
  CHECK_EQ(gd.spells.size(), 105u);
  CHECK_EQ(gd.items.size(), 366u);
  CHECK_EQ(gd.monsters.size(), 401u);
  CHECK_EQ(gd.floors.size(), 3u);
  CHECK_EQ(gd.floors[0].fields.size(), 900u);
  CHECK_EQ(gd.races[1].name, Str("Elf"));
  CHECK_EQ(gd.guilds[1].name, Str("Warrior"));
  CHECK_EQ(gd.guilds[0].abilities[4], 0.7);
}
