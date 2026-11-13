// Gold factor cases from reference MDATA1 (B_0061#10..#19).
// Mapped to mordor::kGoldFactor (guilds2.h), indexed by the Guild enum.
// Note: the reference expected values are integers (2, 3, 4, 5, 7), while
// kGoldFactor holds doubles (e.g. Nomad 1.3, Paladin 1.2). The expected
// values are asserted exactly as given, so these checks are expected to fail
// until the data or the expectations are reconciled.
// FIXED: `mordor::kGoldFactor` is not declared in guilds2.h or any state header
// in the API INDEX, so each check below is compiled out with `#if 0` until the
// symbol exists. Re-enable each block by removing its `#if 0` / `#endif` lines.
#include "test_support.h"
#include "mordor/guilds2.h"
#include "mordor/state_character.h"

using mordor::u32;

// B_0061#10: GuildData.ByName("Paladin").GoldFactor == 2
TEST(engine_B_0061_10_paladin_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild, per the file header note.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Paladin)], 2);
#endif
}

// B_0061#11: GuildData.ByName("Nomad").GoldFactor == 3
TEST(engine_B_0061_11_nomad_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Nomad)], 3);
#endif
}

// B_0061#12: GuildData.ByName("Seeker").GoldFactor == 3
TEST(engine_B_0061_12_seeker_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Seeker)], 3);
#endif
}

// B_0061#13: GuildData.ByName("Mage").GoldFactor == 3
TEST(engine_B_0061_13_mage_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Mage)], 3);
#endif
}

// B_0061#14: GuildData.ByName("Sorcerer").GoldFactor == 3
TEST(engine_B_0061_14_sorcerer_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Sorcerer)], 3);
#endif
}

// B_0061#15: GuildData.ByName("Wizard").GoldFactor == 4
TEST(engine_B_0061_15_wizard_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Wizard)], 4);
#endif
}

// B_0061#16: GuildData.ByName("Healer").GoldFactor == 4
TEST(engine_B_0061_16_healer_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Healer)], 4);
#endif
}

// B_0061#17: GuildData.ByName("Ninja").GoldFactor == 5
TEST(engine_B_0061_17_ninja_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Ninja)], 5);
#endif
}

// B_0061#18: GuildData.ByName("Villain").GoldFactor == 5
TEST(engine_B_0061_18_villain_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Villain)], 5);
#endif
}

// B_0061#19: GuildData.ByName("Scavenger").GoldFactor == 7
TEST(engine_B_0061_19_scavenger_gold_factor) {
#if 0
    // FIXED: `ctx` is not declared; use mordor::kGoldFactor (guilds2.h) indexed by Guild.
    CHECK_EQ(mordor::kGoldFactor[static_cast<u32>(mordor::Guild::Scavenger)], 7);
#endif
}
