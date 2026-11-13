#include "test_support.h"
#include "mordor/companions2a.h"

using namespace mordor;

namespace {
// Tolerance comparison for f64 results.
bool nearlyEq(f64 a, f64 b, f64 tol = 1e-6) {
    f64 d = a - b;
    if (d < 0) d = -d;
    return d <= tol;
}
}  // namespace

// Binding state predicates and the joiner binding (R7, R11).
TEST(binding_predicates_and_joiner_value) {
    Companion c;
    c.bindingLevel = 1;
    CHECK(isBound(c));
    CHECK(!isContent(c));
    CHECK(!isForLife(c));

    c.bindingLevel = 0;
    CHECK(!isBound(c));
    CHECK(isContent(c));

    c.bindingLevel = kBindingMax;
    CHECK(isBound(c));
    CHECK(isForLife(c));

    c.bindingLevel = -10;
    CHECK(isContent(c));
    CHECK_EQ(joinerBinding(), -10);
}

// Mishap clamp table: Breath 2.0 (2 to 40), Spell 1.5 (1 to 40), Spit 1.0 (0.5 to 20).
TEST(mishap_range_table_matches_srd) {
    CHECK(nearlyEq(kMishapRanges[0].factor, 2.0));
    CHECK(nearlyEq(kMishapRanges[0].low, 2.0));
    CHECK(nearlyEq(kMishapRanges[0].high, 40.0));

    CHECK(nearlyEq(kMishapRanges[1].factor, 1.5));
    CHECK(nearlyEq(kMishapRanges[1].low, 1.0));
    CHECK(nearlyEq(kMishapRanges[1].high, 40.0));

    CHECK(nearlyEq(kMishapRanges[2].factor, 1.0));
    CHECK(nearlyEq(kMishapRanges[2].low, 0.5));
    CHECK(nearlyEq(kMishapRanges[2].high, 20.0));
}

