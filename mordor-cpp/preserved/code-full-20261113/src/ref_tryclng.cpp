// ref: src/Mordor.Core/VbConvert.cs:22 (VbConvert.TryCLng)
#include "mordor/ref_tryclng.h"

#include <cstddef>
#include <cstdlib>
#include <string>

#include "mordor/math_util.h"  // ASSUMES double roundHalfEven(double) = SRD round_half_even

namespace mordor {
namespace {

// Half-open span [begin, end) of the text after trimming.
struct Span {
    std::size_t begin;
    std::size_t end;
};

// Reference: text.Trim(' ', '\t')
bool isTrimChar(char c) { return c == ' ' || c == '\t'; }

// UNKNOWN: .NET NumberStyles.AllowLeadingWhite/AllowTrailingWhite accept 0x09..0x0D and 0x20.
bool isNetWhite(char c) { return c == ' ' || (c >= '\t' && c <= '\r'); }

bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Reference HexDigit: value of a hex digit, or -1.
i32 hexDigit(char c) {
    if (c >= '0' && c <= '9') return static_cast<i32>(c - '0');
    if (c >= 'A' && c <= 'F') return static_cast<i32>(c - 'A') + 10;
    if (c >= 'a' && c <= 'f') return static_cast<i32>(c - 'a') + 10;
    return -1;
}

Span trimmed(const Str& text) {
    std::size_t b = 0;
    std::size_t e = text.size();
    while (b < e && isTrimChar(text[b])) ++b;
    while (e > b && isTrimChar(text[e - 1])) --e;
    return Span{b, e};
}

// Reference branch for text starting with '&': &H = hex, &O = octal, bare & = octal.
// Wraps like VB: (short)(ushort)n when n <= 0xFFFF, otherwise (int)(uint)n.
bool parseAmpersand(const Str& text, Span sp, i32& value) {
    i32 radix = 8;
    std::size_t at = sp.begin + 1;
    if (sp.end - sp.begin > 1 && (text[sp.begin + 1] == 'H' || text[sp.begin + 1] == 'h')) {
        radix = 16;
        at = sp.begin + 2;
    } else if (sp.end - sp.begin > 1 && (text[sp.begin + 1] == 'O' || text[sp.begin + 1] == 'o')) {
        at = sp.begin + 2;
    }
    if (at >= sp.end) return false;

    u64 n = 0;
    for (std::size_t i = at; i < sp.end; ++i) {
        i32 d = hexDigit(text[i]);
        if (d < 0 || d >= radix) return false;
        n = n * static_cast<u64>(radix) + static_cast<u64>(d);
        if (n > 0xFFFFFFFFull) return false;
    }

    if (n <= 0xFFFFull) {
        // (short)(ushort)n: 0x8000..0xFFFF wrap to negative
        value = (n >= 0x8000ull) ? static_cast<i32>(n) - 0x10000 : static_cast<i32>(n);
    } else {
        // (int)(uint)n: values above INT32_MAX wrap to negative (two's complement)
        i64 w = static_cast<i64>(n);
        if (w > 0x7FFFFFFFLL) w -= 0x100000000LL;
        value = static_cast<i32>(w);
    }
    return true;
}

// Builds a clean ASCII number for strtod from the decimal branch's grammar:
//   [white] [sign] digits [',' digits]* ['.' digits] [(E|e|D|d) [sign] digits] [white]
// The reference's Replace('D','E') is equivalent here because D is only valid as an exponent letter.
// UNKNOWN: CurrentCulture is not stated; assumed invariant ('.' decimal point, ',' group separator).
// UNKNOWN: AllowCurrencySymbol is accepted by the reference; the culture's symbol is not implemented, so "$5" is refused.
// UNKNOWN: .NET group-separator placement is not validated; here a ',' is accepted only between integer digits.
// Infinity/NaN spellings are refused here; the reference also refuses them via its range checks.
bool cleanDecimal(const Str& text, Span sp, std::string& out) {
    std::size_t i = sp.begin;
    std::size_t end = sp.end;
    while (i < end && isNetWhite(text[i])) ++i;
    while (end > i && isNetWhite(text[end - 1])) --end;

    if (i < end && (text[i] == '+' || text[i] == '-')) {
        out.push_back(text[i]);
        ++i;
    }

    std::size_t intDigits = 0;
    std::size_t fracDigits = 0;
    while (i < end) {
        char c = text[i];
        if (isDigit(c)) {
            out.push_back(c);
            ++intDigits;
            ++i;
        } else if (c == ',' && intDigits > 0 && i + 1 < end && isDigit(text[i + 1])) {
            ++i;  // group separator, not copied
        } else {
            break;
        }
    }

    if (i < end && text[i] == '.') {
        out.push_back('.');
        ++i;
        while (i < end && isDigit(text[i])) {
            out.push_back(text[i]);
            ++fracDigits;
            ++i;
        }
    }
    if (intDigits + fracDigits == 0) return false;

    if (i < end && (text[i] == 'E' || text[i] == 'e' || text[i] == 'D' || text[i] == 'd')) {
        out.push_back('e');
        ++i;
        if (i < end && (text[i] == '+' || text[i] == '-')) {
            out.push_back(text[i]);
            ++i;
        }
        std::size_t expDigits = 0;
        while (i < end && isDigit(text[i])) {
            out.push_back(text[i]);
            ++expDigits;
            ++i;
        }
        if (expDigits == 0) return false;
    }

    return i == end;
}

}  // namespace

bool tryClng(const Str& text, i32& value) {
    value = 0;
    Span sp = trimmed(text);
    if (sp.begin == sp.end) return false;

    if (text[sp.begin] == '&') return parseAmpersand(text, sp, value);

    std::string clean;
    if (!cleanDecimal(text, sp, clean)) return false;

    double v = std::strtod(clean.c_str(), nullptr);
    // Any magnitude this large is out of Long range after rounding; refuse early (also covers inf).
    // NaN fails both comparisons below, so it is refused too.
    if (!(v <= 4.0e9 && v >= -4.0e9)) return false;

    v = roundHalfEven(v);  // SRD round_half_even: half to even
    if (!(v >= -2147483648.0 && v <= 2147483647.0)) return false;

    value = static_cast<i32>(v);
    return true;
}

}  // namespace mordor
