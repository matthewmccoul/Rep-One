// Reference: src/Mordor.Core/Rules.cs:1291 (QuestTier), FRMCTYG.BAS:471-477.
// Rule questtier: clamp(round_half_even(guildLevel / 25), 1, 10).
// Note: guilds1.h, guilds2.h and guilds3.h each declare a quest-tier function
// (questTier in guilds1.h, QuestTier in guilds2.h and guilds3.h). This file uses
// questTier from guilds1.h, the camelCase name matching the convention.
#include "test_support.h"
#include "mordor/guilds1.h"

using namespace mordor;

// B_0061#150
TEST(engine_re_questtier_level5_is_tier1) {
  CHECK_EQ(questTier(5), 1);
}

// B_0061#151
TEST(engine_re_questtier_level12_is_tier1) {
  CHECK_EQ(questTier(12), 1);
}

// B_0061#152
TEST(engine_re_questtier_level13_is_tier1) {
  CHECK_EQ(questTier(13), 1);
}

// B_0061#153
TEST(engine_re_questtier_level25_is_tier1) {
  CHECK_EQ(questTier(25), 1);
}

// B_0061#154
TEST(engine_re_questtier_level100_is_tier4) {
  CHECK_EQ(questTier(100), 4);
}

// B_0061#155
TEST(engine_re_questtier_level250_is_tier10) {
  CHECK_EQ(questTier(250), 10);
}

// B_0061#156
TEST(engine_re_questtier_level999_is_tier10) {
  CHECK_EQ(questTier(999), 10);
}
