#include "test_support.h"
#include "mordor/partya.h"
#include "mordor/partyb.h"

// SKIP B_0068#18: needs a combat-round simulation with a ListUpdate callback; no header function runs monster attacks or reports list updates.
// SKIP B_0068#19: needs a combat-round simulation to count list updates from the characters' turns; no header function provides this.
// SKIP B_0068#20: no header function named RemoveTheDead; pruneOneMember's contract for removing dead and stoned members is not specified closely enough to map this assertion.
// SKIP A_03#34: MDATA12 import is a data-file importer; no header function parses MDATA12 or returns member lists.
// SKIP A_03#43: autoloadMember and autoloadParty return AutoloadOutcome/AutoloadResult enums, not the message text; the "Party.AutoloadPrefix" message is not exposed in the headers.
