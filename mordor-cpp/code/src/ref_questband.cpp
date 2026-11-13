#include "mordor/ref_questband.h"

namespace mordor {

// FRMCTYG.BAS:524: l0110 = Int(l0110 / 1.5#) + 1. The reference uses a C# (int) cast, which truncates
// toward zero. That is not VB Int (floor), so vbInt is deliberately not used here. The two agree for tier >= 0.
i32 questItemBand(i32 tier) {
    return static_cast<i32>(static_cast<double>(tier) / 1.5) + 1;
}

}  // namespace mordor
