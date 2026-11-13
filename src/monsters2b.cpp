// monsters2b.cpp
// Implements the functions declared in include/mordor/monsters2b.h.
// Rule numbers from the SRD part c11p2 are cited by their source chunk where the
// text gives no R-number. Items marked UNKNOWN or CONFLICT are guesses or
// unresolved SRD statements; see each comment.

#include "mordor/monsters2b.h"

#include "mordor/game_data.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

// ---- Rolls ----------------------------------------------------------------

// ---- Encounter window -----------------------------------------------------

// m0282: target floor of the encounter window: level, +1 on STUD [c7].
i32 encounterTargetFloor(i32 dungeonLevel, bool onStud) {
    return dungeonLevel + (onStud ? 1 : 0);
}

// ---- Companions -----------------------------------------------------------

// ---- Respawn and rooms ----------------------------------------------------

// ---- Library ---------------------------------------------------------------

}  // namespace mordor
