#include <cmath>

#include "test_support.h"
#include "mordor/companions1b.h"

namespace mo = mordor;

namespace {
bool approx(double a, double b, double tol) { return std::fabs(a - b) <= tol; }
}  // namespace

