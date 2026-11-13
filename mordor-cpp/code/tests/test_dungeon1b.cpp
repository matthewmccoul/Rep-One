#include "test_support.h"
#include "mordor/dungeon1b.h"

#include <cmath>

using namespace mordor;

static bool nearly(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

