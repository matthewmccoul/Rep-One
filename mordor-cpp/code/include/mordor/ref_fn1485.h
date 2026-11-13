#pragma once
#include "mordor/types.h"

namespace mordor {

// Reference: src/Mordor.Core/Rules.cs:604 (Fn1485Price). Spell price floored after the exact
// product level*rate*u4*19/(10*max(1,sl)); rated = round_half_even(level*rate) is returned via out.
i32 fn1485Price(i32 level, i32 u4, f32 rate, i32 sl, i32& rated);

}  // namespace mordor
