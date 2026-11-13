#include <cmath>

#include "test_support.h"
#include "mordor/status1a.h"

using namespace mordor;

namespace {

// Character with base CON set (stats index 3 = Con) and no gear CON.
Character makeVictim(i32 con) {
    Character c;
    c.abilities.stats[3] = con;
    c.abilities.gearCon = 0;
    return c;
}

bool closeTo(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

}  // namespace

