#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct X87 {
    cs::BigInteger M;
    int32_t E = 0;

    X87() = default;
    X87(cs::BigInteger m, int32_t e);

    static X87 Of(double v);
    static X87 Of(int64_t v);

    friend X87 operator+(X87 a, X87 b);
    friend X87 operator-(X87 a, X87 b);
    friend X87 operator*(X87 a, X87 b);
    friend X87 operator/(X87 a, X87 b);

    static X87 Round(cs::BigInteger m, int32_t e);
    int64_t RoundToLong();
};

X87 operator+(X87 a, X87 b);
X87 operator-(X87 a, X87 b);
X87 operator*(X87 a, X87 b);
X87 operator/(X87 a, X87 b);

}  // namespace mc
