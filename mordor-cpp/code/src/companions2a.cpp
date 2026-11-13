// companions2a.cpp: binding, joining, pen trade, charm, breath, mishap, resistance,
// stone, drift, leave/turn and alignment helpers (SRD c12p2, rules R8-R42).
//
// CONFLICT: this header declares several functions that are also declared with the
// same signature in other module headers (API INDEX). Per the module rules they are
// NOT defined here; they are implemented by the owning module and called from here:
//   firstFreeCompanionSlot, startingBinding, boughtBinding, bindingCostFactor, joinBar
//       -> companions1a.h
//   mishapChance(MishapKind, i32, i32)  -> companions1a.h
//   companionCastFails(Ctx&)            -> companions1b.h
//   clampChanceToLeave(i32)             -> companions2b.h
// CONFLICT: companions2a.h does not include companions1a.h, yet it uses MishapKind and
// SpawnWindow, which are declared there. This file includes companions1a.h first so
// that the header compiles.
// UNKNOWN: kFreeWillBinding (used by inline joinerBinding()) is not in any supplied
// header; it is assumed to be provided by game_data.h or another shared header.

#include "mordor/companions1a.h"
#include "mordor/companions2a.h"
#include "mordor/companions2b.h"

#include <cmath>

namespace mordor {

namespace {

constexpr i32 kPenShelfCap = 9;  // R19 mc0298; engine preferred (enforced)

}  // namespace

}  // namespace mordor
