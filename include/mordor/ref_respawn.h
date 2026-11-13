#pragma once
#include "mordor/types.h"

namespace mordor {

// Respawn delay in seconds for a dungeon room (ref: src/Mordor.Core/DungeonState.cs:82 RespawnDelay).
double respawnDelay(i32 level, i32 headerAreas, i32 tally);

}  // namespace mordor
