#include "mordor/ref_respawn.h"

namespace mordor {

// ref: src/Mordor.Core/DungeonState.cs:82 DungeonState.RespawnDelay
double respawnDelay(i32 level, i32 headerAreas, i32 tally) {
    // Base 1200 s, plus 5 s * (level / 2) * 60 for the floor.
    double t = 1200 + ((5 * (level / 2.0)) * 60);
    // CONFLICT: the community formula adds 5 s per spawn area; the reference (and this port)
    // subtracts 5 s for every room emptied and not yet refilled (headerAreas - tally).
    t = t - ((headerAreas - tally) * 5);
    // Floor of 1200 / 10 = 120 s.
    if (t < 1200 / 10.0) t = 1200 / 10.0;
    return t;
}

}  // namespace mordor

namespace mordor {
double shim_export_respawnDelay(i32 level, i32 headerAreas, i32 tally) {
    return respawnDelay(level, headerAreas, tally);
}
}  // namespace mordor
