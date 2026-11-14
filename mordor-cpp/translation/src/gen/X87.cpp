#include "core.h"

namespace mc {

X87::X87(cs::BigInteger m, int32_t e) { M = m; E = e; }

X87 X87::Of(double v)
{
    if (v == 0) return X87(cs::BigInteger(0), 0);
    int64_t bits = cs::BitConverter::DoubleToInt64Bits(v);
    int32_t exp = (int32_t)((bits >> 52) & 0x7FF);
    int64_t mant = bits & 0xFFFFFFFFFFFFFLL;
    if (exp == 0) exp = 1; else mant |= 1LL << 52;
    return X87(cs::BigInteger(v < 0 ? -mant : mant), exp - 1075);
}

X87 X87::Of(int64_t v) { return Round(cs::BigInteger(v), 0); }

X87 operator+(X87 a, X87 b)
{
    int32_t e = cs::Math::Min(a.E, b.E);
    return X87::Round((a.M << (a.E - e)) + (b.M << (b.E - e)), e);
}

X87 operator-(X87 a, X87 b) { return a + X87(-b.M, b.E); }

X87 operator*(X87 a, X87 b) { return X87::Round(a.M * b.M, a.E + b.E); }

X87 operator/(X87 a, X87 b)
{
    if (b.M.IsZero()) throw cs::DivideByZeroException();
    // enough quotient bits that the remainder decides the rounding exactly
    int32_t k = 66 + (int32_t)b.M.GetBitLength() - (int32_t)cs::BigInteger::Abs(a.M).GetBitLength();
    if (k < 0) k = 0;
    cs::BigInteger r;
    auto q = cs::BigInteger::DivRem(a.M << k, b.M, r);
    // a sticky bit for the remainder: below the 64 kept bits, so it only breaks ties
    q = q << 1;
    if (!r.IsZero()) q = q + cs::BigInteger((long long)(a.M.Sign() * b.M.Sign()));
    return X87::Round(q, a.E - b.E - k - 1);
}

X87 X87::Round(cs::BigInteger m, int32_t e)
{
    if (m.IsZero()) return X87(cs::BigInteger(0), 0);
    auto abs = cs::BigInteger::Abs(m);
    int32_t drop = (int32_t)abs.GetBitLength() - 64;
    if (drop <= 0) return X87(m, e);
    auto kept = abs >> drop;
    auto rest = abs - (kept << drop);
    auto half = cs::BigInteger::One() << (drop - 1);
    int32_t c = rest.CompareTo(half);
    if (c > 0 || (c == 0 && !kept.IsEven())) kept = kept + cs::BigInteger(1);
    return X87(m.Sign() < 0 ? -kept : kept, e + drop);
}

int64_t X87::RoundToLong()
{
    // C# (long)BigInteger throws OverflowException when out of range
    auto toLong = [](const cs::BigInteger& x) -> int64_t {
        if (x < cs::BigInteger((long long)INT64_MIN) || x > cs::BigInteger((long long)INT64_MAX)) throw cs::OverflowException();
        return (int64_t)(long long)x;
    };
    if (E >= 0) return toLong(M << E);
    auto abs = cs::BigInteger::Abs(M);
    auto whole = abs >> -E;
    auto rest = abs - (whole << -E);
    auto half = cs::BigInteger::One() << (-E - 1);
    int32_t c = rest.CompareTo(half);
    if (c > 0 || (c == 0 && !whole.IsEven())) whole = whole + cs::BigInteger(1);
    return toLong(M.Sign() < 0 ? -whole : whole);
}

}  // namespace mc
