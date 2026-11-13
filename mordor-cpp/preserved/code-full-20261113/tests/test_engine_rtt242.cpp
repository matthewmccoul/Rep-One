// Batch t242: assertions from Rescue, PlayClock and Seer reference tests.
// Every case below is skipped until the declarations listed in NEED are available.
// Once they are, replace each SKIP with a TEST that CHECKs the expected value verbatim.
#include "test_support.h"

using namespace mordor;

// SKIP B_0065#117: PlayClock is not in OUR DECLARATIONS (needs PlayClock and Character.playSeconds).
// Expected: Tick(a, 100) then Tick(a, 101) gives play seconds a=1, b=0.

// SKIP B_0066#70: splitCoords is not in OUR DECLARATIONS.
// Expected pieces for "15,5,3": 15|5|3.

// SKIP B_0066#71: splitCoords is not in OUR DECLARATIONS.
// Expected pieces for "15,5,3," (trailing comma): 15|5|3.

// SKIP B_0066#72: splitCoords is not in OUR DECLARATIONS.
// Expected piece count for "1,2,": 2.

// SKIP B_0066#73: rescueVal is not in OUR DECLARATIONS.
// Expected values for "1 5", "3.7", "1-2", "x": 15 4 1 0.

// SKIP B_0059#80: seerMatch and Found are not in OUR DECLARATIONS.
// Expected Match("Hands", itemNames): (One, 0).

// SKIP B_0059#79: seerMatch and Found are not in OUR DECLARATIONS.
// Expected Match("Kobold", names, plurals).found: One.

// SKIP B_0059#78: seerMatch and Found are not in OUR DECLARATIONS.
// Expected Match("Kobolds", names).found: None.
