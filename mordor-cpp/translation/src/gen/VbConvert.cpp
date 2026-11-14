#include "core.h"

namespace mc {

bool VbConvert::TryCLng(cs::String text, int32_t& value) {
    value = 0;
    cs::String s = cs::Str::Trim(text, " \t");
    if ((int32_t)s.size() == 0) return false;

    if (s[0] == '&') {
        int32_t radix = 8, at = 1;
        if ((int32_t)s.size() > 1 && (s[1] == 'H' || s[1] == 'h')) { radix = 16; at = 2; }
        else if ((int32_t)s.size() > 1 && (s[1] == 'O' || s[1] == 'o')) { at = 2; }
        if (at >= (int32_t)s.size()) return false;
        uint64_t n = 0;
        for (int32_t i = at; i < (int32_t)s.size(); i++) {
            int32_t d = HexDigit(s[i]);
            if (d < 0 || d >= radix) return false;
            n = n * (uint64_t)radix + (uint64_t)d;
            if (n > 0xFFFFFFFFULL) return false;
        }
        value = n <= 0xFFFF ? (int32_t)(int16_t)(uint16_t)n : (int32_t)(uint32_t)n;
        return true;
    }

    // the D exponent is VB's double-precision spelling of E
    cs::String e = cs::Str::Replace(cs::Str::Replace(s, 'D', 'E'), 'd', 'E');
    double v = 0;
    // double.TryParse(e, Styles, CultureInfo.CurrentCulture, out v)
    if (!cs::Parse::TryParse(e, v)) return false;
    v = cs::Math::Round(v, cs::MidpointRounding::ToEven);
    if (std::isnan(v) || v < (double)INT32_MIN || v > (double)INT32_MAX) return false;
    value = cs::CastDouble<int32_t>(v);
    return true;
}

int32_t VbConvert::HexDigit(char c) {
    return c >= '0' && c <= '9' ? c - '0'
        : c >= 'A' && c <= 'F' ? c - 'A' + 10
        : c >= 'a' && c <= 'f' ? c - 'a' + 10
        : -1;
}

}  // namespace mc
