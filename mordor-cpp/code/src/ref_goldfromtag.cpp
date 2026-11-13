// Port of GiveCommand.GoldFromTag / CLng (src/Mordor.Core/GiveCommand.cs:101).
// NOTE: rounding uses std::nearbyint, whose default mode is round-half-even, so no
// math_util roundHalfEven is needed (not among the supplied types).
// UNKNOWN: thousands-separator grouping is not validated (.NET AllowThousands is
// lenient here too); commas are accepted between integer digits only.
// UNKNOWN: &H/&O digit validation and overflow wrap are approximated: any
// out-of-range value is treated as Overflow (null).
// Assumes the C locale (decimal point '.'), which the core never changes.
#include "mordor/ref_goldfromtag.h"

#include <cmath>
#include <cstdlib>
#include <string>

namespace mordor {
namespace {

}  // namespace

}  // namespace mordor
