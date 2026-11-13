#include "test_support.h"
#include "mordor/casting1.h"

#include <cmath>

// Tests derived from SRD c09p1 (R1-R38). Only header-declared functions are called.
// Functions whose rules are not in this SRD part (breath, monster spells, map wrap,
// effect ids, item-only levels) are intentionally not tested.

TEST(guild_spell_level_follows_formula_and_clamps) {
    // R21: SL = clamp(Int(guildLevel/2 + 0.5), 1, 255)
    CHECK_EQ(mordor::guildSpellLevel(0), 1);
    CHECK_EQ(mordor::guildSpellLevel(1), 1);
    CHECK_EQ(mordor::guildSpellLevel(2), 1);
    CHECK_EQ(mordor::guildSpellLevel(3), 2);
    CHECK_EQ(mordor::guildSpellLevel(4), 2);
    CHECK_EQ(mordor::guildSpellLevel(19), 10);
    CHECK_EQ(mordor::guildSpellLevel(20), 10);
    CHECK_EQ(mordor::guildSpellLevel(600), 255);
}

TEST(reach_teleport_example_needs_guild_level_23) {
    // R27 / R23: Teleport level 12, Wizard rate 1. GL >= 2*12 - 1 = 23.
    CHECK(!mordor::guildReachesSpell(12, 1.0f, 22));
    CHECK(mordor::guildReachesSpell(12, 1.0f, 23));
}

TEST(reach_requires_positive_rate_and_inclusive_boundary) {
    // R23: rate must be above 0. Charm of Opening (level 1) at rate 3:
    // GL 4 gives half = 2.5 < 3 (no); GL 5 gives half = 3.0 and 3 <= 3 (yes).
    CHECK(!mordor::guildReachesSpell(1, 3.0f, 4));
    CHECK(mordor::guildReachesSpell(1, 3.0f, 5));
    CHECK(!mordor::guildReachesSpell(1, 0.0f, 50));
}

