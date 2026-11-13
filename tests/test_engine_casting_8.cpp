#include "test_support.h"
#include "mordor/casting2.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0062#42: MdrLoader.LoadSpells has no equivalent in the provided headers (no loader API).
// SKIP B_0062#43: Character.Create and Spellbook.Known have no equivalent in the provided headers.
// SKIP B_0062#44: Character.Create and Spellbook.Known have no equivalent in the provided headers.
// SKIP B_0062#45: Character.Create and Spellbook.Known have no equivalent in the provided headers.
// SKIP B_0062#46: Character.Create and Spellbook.Known have no equivalent in the provided headers.
// SKIP B_0062#59: needs MdrLoader.LoadSpells on MDATA2.MDR; no loader API in the provided headers.
// SKIP B_0062#60: needs MdrLoader.LoadItems and LoadSpells on MDATA3.MDR; no loader API in the provided headers.

TEST(engine_B_0062_61_highest_spell_ceiling_across_guilds) {
    // Highest spell ceiling any guild reaches in any school.
    CHECK_EQ(spellCeilingAcrossGuilds(tsup::data().guilds), 15);
}

// SKIP B_0062#62: needs Character.PutInPack and Spellbook.Castables; no equivalent in the provided headers.
// SKIP B_0063#38: needs Spellbook.Castables and item-from-carrier castability; no equivalent in the provided headers.
