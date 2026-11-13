#include "mordor/ref_splitcoords.h"

namespace mordor {

namespace {

// VB Val() for the characters this loop can produce: skips leading blanks,
// takes an optional sign and the leading digits, and stops at the first other character.
// UNKNOWN: decimal points, hex (&H) and exponents are not handled; the reference's Val
// result is truncated to i32 here, and values beyond i32 range are clamped.
i32 vbVal(const Str& t) {
    size_t i = 0;
    while (i < t.size() && (t[i] == ' ' || t[i] == '\t')) {
        ++i;
    }
    bool neg = false;
    if (i < t.size() && (t[i] == '+' || t[i] == '-')) {
        neg = (t[i] == '-');
        ++i;
    }
    i64 v = 0;
    while (i < t.size() && t[i] >= '0' && t[i] <= '9') {
        v = v * 10 + static_cast<i64>(t[i] - '0');
        if (v > 2147483647LL) {
            v = 2147483647LL;
        }
        ++i;
    }
    return static_cast<i32>(neg ? -v : v);
}

}  // namespace

Vec<i32> splitCoords(const Str& s) {
    Vec<i32> pieces;
    const i64 n = static_cast<i64>(s.size());
    // UNKNOWN: the reference reads s.Substring(0, 1) before checking the length, so an
    // empty string throws there. Here an empty string yields no pieces.
    if (n == 0) {
        return pieces;
    }

    // One-based cursors as in the reference: 'at' is the position of the character being
    // examined, 'start' is the first character of the current piece.
    i64 at = 0;
    i64 start = 1;
    do {
        at++;
        if (s[static_cast<size_t>(at - 1)] == ',' || at == n) {
            // The last character is kept with the piece before it (see reference summary).
            if (at == n) {
                at++;
            }
            i64 len = at - start;
            const i64 rest = n - (start - 1);
            if (rest < len) {
                len = rest;
            }
            if (len > 0) {
                pieces.push_back(vbVal(s.substr(static_cast<size_t>(start - 1), static_cast<size_t>(len))));
            } else {
                pieces.push_back(vbVal(Str()));
            }
            start = at + 1;
        }
    } while (at < n);

    return pieces;
}

}  // namespace mordor
