#include "test_support.h"
#include "mordor/companions1b.h"

using namespace mordor;

// SKIP B_0064#99: runCarryChoice takes no owner name or on-screen flag, so the "Ann ... dropped" message text cannot be set up or checked.

// SKIP B_0064#101: the "0,-,-,-" notation for slot contents is not defined by the headers; cannot tell what it asserts.

// SKIP B_0064#102: the "Ann, you are now carrying" message needs the owner name and off-screen state, which runCarryChoice does not take.

// SKIP B_0064#107: needs a 3000-round combat simulation; no single function in the headers covers it.

// SKIP B_0064#108: needs a 3000-round combat simulation; no single function in the headers covers it.
