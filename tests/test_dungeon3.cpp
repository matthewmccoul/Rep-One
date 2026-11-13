#include <cmath>

#include "test_support.h"
#include "mordor/dungeon2b.h" // FIXED: 2-argument rescueChance(i32, bool) is declared here; dungeon3.h only declares the 3-argument overload.
#include "mordor/dungeon3.h"

using namespace mordor;

// Helper: absolute-difference comparison for double results.
static bool near(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

