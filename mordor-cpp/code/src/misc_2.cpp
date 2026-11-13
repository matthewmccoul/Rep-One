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

}  // namespace

}  // namespace mordor
