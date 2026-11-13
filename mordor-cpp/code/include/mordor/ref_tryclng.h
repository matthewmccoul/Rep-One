#pragma once
#include "mordor/types.h"

namespace mordor {

// ref: src/Mordor.Core/VbConvert.cs:22 VbConvert.TryCLng. Returns false on refusal, with value left at 0.
bool tryClng(const Str& text, i32& value);

}  // namespace mordor
