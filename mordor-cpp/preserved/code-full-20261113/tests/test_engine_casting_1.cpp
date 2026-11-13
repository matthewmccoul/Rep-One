#include "test_support.h"

#include "mordor/casting2.h"

// SKIP B_0056#5: needs CombatEngine.CastSpell and a 300-run combat simulation; no equivalent in the headers.
// SKIP B_0056#34: needs GuildData.All and MDATA2 spell tables; no equivalent in the headers.
// SKIP B_0058#30: asks whether the port's DungeonWindow.axaml.cs orders RNG draws; source inspection, not a unit test.
// SKIP B_0058#31: asks about DungeonWindow.Mdi.cs UI gesture wiring; needs UI, not a unit test.
// SKIP B_0058#32: asks about DungeonWindow.Mdi.cs class-box refresh behaviour; needs UI, not a unit test.
// SKIP B_0058#33: asks about DungeonWindow.Mdi.cs reopen behaviour; needs UI, not a unit test.

// B_0061#29: Spellbook.MaxSpellPoints for the Nomad. The header signature takes only
// naturalInt and naturalWis, so race and alignment are not inputs here.
TEST(engine_B_0061_29_max_spell_points_nomad) {
    CHECK_EQ(mordor::maxSpellPoints(12, 12), 120);
}

// SKIP B_0061#30: needs CastGateProbe.Compare and MDATA2 help labels; no equivalent in the headers.
// SKIP B_0061#31: needs CastGateProbe.Compare agreement count; no equivalent in the headers.
// SKIP B_0061#33: needs GuildData.ByName and the MDATA2 rate for Charm of Opening; shownSpellLevel needs a rate that the case does not provide.
