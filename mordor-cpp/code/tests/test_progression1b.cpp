#include <cmath>

#include "test_support.h"
#include "mordor/progression1b.h"

using namespace mordor;

namespace {
bool nearVal(double a, double b, double tol) { return std::fabs(a - b) <= tol; }
}  // namespace

// R19: starting age is 16 years in days.
TEST(start_age_is_16_years_in_days) {
    CHECK_EQ(kAgeStartDays, 5840);
    CHECK_EQ(kAgeStartDays / 365, 16);
}

