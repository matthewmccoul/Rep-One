#include "mordor/ref_fn1485.h"

#include <cmath>
#include <cstring>

namespace mordor {
namespace {

// Round half to even (SRD round_half_even), for a double, returning i64.
i64 roundHalfEvenLocal(double x) {
    double f = std::floor(x);
    double d = x - f;
    i64 fi = static_cast<i64>(f);
    if (d > 0.5) return fi + 1;
    if (d < 0.5) return fi;
    return (fi % 2 == 0) ? fi : fi + 1;
}

// 2^k as i64 for the shifts used here.
// UNKNOWN: the reference uses BigInteger; this port uses i64 and is exact while the
// intermediate product fits. Shipped spell rates keep |exp| small, so this holds in practice.
i64 pow2(i32 k) {
    return static_cast<i64>(1) << k;
}

}  // namespace

// Reference: src/Mordor.Core/Rules.cs:604 (Fn1485Price).
i32 fn1485Price(i32 level, i32 u4, f32 rate, i32 sl, i32& rated) {
    f32 r = (rate == 0.0f) ? 1.0f : rate;                               // 2886-2889
    double prod = static_cast<double>(level) * static_cast<double>(r);  // exact in double
    i64 ratedL = roundHalfEvenLocal(prod);                              // 2890
    rated = static_cast<i32>(ratedL);

    // The Single rate as mant * 2^exp, exactly.
    u32 bits = 0;
    std::memcpy(&bits, &r, sizeof bits);
    i32 e = static_cast<i32>((bits >> 23) & 0xFFu);
    i64 mant = static_cast<i64>(bits & 0x7FFFFFu);
    if (e != 0) {
        mant |= 0x800000;
    } else {
        e = 1;  // denormal
    }
    bool negative = (bits >> 31) != 0;
    i32 exp = e - 127 - 23;

    // Strip trailing zero bits: same value, smaller operands.
    while (mant != 0 && (mant & 1) == 0) {
        mant >>= 1;
        exp += 1;
    }
    if (negative) mant = -mant;

    i64 num = ratedL * static_cast<i64>(u4) * mant * 19;
    i64 den = 10 * static_cast<i64>(sl < 1 ? 1 : sl);
    if (exp >= 0) {
        num *= pow2(exp);
    } else {
        den *= pow2(-exp);
    }

    // C++ integer division truncates toward zero, as BigInteger.DivRem does.
    i64 q = num / den;
    i64 rem = num % den;
    if (rem < 0) {
        q -= 1;  // a floor, not a truncation
    } else if (rem == 0 && num > 0) {
        // Whole: the true product is just under it, so the page floors one lower.
        q -= 1;
    }
    return static_cast<i32>(q);
}

}  // namespace mordor
