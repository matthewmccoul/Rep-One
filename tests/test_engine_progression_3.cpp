#include "test_support.h"
#include "mordor/progression1a.h"

// B_0065#111: m0 (neutral, blade) lands a blow that kills a one-hit monster.
// Party of two, not a companion strike, no owner index.
// The guild levels are not given in the case; 10/10 are placeholder inputs.
// The assertion only checks that the blow pays experience.
TEST(engine_B_0065_111_blow_on_one_hit_monster_pays_xp) {
    mordor::BlowAward award = mordor::blowExperience(1, 10, 10, false, 2, -1);
    CHECK(award.xp > 0);
}

// SKIP A_03#11: the case reads a binary character record at byte offset 36 and
// asserts the imported Age. The headers expose no record importer or byte-offset
// reader, so there is no equivalent function to call.
