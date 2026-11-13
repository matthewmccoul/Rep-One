#include "test_support.h"
#include "mordor/dungeon2b.h"  // FIXED: mordor/rescue.h does not exist. Rescue types live in dungeon2b.h (RescueResult); daysUntilFound is assumed to be declared there. Verify.

using namespace mordor;

// NEED: daysUntilFound(Ctx&, i32 dungeonLevel, i32 areaId, bool easyToFind) -> i32
//       Declaration not provided. Signature assumed to follow the Ctx-first rule convention.
//       Also needs Raising::resolve / Stats / race minimum data for the B_0065 cases.

// SKIP B_0065#100: Raising.Resolve is not among the provided declarations, and the
//   expected value is symbolic ("the race minimum"), which needs the race table and a
//   Stats/Constitution accessor to compare against.

// SKIP B_0065#102: Raising.Resolve (counter raise) is not among the provided declarations.
//   The expected value is symbolic ("race minimum minus 1"), which needs the race table
//   and a Stats/Constitution accessor to compare against.

TEST(engine_rtt195_days_until_found_zero_roll_level1_area5) {
  // B_0068#7: ZeroRandom always rolls 0. Level 1, area 5 (<= 18) gives ceiling 100.
  // chance = 16 - 1 = 15, not easy to find. 0 * 100 <= 15 on the first pass, so days = 1.
  tsup::Env env({0.0});  // FIXED: Env is declared in tsup namespace (test_support.h), not at global scope.
  i32 days = daysUntilFound(env.ctx, 1, 5, false);
  CHECK_EQ(days, 1);
}

// SKIP B_0058#3: Asserts on the text of a decompiled BASIC source file (FRMCONTR.BAS
//   Form_Unload). This is not observable through the C++ port's API.
