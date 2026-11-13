// Arithmetic helpers that reproduce Visual Basic semantics used by the original rules.
#pragma once
#include <cmath>
#include "mordor/types.h"

namespace mordor {

// VB Int(x): the largest integer <= x (floor), not truncation.
inline i64 vbInt(double x) { return static_cast<i64>(std::floor(x)); }

// VB Fix(x): truncation toward zero.
inline i64 vbFix(double x) { return static_cast<i64>(x < 0 ? std::ceil(x) : std::floor(x)); }

// VB CLng/CInt and the "round_half_even" of the SRD: nearest integer, halves go to the even number.
inline i64 roundHalfEven(double x) {
  double f = std::floor(x);
  double d = x - f;
  if (d < 0.5) return static_cast<i64>(f);
  if (d > 0.5) return static_cast<i64>(f) + 1;
  i64 lo = static_cast<i64>(f);
  return (lo % 2 == 0) ? lo : lo + 1;
}

template <class T> inline T clampv(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline i32 minv(i32 a, i32 b) { return a < b ? a : b; }
inline i32 maxv(i32 a, i32 b) { return a > b ? a : b; }

// Currency <-> whole gold (whole part, truncated toward zero).
inline i64 currencyWhole(Currency c) { return c / kCurrencyScale; }
inline Currency currencyFromWhole(i64 gold) { return gold * kCurrencyScale; }

}  // namespace mordor
