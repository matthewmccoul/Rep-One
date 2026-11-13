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

// SKIP B_0058#3: Asserts on the text of a decompiled BASIC source file (FRMCONTR.BAS
//   Form_Unload). This is not observable through the C++ port's API.
