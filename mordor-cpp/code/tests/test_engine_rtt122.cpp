#include "test_support.h"
#include "mordor/items1b.h"

using namespace mordor;

namespace {

constexpr int kTheftDraws = 120000;
constexpr int kTheftSlots = 42;  // outcomes 0..41

struct TheftTally {
  int count[kTheftSlots];
  int outside;  // draws outside 0..41 (should not happen)
};

// Same 120000 draws (seed 20260922) for every case. Local state only.
TheftTally tallyTheft() {
  TheftTally t{};
  tsup::Env env(static_cast<u64>(20260922));
  for (int i = 0; i < kTheftDraws; ++i) {
    i32 s = theftSlot(env.ctx);
    if (s >= 0 && s < kTheftSlots) {
      t.count[s]++;
    } else {
      t.outside++;
    }
  }
  return t;
}

}  // namespace

// B_0063#92: how many times is slot 0 drawn? expected 0
TEST(engine_rtt122_theft_slot0_never_drawn) {
  TheftTally t = tallyTheft();
  CHECK_EQ(t.count[0], 0);
}

// B_0063#93: how many times is slot 1 drawn? expected 0
TEST(engine_rtt122_theft_slot1_never_drawn) {
  TheftTally t = tallyTheft();
  CHECK_EQ(t.count[1], 0);
}

// B_0063#94: which of slots 2 to 39 are never drawn? expected (none)
TEST(engine_rtt122_theft_slots2to39_all_drawn) {
  TheftTally t = tallyTheft();
  int neverDrawn = 0;
  for (int s = 2; s <= 39; ++s) {
    if (t.count[s] == 0) neverDrawn++;
  }
  CHECK_EQ(neverDrawn, 0);
}

// B_0063#95: how many times is slot 40 drawn? expected at least 1
TEST(engine_rtt122_theft_slot40_drawn) {
  TheftTally t = tallyTheft();
  CHECK(t.count[40] >= 1);
}

// B_0063#96: how many times is slot 41 drawn? expected 0
TEST(engine_rtt122_theft_slot41_never_drawn) {
  TheftTally t = tallyTheft();
  CHECK_EQ(t.count[41], 0);
}

// B_0063#97: how many distinct slot outcomes (0 to 41) occur? expected 39
TEST(engine_rtt122_theft_distinct_outcomes_39) {
  TheftTally t = tallyTheft();
  CHECK_EQ(t.outside, 0);
  int distinct = 0;
  for (int s = 0; s < kTheftSlots; ++s) {
    if (t.count[s] > 0) distinct++;
  }
  CHECK_EQ(distinct, 39);
}

// B_0063#98: across slots 2 to 40, (hi - lo) < lo / 5 ? expected true
TEST(engine_rtt122_theft_slots2to40_even_spread) {
  TheftTally t = tallyTheft();
  int lo = t.count[2];
  int hi = t.count[2];
  for (int s = 2; s <= 40; ++s) {
    if (t.count[s] < lo) lo = t.count[s];
    if (t.count[s] > hi) hi = t.count[s];
  }
  CHECK((static_cast<double>(hi - lo)) < (static_cast<double>(lo) / 5.0));
}

// SKIP B_0056#1: CombatMonster / CombatMonster::LoadRecord (with Hits, AtNought, Alive)
// are not declared in our headers, so the MDATA5 record load cannot be expressed yet.
