#include "test_support.h"
#include "mordor/guilds2.h"

using namespace mordor;

// B_0061#155: QuestTier clamps CInt(level / 25) to 1..10 (R-quest tier, FRMCTYG.BAS:471-477)
TEST(engine_rtt167_quest_tier_level_250_is_10) {
  CHECK_EQ(QuestTier(250), 10);
}

// B_0061#156
TEST(engine_rtt167_quest_tier_level_999_is_10) {
  CHECK_EQ(QuestTier(999), 10);
}

// SKIP B_0061#159: needs Character, Guild and GuildQuest layouts plus the guild table
// (QuestPercent per guild) to find the most generous guild and count 2000 RollQuest results; not declared.

// SKIP B_0063#126: no declaration of the teleporter square draw (ForSquare equivalent); only TeleporterDest
// is declared in dungeon1b.h and its fields and the Floor/map type are not shown.

// SKIP B_0063#127: same as B_0063#126 - the party-split draw over 200 jumps needs the teleport destination
// function and the Floor/map type, which are not declared.

// SKIP B_0063#128: same as B_0063#126 - fixed-destination level-change teleport needs the teleport
// destination function and the Floor/map type, which are not declared.
