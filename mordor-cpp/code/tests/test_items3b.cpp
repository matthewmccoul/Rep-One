#include <cmath>

#include "test_support.h"
#include "mordor/items3b.h"

using namespace mordor;

namespace {

bool approxEq(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

}  // namespace

