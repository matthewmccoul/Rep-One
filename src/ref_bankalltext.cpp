#include "mordor/ref_bankalltext.h"

namespace mordor {
namespace {

// Appends the decimal digits of a non-negative value.
void appendDigits(Str& out, i64 v) {
    char buf[24];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + v % 10);
        v /= 10;
    } while (v > 0);
    while (n > 0) {
        out.push_back(buf[--n]);
    }
}

}  // namespace

// Str$ of a currency value (1/10000 gold) with LTrim applied: no leading space,
// no leading zero before the point when the whole part is 0 (".25"), and
// fraction digits trimmed of trailing zeros ("89.5").
Str bankalltext(Currency balance) {
    Str out;
    if (balance < 0) {
        out.push_back('-');
    }
    i64 a = balance < 0 ? -balance : balance;
    i64 whole = a / 10000;
    i64 frac = a % 10000;
    if (frac == 0) {
        appendDigits(out, whole);
        return out;
    }
    if (whole > 0) {
        appendDigits(out, whole);
    }
    out.push_back('.');
    char d[4];
    for (int i = 3; i >= 0; --i) {
        d[i] = static_cast<char>('0' + frac % 10);
        frac /= 10;
    }
    int len = 4;
    while (len > 0 && d[len - 1] == '0') {
        --len;
    }
    for (int i = 0; i < len; ++i) {
        out.push_back(d[i]);
    }
    return out;
}

}  // namespace mordor
