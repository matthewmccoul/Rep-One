#include "mordor/companions1b.h"

#include <cmath>
#include "mordor/companions2a.h"

namespace mordor {
namespace {
// UNKNOWN: Att of a companion caster is not identified in R27.3; Fight (Att 2) is used.
constexpr i32 kUnknownCasterAtt = 2;

}  // namespace

// ---------------------------------------------------------------- R27 spell or blow

// ---------------------------------------------------------------- R28 blows and flags

bool specialFlagBit(u32 flags, i32 bit) {
    if (bit < 0 || bit > 31) {
        return false;
    }
    return ((flags >> static_cast<u32>(bit)) & 1u) != 0u;
}

// ---------------------------------------------------------------- R32 mishap sounds and captions

// ---------------------------------------------------------------- R34 drift

}  // namespace mordor
