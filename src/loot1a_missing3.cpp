#include "mordor/loot1a.h"

namespace mordor {

// R3 / F2: a lock is rolled only if the roll is at most the lead's LockedChance
// and the treasure class is above 1.
// CONFLICT-1: the SRD gives two forms of this test. The engine form [c7] draws
// Int(Rnd*100)+1 and compares it with LockedChance. The [c5] form uses the same
// draw and the same class test. This implementation takes the engine form, as the
// SRD preamble directs.
// ASSUMED: the draw is always consumed, even when the class is 1 or lower, so the
// RNG sequence stays aligned with the source. The [c5] form also draws first.
bool rollChestLock(Ctx& ctx, i32 treasureClass, i32 lockedChance) {
    const i32 roll = ctx.rng.below(100) + 1;   // Int(Rnd*100)+1
    const bool rolled = roll <= lockedChance;  // roll at most LockedChance
    return rolled && treasureClass > 1;        // class above 1 (R3)
}

}  // namespace mordor
