#include "test_support.h"
#include "mordor/items1b.h"

using namespace mordor;

namespace {

// Draws the theft roll n times with a fixed seed and tallies each slot.
// Local state only; no statics.
struct TheftTally {
  i64 counts[64] = {0};
  i64 outOfRange = 0;
};

TheftTally drawTheft(i64 n) {
  TheftTally t;
  tsup::Env env(static_cast<u64>(20260922));
  for (i64 i = 0; i < n; ++i) {
    i32 s = theftSlot(env.ctx);
    if (s < 0 || s >= 64) {
      ++t.outOfRange;
    } else {
      t.counts[s]++;
    }
  }
  return t;
}

}  // namespace

// [B_0063#92] slot 0 is never drawn
TEST(engine_re_theftslot_slot0_never_drawn) {
  TheftTally t = drawTheft(120000);
  CHECK_EQ(t.counts[0], 0);
}

// [B_0063#93] slot 1 is never drawn
TEST(engine_re_theftslot_slot1_never_drawn) {
  TheftTally t = drawTheft(120000);
  CHECK_EQ(t.counts[1], 0);
}

// [B_0063#94] every slot 2..39 is drawn at least once
TEST(engine_re_theftslot_slots2_to_39_all_drawn) {
  TheftTally t = drawTheft(120000);
  i32 missing = 0;
  for (i32 s = 2; s <= 39; ++s) {
    if (t.counts[s] == 0) ++missing;
  }
  CHECK_EQ(missing, 0);
}

// [B_0063#95] slot 40 is drawn at least once
TEST(engine_re_theftslot_slot40_drawn) {
  TheftTally t = drawTheft(120000);
  CHECK(t.counts[40] >= 1);
}

// [B_0063#96] slot 41 is never drawn
TEST(engine_re_theftslot_slot41_never_drawn) {
  TheftTally t = drawTheft(120000);
  CHECK_EQ(t.counts[41], 0);
}

// [B_0063#97] exactly 39 distinct outcomes in 0..41
TEST(engine_re_theftslot_distinct_outcomes_39) {
  TheftTally t = drawTheft(120000);
  i32 distinct = 0;
  for (i32 s = 0; s <= 41; ++s) {
    if (t.counts[s] > 0) ++distinct;
  }
  CHECK_EQ(distinct, 39);
}

// [B_0063#98] counts over slots 2..40 are within a fifth of each other
TEST(engine_re_theftslot_counts_within_a_fifth) {
  TheftTally t = drawTheft(120000);
  i64 lo = t.counts[2];
  i64 hi = t.counts[2];
  for (i32 s = 2; s <= 40; ++s) {
    if (t.counts[s] < lo) lo = t.counts[s];
    if (t.counts[s] > hi) hi = t.counts[s];
  }
  CHECK((hi - lo) < (lo / 5));
}
