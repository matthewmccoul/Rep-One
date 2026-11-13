#include "mordor/ref_placeof.h"

namespace mordor {

namespace {

// Appends the decimal form of v to out. Avoids stdio and <string> conversions.
// Assumes Str supports operator+= with a char (std::string-compatible).
void appendInt(Str& out, i32 v) {
    if (v < 0) {
        out += '-';
        v = -v;
    }
    char buf[12];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v > 0);
    while (n > 0) {
        out += buf[--n];
    }
}

}  // namespace

bool placeOf(bool lostLevel, bool lostPosition, i32 level, i32 x, i32 y, Str& out) {
    // Live square is passed in by the caller (dungeon's current square), not the saved record.
    if (lostLevel || lostPosition || level <= 0) {
        return false;
    }
    out = Str();
    appendInt(out, x + 1);
    out += ',';
    appendInt(out, y + 1);
    out += ',';
    appendInt(out, level);
    return true;
}

}  // namespace mordor
