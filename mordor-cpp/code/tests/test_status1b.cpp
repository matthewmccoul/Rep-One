#include <cmath>

#include "test_support.h"
#include "mordor/status1b.h"

using namespace mordor;

static bool nearly(double a, double b, double tol = 1e-3) {
    return std::fabs(a - b) < tol;
}

TEST(chest_trap_count_is_ten) {
    CHECK_EQ(kChestTrapCount, 10);
    CHECK_EQ(static_cast<i32>(ChestTrap::NotTrapped), 0);
    CHECK_EQ(static_cast<i32>(ChestTrap::Trap10), 10);
}

TEST(cure_thresholds_and_disease_classes) {
    CHECK_EQ(kCureControlLevel, 3);
    CHECK_EQ(kCureParalysisLevel, 4);
    CHECK_EQ(kCureDiseaseLevel, 5);
    CHECK_EQ(kCureLevel, 8);
    CHECK(kCureClass == Guild::Healer);
    // Cure Disease is taught to Pal/Mag/Hea only; Villain is excluded.
    for (u32 i = 0; i < 3; ++i) {
        CHECK(kCureDiseaseClasses[i] != Guild::Villain);
    }
}

