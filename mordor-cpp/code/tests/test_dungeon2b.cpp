#include "test_support.h"
#include "mordor/dungeon2b.h"

#include <cmath>

// Helper for floating-point comparisons in the rescue and disarm formulas.
static bool nearly(double a, double b) {
    return std::fabs(a - b) < 1e-9;
}

// R23 / c44: the preferred square-effect order places the pit before the
// stairs caption, the teleporter after the caption, and the chute last.
TEST(square_effect_order_follows_c44) {
    CHECK_EQ(mordor::kSquareEffectOrder[0], 0);
    CHECK_EQ(mordor::kSquareEffectOrder[5], 5);
    CHECK_EQ(mordor::kSquareEffectOrder[6], 6);
    CHECK_EQ(mordor::kSquareEffectOrder[7], 7);
    CHECK_EQ(mordor::kSquareEffectOrder[8], 8);
}

// Constants stated in the header for the rescue and build rules.
TEST(rescue_and_build_constants_match_srd) {
    CHECK_EQ(mordor::kRescueBaseLevel, 16);
    CHECK_EQ(mordor::kShareMonsterItemReach, 16);
    CHECK(nearly(mordor::kRescueCeilingChance, 100.0));
}
