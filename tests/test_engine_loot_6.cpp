#include "test_support.h"
#include "mordor/loot1a.h"
#include "mordor/loot1b.h"
#include "mordor/state_character.h"

using namespace mordor;

// SKIP B_0065#17: needs a PayGold fight simulation (gold on hand vs. amount owed); no single-call equivalent in headers
// SKIP B_0065#18: needs AwardSpoils fight flow; no single-call equivalent in headers
// SKIP B_0065#26: expects a "none" result for a logarithm error; openingRoundsForDisarm returns a plain i32 with no error channel
// SKIP B_0065#27: needs a seeded wall-monster encounter simulation to decide encounter gold; no single-call equivalent

