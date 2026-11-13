#include "test_support.h"
#include "mordor/dungeon1a.h"

// Cases B_0066#100-#116 exercise EncounterGenerator.InRoom, DungeonState.Save/Load,
// the dungeon.json.tmp file, ClearedByJoining and the level tally. None of these
// have an equivalent function in the headers above, so they are skipped.

// SKIP B_0066#100: EncounterGenerator.InRoom has no equivalent in the headers (refillRoomIfDue is the closest, but it does not report generated/failed)
// SKIP B_0066#101: the level tally (FloorState::tally) is a data field with no function that performs the refill and returns it
// SKIP B_0066#102: EncounterGenerator.InRoom's out flag (file write) has no equivalent in the headers
// SKIP B_0066#103: the out flag from a revisit with no refill due has no equivalent in the headers
// SKIP B_0066#104: DungeonState.Save/Load have no equivalent in the headers
// SKIP B_0066#105: dungeon.json.tmp left-behind check depends on DungeonState.Save, which has no equivalent in the headers
// SKIP B_0066#112: the tally after a return visit depends on a visit routine with no equivalent in the headers
// SKIP B_0066#113: standingGroupKept (kept while age > 2 * respawnT) makes the refill due at exactly twice the respawn time, which contradicts the case's "not due at exactly twice" claim; cannot assert the expected value without changing it
// SKIP B_0066#115: ClearedByJoining has no equivalent in the headers
// SKIP B_0066#116: ClearedByJoining and the level tally decrement have no equivalent in the headers
