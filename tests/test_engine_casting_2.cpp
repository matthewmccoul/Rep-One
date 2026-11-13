#include "test_support.h"
#include "mordor/casting2.h"

using namespace mordor;

// SKIP B_0061#34: needs GuildData.ByName lookup and the Nomad spell rate from MDATA2; no such lookup in the headers
// SKIP B_0061#35: SpellHelpProbe.Compare is a help-text vs. MDATA comparison probe; no equivalent in the headers
// SKIP B_0061#36: SpellHelpProbe.Compare is a help-text vs. MDATA comparison probe; no equivalent in the headers
// SKIP B_0061#37: SpellHelpProbe.Compare is a help-text vs. MDATA comparison probe; no equivalent in the headers
// SKIP B_0061#38: SpellHelpProbe.Compare is a help-text vs. MDATA comparison probe; no equivalent in the headers
// SKIP B_0061#42: needs GuildData.ByName("Wizard").SpellRate; GuildDef field names are not in the headers

TEST(engine_B_0061_43_wizard_level22_cannot_cast_teleport) {
    // Wizard at guild level 22, Teleport spell level 12, rate 1 -> cannot cast
    CHECK(!levelReachesSpell(22, 1.0, 12));
}

TEST(engine_B_0061_44_wizard_level23_can_cast_teleport) {
    // Wizard at guild level 23, Teleport spell level 12, rate 1 -> can cast
    CHECK(levelReachesSpell(23, 1.0, 12));
}

TEST(engine_B_0061_53_wizard_level26_spell_level_13) {
    CHECK_EQ(spellLevelFromGuildLevel(26), 13);
}

TEST(engine_B_0061_56_sorcerer_firebolt_cost_26) {
    // Sorcerer guild level 1, Fire rate 1, base level 1, spell point ratio 14
    CHECK_EQ(spellCostPublished(1, 14, 1.0f, 1), 26);
}
