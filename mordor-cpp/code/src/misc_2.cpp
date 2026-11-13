// misc_2.cpp - Chapter 22 miscellaneous rules (part c22p1), part 2.
// Covers: currency conversion (R27-R28), X87 extended precision (R29),
// WordSort (R31), and quest gating (R32-R36).
// Part 1 (Asmodeus, teleporter, chute, Hall, power, skills, pluralisation) is in misc.cpp.
#include <cmath>

#include "mordor/misc.h"
#include "mordor/math_util.h"

namespace mordor {
namespace {

// 128-bit unsigned helper for X87 window arithmetic.
struct U128 {
    u64 hi;
    u64 lo;
};

U128 u128Shl(U128 x, i32 n) {
    if (n <= 0) return x;
    if (n >= 128) return U128{0, 0};
    if (n >= 64) return U128{x.lo << (n - 64), 0};
    return U128{(x.hi << n) | (x.lo >> (64 - n)), x.lo << n};
}

U128 u128Shr(U128 x, i32 n) {
    if (n <= 0) return x;
    if (n >= 128) return U128{0, 0};
    if (n >= 64) return U128{0, x.hi >> (n - 64)};
    return U128{x.hi >> n, (x.lo >> n) | (x.hi << (64 - n))};
}

// Returns true on carry out of bit 127.
bool u128Add(U128 a, U128 b, U128& out) {
    u64 lo = a.lo + b.lo;
    u64 carryLo = lo < a.lo ? 1 : 0;
    u64 hi = a.hi + b.hi;
    bool over = hi < a.hi;
    u64 hi2 = hi + carryLo;
    if (hi2 < hi) over = true;
    out = U128{hi2, lo};
    return over;
}

// Requires a >= b.
U128 u128Sub(U128 a, U128 b) {
    u64 borrow = a.lo < b.lo ? 1 : 0;
    return U128{a.hi - b.hi - borrow, a.lo - b.lo};
}

i32 u128Cmp(U128 a, U128 b) {
    if (a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
    if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
    return 0;
}

// Index of the highest set bit, or -1 for zero.
i32 topBit(U128 x) {
    for (i32 i = 127; i >= 0; --i) {
        u64 w = i >= 64 ? x.hi : x.lo;
        i32 b = i >= 64 ? i - 64 : i;
        if (((w >> b) & 1u) != 0) return i;
    }
    return -1;
}

// 64x64 -> 128 product.
U128 mul64(u64 a, u64 b) {
    u64 aL = a & 0xffffffffu, aH = a >> 32;
    u64 bL = b & 0xffffffffu, bH = b >> 32;
    u64 ll = aL * bL;
    u64 lh = aL * bH;
    u64 hl = aH * bL;
    u64 hh = aH * bH;
    u64 mid = (ll >> 32) + (lh & 0xffffffffu) + (hl & 0xffffffffu);
    U128 r{0, 0};
    r.lo = (mid << 32) | (ll & 0xffffffffu);
    r.hi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
    return r;
}

// Rounds value = r * 2^exp (plus a sticky fraction below r) to a 64-bit mantissa,
// half to even. Each X87 operation rounds this way.
X87Value normalizeRound(bool negative, i64 exp, U128 r, bool sticky) {
    i32 p = topBit(r);
    if (p < 0) return X87Value{0, 0, false};
    i32 k = p - 63;
    if (k <= 0) {
        // Left shift is exact; sticky cannot be set on this path.
        U128 m = u128Shl(r, -k);
        return X87Value{m.lo, exp + k, negative};
    }
    U128 mant = u128Shr(r, k);
    U128 rem = u128Sub(r, u128Shl(mant, k));
    U128 half = u128Shl(U128{0, 1}, k - 1);
    i32 c = u128Cmp(rem, half);
    bool up = c > 0 || (c == 0 && (sticky || (mant.lo & 1u) != 0));
    i64 e = exp + k;
    if (up) {
        mant.lo += 1;
        if (mant.lo == 0) {  // carried out of 64 bits
            mant.lo = u64{1} << 63;
            e += 1;
        }
    }
    return X87Value{mant.lo, e, negative};
}

// One long-division step: r < mb on entry. Returns the quotient bit.
bool divStep(u64& r, u64 mb) {
    bool carry = (r >> 63) != 0;
    r <<= 1;
    if (carry || r >= mb) {
        r -= mb;  // wrapping subtraction is exact here
        return true;
    }
    return false;
}

// Signed result from an unsigned magnitude, saturating at the i64 range.
i64 signedOrSaturate(u64 mag, bool negative) {
    const u64 kMaxMag = 9223372036854775807ULL;
    if (negative) {
        if (mag > kMaxMag) return -9223372036854775807LL - 1;
        return -static_cast<i64>(mag);
    }
    if (mag > kMaxMag) return 9223372036854775807LL;
    return static_cast<i64>(mag);
}

bool isAsciiAlnum(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

char lowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

u32 skipToAlnum(const Str& s, u32 i) {
    u32 n = static_cast<u32>(s.size());
    while (i < n && !isAsciiAlnum(s[i])) ++i;
    return i;
}

}  // namespace

// R28 [c22]: FromDouble(d) = round_half_even(d * 10000); overflow if NaN or outside +/-2^63.
CurrencyFromDoubleResult currencyFromDoubleChecked(double value) {
    CurrencyFromDoubleResult res{};
    double d = value * 10000.0;
    const double two63 = 9223372036854775808.0;
    if (d != d || d >= two63 || d < -two63) {
        res.overflow = true;
        return res;
    }
    res.value = roundHalfEven(d);
    return res;
}

// R29 [c22]: value = M * 2^E with a 64-bit mantissa.
// UNKNOWN: NaN and infinity are not representable here; they map to zero.
X87Value x87FromDouble(double value) {
    if (value == 0.0 || value != value || !(std::fabs(value) <= 1.7976931348623157e308)) {
        return X87Value{0, 0, false};
    }
    int e = 0;
    double f = std::frexp(std::fabs(value), &e);  // |value| = f * 2^e, f in [0.5, 1)
    u64 m = static_cast<u64>(std::ldexp(f, 64));  // exact: f has at most 53 bits
    return X87Value{m, static_cast<i64>(e) - 64, value < 0.0};
}

// R29: addition rounded to 64 bits, half to even.
X87Value x87Add(const X87Value& a, const X87Value& b) {
    if (a.mantissa == 0) return b;
    if (b.mantissa == 0) return a;
    bool aBig = a.exponent > b.exponent ||
                (a.exponent == b.exponent && a.mantissa >= b.mantissa);
    const X87Value& big = aBig ? a : b;
    const X87Value& small = aBig ? b : a;
    i64 d = big.exponent - small.exponent;  // >= 0
    // Window unit is 2^(big.exponent - 64). Big mantissa sits in the high word.
    U128 bigW{big.mantissa, 0};
    U128 smallW{0, 0};
    bool sticky = false;
    if (d <= 64) {
        smallW = u128Shl(U128{0, small.mantissa}, static_cast<i32>(64 - d));
    } else {
        i64 t = d - 64;
        if (t >= 64) {
            sticky = small.mantissa != 0;
        } else {
            smallW = U128{0, small.mantissa >> t};
            sticky = (small.mantissa & ((u64{1} << t) - 1)) != 0;
        }
    }
    bool negative = big.negative;
    i64 exp = big.exponent - 64;
    U128 r{0, 0};
    if (big.negative == small.negative) {
        U128 sum{0, 0};
        bool carry = u128Add(bigW, smallW, sum);
        if (carry) {  // 129-bit result: shift right one, keep the lost bit as sticky
            sticky = sticky || ((sum.lo & 1u) != 0);
            r = U128{(u64{1} << 63) | (sum.hi >> 1), (sum.hi << 63) | (sum.lo >> 1)};
            exp += 1;
        } else {
            r = sum;
        }
    } else {
        U128 diff = u128Sub(bigW, smallW);
        if (sticky) diff = u128Sub(diff, U128{0, 1});  // B - (S + f) = (B - S - 1) + (1 - f)
        r = diff;
    }
    return normalizeRound(negative, exp, r, sticky);
}

// R29: multiplication rounded to 64 bits, half to even.
X87Value x87Mul(const X87Value& a, const X87Value& b) {
    if (a.mantissa == 0 || b.mantissa == 0) return X87Value{0, 0, false};
    U128 p = mul64(a.mantissa, b.mantissa);
    return normalizeRound(a.negative != b.negative, a.exponent + b.exponent, p, false);
}

// R29: division rounded to 64 bits, half to even.
// UNKNOWN: division by zero (VB raises an error); returns zero here.
X87Value x87Div(const X87Value& a, const X87Value& b) {
    if (a.mantissa == 0 || b.mantissa == 0) return X87Value{0, 0, false};
    u64 ma = a.mantissa;
    u64 mb = b.mantissa;
    bool ip = ma >= mb;  // quotient in (0.5, 2)
    u64 r = ip ? ma - mb : ma;
    u64 q = ip ? 1 : 0;
    i32 fracBits = ip ? 63 : 64;
    for (i32 i = 0; i < fracBits; ++i) {
        q = (q << 1) | (divStep(r, mb) ? 1u : 0u);
    }
    bool guard = divStep(r, mb);
    bool sticky = r != 0;
    i64 exp = a.exponent - b.exponent - (ip ? 63 : 64);
    if (guard && (sticky || (q & 1u) != 0)) {
        q += 1;
        if (q == 0) {
            q = u64{1} << 63;
            exp += 1;
        }
    }
    return X87Value{q, exp, a.negative != b.negative};
}

// R29 [c22]: storing to Long rounds to nearest, half to even.
// Saturates at the Long range; the call-site overflow policy is the caller's choice.
i64 x87StoreToLong(const X87Value& v) {
    if (v.mantissa == 0) return 0;
    if (v.exponent >= 0) {
        if (v.exponent == 0) return signedOrSaturate(v.mantissa, v.negative);
        if (v.exponent >= 64 || (v.mantissa >> (63 - v.exponent)) != 0) {
            return signedOrSaturate(~u64{0}, v.negative);
        }
        return signedOrSaturate(v.mantissa << v.exponent, v.negative);
    }
    i64 shift = -v.exponent;
    if (shift > 64) return 0;  // magnitude below one half
    u64 ip = 0;
    u64 rem = v.mantissa;
    if (shift < 64) {
        ip = v.mantissa >> shift;
        rem = v.mantissa & ((u64{1} << shift) - 1);
    }
    u64 half = u64{1} << (shift - 1);
    bool up = rem > half || (rem == half && (ip & 1u) != 0);
    u64 mag = ip + (up ? 1u : 0u);
    return signedOrSaturate(mag, v.negative);
}

// R31 [c58]: WordSort. Ignores non-alphanumeric characters (spaces, hyphens,
// apostrophes) and compares case-insensitively, matching the documented examples
// (Archer < Arch-Fiend < Archmage; Dragons < Dragon's Blood; Tarantula < T'Kovant).
i32 wordSortCompare(const Str& a, const Str& b) {
    u32 i = 0;
    u32 j = 0;
    u32 na = static_cast<u32>(a.size());
    u32 nb = static_cast<u32>(b.size());
    while (true) {
        i = skipToAlnum(a, i);
        j = skipToAlnum(b, j);
        bool endA = i >= na;
        bool endB = j >= nb;
        if (endA || endB) {
            if (endA && endB) return 0;
            return endA ? -1 : 1;
        }
        char ca = lowerAscii(a[i]);
        char cb = lowerAscii(b[j]);
        if (ca != cb) return ca < cb ? -1 : 1;
        ++i;
        ++j;
    }
}

// R32 [c91]: monster quests from gLevel 5; item quests from gLevel 15.
// UNKNOWN: Companion quests have no stated gate; treated as ineligible.
bool questKindEligible(QuestKind kind, i32 guildLevel) {
    if (kind == QuestKind::Defeat) return guildLevel >= kQuestMonsterMinGuildLevel;
    if (kind == QuestKind::Fetch) return guildLevel >= kQuestItemMinGuildLevel;
    return false;
}

// R33 [c91]: mMax = min(floor(gLevel/25), 10).
i32 questMonsterLevelCap(i32 guildLevel) {
    i64 v = vbInt(static_cast<double>(guildLevel) / kQuestGuildLevelDivisor);
    return v < kQuestMonsterLevelCap ? static_cast<i32>(v) : kQuestMonsterLevelCap;
}

// R33 [c91]: iMax = min(7, (floor(min(gLevel/25 floored, 10)) / 1.5) + 1).
// UNKNOWN: whether the division result is floored before the min. Here the final
// value is floored to an integer level, which is the simplest integer reading.
i32 questItemLevelCap(i32 guildLevel) {
    double base = static_cast<double>(questMonsterLevelCap(guildLevel));
    double v = base / 1.5 + 1.0;
    i64 f = vbInt(v);
    return f < kQuestItemLevelCap ? static_cast<i32>(f) : kQuestItemLevelCap;
}

// R33: level drawn from 1 to the cap, inclusive.
// UNKNOWN: a cap below 1 yields 1.
i32 drawQuestLevel(Ctx& ctx, i32 levelCap) {
    if (levelCap < 1) return 1;
    return ctx.rng.below(levelCap) + 1;
}

// R34 [c91]: rare monster has spawn weight <= 9.
bool isRareQuestMonster(i32 spawnWeight) {
    return spawnWeight <= kQuestRareMonsterMaxWeight;
}

// R34 [c91]: rare item has spawn weight <= 4.
bool isRareQuestItem(i32 spawnWeight) {
    return spawnWeight <= kQuestRareItemMaxWeight;
}

// R34 [c91]: rare entries are rerolled 74% of the time.
bool rareQuestReroll(Ctx& ctx) {
    return ctx.rng.percent(kQuestRareRerollPercent);
}

// R35 [c91]: cursed items, iLevel 0 items, and items worth under 100 gold are never assigned.
bool questItemExcluded(const Item& item, i32 itemLevel, Currency itemValue) {
    return item.cursed != CurseNone || itemLevel == 0 || itemValue < kQuestMinItemValue;
}

// R36 [c91]: how the 1%, 6% and 25% values combine is UNKNOWN. Chosen order:
// 6% no quest first; then, when item quests are eligible, 25% item quest; else a
// monster quest when eligible. The 1% minimum assignment chance is not applied.
QuestKind rollQuestKind(Ctx& ctx, i32 guildLevel) {
    if (ctx.rng.percent(kQuestNoQuestPercent)) return QuestKind::None;
    if (guildLevel >= kQuestItemMinGuildLevel &&
        ctx.rng.percent(kQuestItemFrequencyPercent)) {
        return QuestKind::Fetch;
    }
    if (guildLevel >= kQuestMonsterMinGuildLevel) return QuestKind::Defeat;
    return QuestKind::None;
}

}  // namespace mordor
