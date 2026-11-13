#include "test_support.h"
// Dungeon state and encounter generation headers are not declared yet (see NEED above).
// Add the matching #include lines once they exist.
using namespace mordor;

// SKIP B_0058#10: DungeonWindow.SameAreaReload is UI code in the port's DungeonWindow.axaml.cs; no C++ counterpart is declared.
// SKIP B_0058#9: DungeonWindow.SettleArea is UI code in the port's DungeonWindow.axaml.cs; no C++ counterpart is declared.
// SKIP B_0066#115: EncounterGenerator.ClearedByJoining and DungeonState.IsOccupied are not declared in our API.
// SKIP B_0066#116: EncounterGenerator.ClearedByJoining and DungeonState.TallyOf are not declared in our API.
// SKIP B_0066#84: EncounterGenerator.Generate and the MDATA5/MDATA11 loaders are not declared in our API.
// SKIP B_0066#89: EncounterGenerator.InRoom and the Floor 2 loader are not declared in our API.
// SKIP B_0066#100: EncounterGenerator.InRoom and DungeonState.MarkCleared are not declared in our API.
// SKIP B_0066#102: EncounterGenerator.InRoom's out flag (written) is not declared in our API.
