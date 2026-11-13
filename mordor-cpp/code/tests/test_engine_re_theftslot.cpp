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

}  // namespace

