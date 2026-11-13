#include "test_support.h"
#include "mordor/companions1b.h"

using namespace mordor;

// SKIP B_0055#28: draw count (2) not observable; ScriptedRng exposes no draw counter in the headers.

// SKIP B_0055#29: draw count (2) not observable; ScriptedRng exposes no draw counter in the headers.

// SKIP B_0055#30: random draw count not observable through the headers.
// SKIP B_0055#31: random draw count (0) not observable through the headers.

// SKIP B_0055#44: Settle(monsters) has no equivalent in the headers. settleLegacyCompanionId returns no count.
// SKIP B_0055#51: needs sequential RNG state across two checks, plus name resolution ("Shadow Ninja") not available in the headers.
