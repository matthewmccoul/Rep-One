// dungeon1b: room readout (R99, R100, R101).
// The SRD excerpt provided does not contain the text of R99 or R101, so the
// wording below is ASSUMED. Counts come from countRoster (R100), which is
// declared in dungeon1b.h and assumed to be implemented elsewhere.

#include "mordor/dungeon1b.h"

namespace mordor {
namespace {

// ASSUMED: plain decimal text with no locale or grouping.
Str numberText(i32 n) {
    if (n == 0) {
        return Str("0");
    }
    const bool negative = n < 0;
    i64 v = negative ? -static_cast<i64>(n) : static_cast<i64>(n);
    Str digits;
    while (v > 0) {
        digits = Str(1, static_cast<char>('0' + static_cast<int>(v % 10))) + digits;
        v /= 10;
    }
    if (negative) {
        digits = Str("-") + digits;
    }
    return digits;
}

}  // namespace


}  // namespace mordor
