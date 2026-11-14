#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct VbConvert {
    static bool TryCLng(cs::String text, int32_t& value);
    static int32_t HexDigit(char c);
};

}  // namespace mc
