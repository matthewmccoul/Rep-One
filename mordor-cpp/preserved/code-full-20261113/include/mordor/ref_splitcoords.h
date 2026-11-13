#pragma once
#include "mordor/types.h"

namespace mordor {

// Ref: src/Mordor.Core/Rescue.cs:128 Rescue.SplitCoords (FRMMORG.BAS:118-135), morgue comma loop.
// Pure text parsing: no Ctx, no RNG, no events.
Vec<i32> splitCoords(const Str& s);

}  // namespace mordor
