#include "test_support.h"
#include "mordor/companions2a.h"

using namespace mordor;

// Case ids use '_' in place of '#' in test names (e.g. B_0064#42 -> B_0064_42).

// SKIP B_0059#75: fn1163 is a routine from the original MAINMOD2.BAS; no equivalent function in the new headers.

TEST(engine_B_0064_42_breath_guild1_four_bodies_mishap_chance) {
    CHECK_EQ(mishapChance(MishapKind::Breath, 1, 4), 40.0);
}

TEST(engine_B_0064_43_breath_guild999_four_bodies_mishap_chance) {
    CHECK_EQ(mishapChance(MishapKind::Breath, 999, 4), 2.0);
}

TEST(engine_B_0064_44_spit_guild1_four_bodies_mishap_chance) {
    CHECK_EQ(mishapChance(MishapKind::Spit, 1, 4), 20.0);
}

TEST(engine_B_0064_45_spit_guild9990_four_bodies_mishap_chance) {
    CHECK_EQ(mishapChance(MishapKind::Spit, 9990, 4), 0.5);
}

TEST(engine_B_0064_46_spell_guild999_four_bodies_mishap_chance) {
    CHECK_EQ(mishapChance(MishapKind::Spell, 999, 4), 1.0);
}

TEST(engine_B_0064_47_breath_guild100_five_bodies_mishap_range) {
    f64 v = mishapChance(MishapKind::Breath, 100, 5);
    // Expected: "Between 4.99 and 5.00." (bounds treated as inclusive)
    CHECK(v >= 4.99 && v <= 5.00);
}

TEST(engine_B_0064_48_breath_guild50_gt_guild200_five_bodies) {
    CHECK(mishapChance(MishapKind::Breath, 50, 5) > mishapChance(MishapKind::Breath, 200, 5));
}

TEST(engine_B_0064_49_breath_guild100_ten_bodies_gt_five_bodies) {
    CHECK(mishapChance(MishapKind::Breath, 100, 10) > mishapChance(MishapKind::Breath, 100, 5));
}

TEST(engine_B_0064_50_three_characters_no_companions_bodies) {
    CHECK_EQ(countBodies(3, 0), 3);
}
