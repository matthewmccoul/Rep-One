#include "test_support.h"

// Batch t160. None of the reference cases can be expressed against the declarations
// provided: Chute.MoveDown, Floor.AreaIdAt and the whole DungeonState API (RespawnDelay,
// IsOccupied, RefillDue, MarkCleared, TallyOf, LoadRoom) are not declared in our headers.
// Tests will be added once those declarations exist (see NEED line in the batch reply).

using namespace mordor;

// SKIP B_0066#62: Chute.MoveDown / Floor.AreaIdAt not declared (NEED: moveDown, areaIdAt, Character level/x/y/area fields)
// SKIP B_0066#95: DungeonState.IsOccupied not declared (NEED: DungeonState::isOccupied, respawnSeconds, tallyOf)
// SKIP B_0066#96: DungeonState.IsOccupied not declared (NEED: DungeonState::isOccupied, respawnSeconds, tallyOf)
// SKIP B_0066#98: DungeonState.MarkCleared / LoadRoom not declared (NEED: DungeonState::markCleared, loadRoom)
// SKIP B_0066#94: DungeonState.MarkCleared / TallyOf not declared (NEED: DungeonState::markCleared, tallyOf)
// SKIP B_0066#97: DungeonState.MarkCleared / TallyOf not declared (NEED: DungeonState::markCleared, tallyOf)
// SKIP B_0066#113: DungeonState.RefillDue / RespawnSeconds not declared (NEED: DungeonState::refillDue, respawnSeconds)
// SKIP B_0066#90: DungeonState.RespawnDelay not declared (NEED: DungeonState::respawnDelay; pure arithmetic, expected 1350 for level 1, headerAreas 50, tally 50)
