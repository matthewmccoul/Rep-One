#include <cmath>

#include "test_support.h"
#include "mordor/loot1b.h"

namespace {

// Currency is stored in 1/10000 gold units (foundation note).
constexpr mordor::Currency kLootGold = 10000;

bool lootNear(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

}  // namespace

// NOT FIXED in this file: every "tsup::Env env({<double>})" below narrows
// double -> mordor::u64 because tsup::Env's constructor takes u64 elements.
// The fix belongs in test_support.h: Env must accept the double sequence
// (Rnd values in [0,1)). Changing the literals here would change test inputs,
// and test_support.h was not provided, so this file still fails to compile.

