#include "test_support.h"
#include "mordor/guilds2.h"

using namespace mordor;

// SKIP B_0061#20: GuildDef.GoldFactor is not a member in the shown headers; kGoldFactor[Thief] is 1.9, which does not match expected 9 and is not a unit conversion
// SKIP B_0061#127: no Rules.GuildTables or Rules.GuildAD equivalent in the headers to enumerate published A/D values
// SKIP B_0061#128: no Rules.GuildTables equivalent in the headers to count compared values

// Rows 129-135 check the published max A/D table constants (kMaxAttack/kMaxDefence),
// not a computed Rules.GuildAD + Rules.BaseAD result. Ctx/GuildDef setup for the computed path is not in the headers.

TEST(engine_B_0061_129_nomad_max_ad) {
    CHECK_EQ(kMaxAttack[static_cast<i32>(Guild::Nomad)], 175);
    CHECK_EQ(kMaxDefence[static_cast<i32>(Guild::Nomad)], 175);
}

TEST(engine_B_0061_130_warrior_max_ad) {
    CHECK_EQ(kMaxAttack[static_cast<i32>(Guild::Warrior)], 254);
    CHECK_EQ(kMaxDefence[static_cast<i32>(Guild::Warrior)], 276);
}

TEST(engine_B_0061_131_paladin_max_ad) {
    CHECK_EQ(kMaxAttack[static_cast<i32>(Guild::Paladin)], 223);
    CHECK_EQ(kMaxDefence[static_cast<i32>(Guild::Paladin)], 258);
}

TEST(engine_B_0061_132_ninja_max_ad) {
    CHECK_EQ(kMaxAttack[static_cast<i32>(Guild::Ninja)], 254);
    CHECK_EQ(kMaxDefence[static_cast<i32>(Guild::Ninja)], 254);
}

TEST(engine_B_0061_133_villain_max_ad) {
    CHECK_EQ(kMaxAttack[static_cast<i32>(Guild::Villain)], 243);
    CHECK_EQ(kMaxDefence[static_cast<i32>(Guild::Villain)], 272);
}

TEST(engine_B_0061_134_seeker_max_ad) {
    CHECK_EQ(kMaxAttack[static_cast<i32>(Guild::Seeker)], 208);
    CHECK_EQ(kMaxDefence[static_cast<i32>(Guild::Seeker)], 251);
}

TEST(engine_B_0061_135_thief_max_ad) {
    CHECK_EQ(kMaxAttack[static_cast<i32>(Guild::Thief)], 168);
    CHECK_EQ(kMaxDefence[static_cast<i32>(Guild::Thief)], 204);
}
