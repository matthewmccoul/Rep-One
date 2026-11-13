#include <cmath>
#include "mordor/spells1a.h"

#include "test_support.h"
#include "mordor/companions1a.h"

namespace mr = mordor;

namespace {

bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

mr::Companion filledSlot(mr::i32 hits) {
    mr::Companion c;
    c.ref = 5;
    c.occupied = true;
    c.hits = hits;
    c.maxHits = 12;
    return c;
}

mr::Companion deadSlot() {
    mr::Companion c = filledSlot(0);
    c.isDead = true;
    c.deathState = mr::CompanionDeathState::Dead;
    return c;
}

}  // namespace

TEST(binding_predicates) {
    mr::Companion c;
    c.bindingLevel = 0;
    CHECK(!mr::companionBound(c));
    c.bindingLevel = mr::kFreeWillBinding;
    CHECK(!mr::companionBound(c));
    c.bindingLevel = 1;
    CHECK(mr::companionBound(c));
    c.bindingLevel = 99;
    CHECK(!mr::companionForLifeBinding(c));
    c.bindingLevel = 100;
    CHECK(mr::companionForLifeBinding(c));
    CHECK(mr::companionBound(c));
}

TEST(binding_constants_match_srd) {
    CHECK_EQ(mr::kBindingBase, 5);
    CHECK_EQ(mr::kStartBindingCap, 10);
    CHECK_EQ(mr::kBoughtBindingCap, 10);
    CHECK_EQ(mr::kBindingStep, 4);
    CHECK_EQ(mr::kBindingPurchaseMax, 12);
    CHECK_EQ(mr::kBindingCostConst, 1237);
    CHECK_EQ(mr::kBindingCostDivisor, 4);
    CHECK_EQ(mr::kFreeWillBinding, -10);
    CHECK_EQ(mr::kBindingMax, 100);
    CHECK_EQ(mr::kJoinRollMax, 3000);
    CHECK_EQ(mr::kJoinBarChaScale, 10);
    CHECK_EQ(mr::kSubtypeResetTries, 150);
    CHECK_EQ(mr::kSubtypeFailTries, 300);
    CHECK_EQ(mr::kBreathChanceStrong, 80);
    CHECK_EQ(mr::kBreathChanceWeak, 20);
    CHECK_EQ(mr::kSpitRetryCap, 1000);
    CHECK(near(mr::kMishapRows[0].factor, 2.0));
    CHECK(near(mr::kMishapRows[2].high, 20.0));
}

