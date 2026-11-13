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

bool isBlank(char c) { return c == ' ' || c == '\t'; }

bool isDigit(char c) { return c >= '0' && c <= '9'; }

char lowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// Value of a hex/octal digit character, or 99 if not a digit at all.
int digitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 99;
}

// VB Long range, as the reference's int.MaxValue / int.MinValue check.
bool fitsLong(double r) { return r <= 2147483647.0 && r >= -2147483648.0; }

}  // namespace

bool goldFromTag(const Str& tag, i64& out) {
    // Trim blanks (' ' and '\t') from both ends.
    size_t b = 0;
    size_t e = tag.size();
    while (b < e && isBlank(tag[b])) ++b;
    while (e > b && isBlank(tag[e - 1])) --e;

    // &H (radix 16) or &O (radix 8) prefix, case-insensitive.
    if (e - b >= 2 && tag[b] == '&' &&
        (lowerAscii(tag[b + 1]) == 'h' || lowerAscii(tag[b + 1]) == 'o')) {
        const int radix = (lowerAscii(tag[b + 1]) == 'h') ? 16 : 8;
        size_t p = b + 2;
        bool any = false;
        i64 acc = 0;
        for (; p < e; ++p) {
            const int d = digitValue(tag[p]);
            if (d >= radix) break;
            any = true;
            acc = acc * radix + d;
            if (acc > (static_cast<i64>(1) << 40)) return false;  // far past Long
        }
        if (!any || p != e) return false;
        if (acc > 2147483647LL) return false;  // Overflow (UNKNOWN: VB wrap)
        out = acc;
        return true;
    }

    // Decimal: optional sign, digits with thousands commas, optional
    // fraction, optional E/D exponent. The whole trimmed string must match.
    std::string s;
    size_t p = b;
    if (p < e && (tag[p] == '+' || tag[p] == '-')) {
        s.push_back(tag[p]);
        ++p;
    }

    size_t intDigits = 0;
    while (p < e) {
        if (isDigit(tag[p])) {
            s.push_back(tag[p]);
            ++intDigits;
            ++p;
        } else if (tag[p] == ',' && intDigits > 0) {
            ++p;  // thousands separator: dropped from the value
        } else {
            break;
        }
    }

    size_t fracDigits = 0;
    if (p < e && tag[p] == '.') {
        s.push_back('.');
        ++p;
        while (p < e && isDigit(tag[p])) {
            s.push_back(tag[p]);
            ++fracDigits;
            ++p;
        }
    }
    if (intDigits + fracDigits == 0) return false;  // not a number

    // VB takes D as well as E for the exponent.
    if (p < e && (lowerAscii(tag[p]) == 'e' || lowerAscii(tag[p]) == 'd')) {
        s.push_back('e');
        ++p;
        if (p < e && (tag[p] == '+' || tag[p] == '-')) {
            s.push_back(tag[p]);
            ++p;
        }
        size_t expDigits = 0;
        while (p < e && isDigit(tag[p])) {
            s.push_back(tag[p]);
            ++expDigits;
            ++p;
        }
        if (expDigits == 0) return false;
    }

    if (p != e) return false;  // trailing junk: Type mismatch

    const double v = std::strtod(s.c_str(), nullptr);
    if (!std::isfinite(v)) return false;  // past a Double

    const double r = std::nearbyint(v);  // half to even (default rounding mode)
    if (!fitsLong(r)) return false;      // Overflow
    out = static_cast<i64>(r);
    return true;
}

}  // namespace mordor
