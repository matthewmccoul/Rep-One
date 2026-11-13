#include "mordor/companions2b.h"
#include "mordor/math_util.h"

namespace mordor {

// Clamps a raw chance-to-leave value into the range the port allows.
// Upper bound is kChanceToLeaveMax (90) from state_companion.h [c2].
i32 clampChanceToLeave(i32 raw) {
    // RESOLVED-BY-REFERENCE: the lower bound is 0. Character.cs:192 (ChanceToLeave) ends with
    // `return Math.Clamp(x, 0, 90);`, and SRD [ch08] gives leaveValue = clamp((v/2 + 15*missingHP/maxHP)/2, 0, 90),
    // with [ch12] stating "clamped 0 to 90". The upper bound is kChanceToLeaveMax, which must equal 90.
    return clampv(raw, 0, kChanceToLeaveMax);
}


}  // namespace mordor
