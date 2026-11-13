// death2b.cpp: old-age death (R36) and party copy lookup (R37).
#include "mordor/death2b.h"

#include <cmath>

#include "mordor/ctx.h"
#include "mordor/events.h"
#include "mordor/rng.h"

namespace mordor {

namespace {
// UNKNOWN: year length in days. 365 is used because kAgeStartDays = 16 * 365.
inline constexpr double kDaysPerYear = 365.0;
}  // namespace

}  // namespace mordor
