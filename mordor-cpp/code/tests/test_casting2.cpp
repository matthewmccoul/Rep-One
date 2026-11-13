#include "test_support.h"
#include "mordor/casting2.h"

using namespace mordor;

// Worked example: Help's tutorial Nomad (INT 12, WIS 12) has 120 max SP.
TEST(max_spell_points_worked_example) {
    CHECK_EQ(maxSpellPoints(12, 12), 120);
}

