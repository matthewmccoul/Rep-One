#include "test_support.h"
#include "mordor/guilds1.h"

using namespace mordor;

// SKIP B_0063#86: our perceiveSquare(Ctx&, SquareState&) is not the pure rule (rng, bit, best, bestName, inParty, lost) -> Outcome; no PerceiveSquare overload or Outcome type is declared, so the lost-character certainty line cannot be checked through our API.

TEST(engine_rtt166_quest_item_band_deepest_tier10) {
  // B_0061#157: deepest item level a quest can send for, at tier 10
  CHECK_EQ(itemBand(10), 7);
}

TEST(engine_rtt166_quest_item_band_tier1) {
  // B_0061#158: item band for tier 1
  CHECK_EQ(itemBand(1), 1);
}

TEST(engine_rtt166_quest_tier_guild_level_5) {
  // B_0061#150
  CHECK_EQ(questTier(5), 1);
}

TEST(engine_rtt166_quest_tier_guild_level_12) {
  // B_0061#151
  CHECK_EQ(questTier(12), 1);
}

TEST(engine_rtt166_quest_tier_guild_level_13) {
  // B_0061#152
  CHECK_EQ(questTier(13), 1);
}

TEST(engine_rtt166_quest_tier_guild_level_25) {
  // B_0061#153
  CHECK_EQ(questTier(25), 1);
}

TEST(engine_rtt166_quest_tier_guild_level_100) {
  // B_0061#154
  CHECK_EQ(questTier(100), 4);
}
