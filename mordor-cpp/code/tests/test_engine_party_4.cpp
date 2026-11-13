#include "test_support.h"
#include "mordor/partya.h"
#include "mordor/partyb.h"

using namespace mordor;

// SKIP B_0059#31: needs message counting on env.log; EventLog API is not in the headers.
// SKIP B_0059#32: needs ordered message capture on env.log; EventLog API is not in the headers.
// SKIP B_0059#33: the give name list is a UI list; no headers function returns it (giveAvailable returns bool only).
// SKIP B_0059#83: asks about statement order inside the original MAINMOD2.BAS source; not testable through the API.
// SKIP B_0059#84: asks about statement order inside the original MAINMOD2.BAS source; not testable through the API.
// SKIP B_0059#89: asks about the original FRMGROUP.BAS UPDATE routine; a UI/source-level behaviour, no matching function.
// SKIP B_0067#17: Party.PlanShortfall is not declared in the headers.
// SKIP B_0067#18: Party.PlanShortfall is not declared in the headers.

