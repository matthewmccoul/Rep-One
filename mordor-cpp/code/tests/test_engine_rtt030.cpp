#include "test_support.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0062#17: The expected values are the SRD Races statistics, resistance and size tables, and our code has no race-table comparison to run. Without the race record field names from game_data.h, the field-by-field match count cannot be written.
// SKIP B_0062#18: Same as B_0062#17. The mismatch list depends on the same unseen race record layout and SRD table values.
// SKIP B_0064#3: Needs the name of the race table field in GameData (not in the declarations provided). Count would be data().<races>.size() == 9 once the field is known.

// NEED: GameData race table field name (races vector in game_data.h), RaceRecord field names (maxAge, expFactor, stat min/max, alignMask, size, the eleven resistances)
