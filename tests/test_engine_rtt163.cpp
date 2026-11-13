#include "test_support.h"
#include "mordor/game_data.h"

using namespace mordor;

// No test in this batch can be expressed against the declarations we have.
// Each case below is skipped until the declarations listed in NEED are available.

// SKIP B_0066#103: needs EncounterGenerator::InRoom, DungeonState (RefillDue/StoreRoom) and the out-flag; none declared yet
// SKIP B_0066#88: needs Floor::AreaIdAt / Floor::Areas and the MDATA11.MDR floor 1 loader; not declared
// SKIP B_0063#77: needs Floor::RESCUE automap sheet flag-bit loop; not declared
// SKIP B_0063#76: needs Floor::STUD automap sheet flag-bit loop; not declared
// SKIP B_0066#10: needs Character (IsDead, LostPosition, LostLevel, LostFacing), Facing and LookAround::Readout; not declared
// SKIP B_0056#29: needs LookAround::Roster, LookAround::Walk, Character and the saved-record list; not declared
// SKIP B_0066#64: needs Party::HereToJoin and Floor area membership; not declared
// SKIP B_0066#65: needs Party::HereToJoin and chute-square placement on the lower floor; not declared
