#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"

using namespace mordor;

// SKIP B_0058#15: CityWindow is a UI class in the port; no equivalent in our C++ API.
// SKIP B_0058#13: DungeonWindow.AskToJoin is a UI method in the port; no equivalent in our C++ API.
// SKIP B_0058#14: DungeonWindow.AskToLeave is a UI method in the port; no equivalent in our C++ API.
// SKIP B_0068#18: needs CombatEngine (NoteWhoIsAlive, RoundPaced, ListUpdate callback); no declaration in our headers yet (see NEED).
// SKIP B_0068#19: needs CombatEngine.RoundPaced and a ListUpdate counter; no declaration in our headers yet (see NEED).
// SKIP B_0059#29: needs a Give command function (Ctx, from/to Character, pack slot, gold, item lookup) that emits "Give of ... Completed!" events; no declaration in our headers yet (see NEED).
// SKIP B_0059#32: needs the same Give command function; guild-issued refusal text is not declared in our headers (see NEED).
// SKIP B_0059#31: needs the same Give command function to count emitted messages for an empty slot with 0 gold (see NEED).
