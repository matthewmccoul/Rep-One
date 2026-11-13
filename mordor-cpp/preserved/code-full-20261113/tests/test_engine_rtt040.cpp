#include "test_support.h"
#include "mordor/game_data.h"

using namespace mordor;

namespace {
const GuildDef* guildNamed(const GameData& gd, const char* name) {
  for (const auto& g : gd.guilds) {
    if (g.name == name) return &g;
  }
  return nullptr;
}
}  // namespace

// B_0064#4: guilds with ExtraSwings > 0 (Multiple Swings) in the guild table.
TEST(engine_rtt040_extra_swings_count) {
  const GameData& gd = tsup::data();
  i32 n = 0;
  for (const auto& g : gd.guilds) {
    if (g.extraSwings > 0) ++n;
  }
  CHECK_EQ(n, 5);
}

// B_0061#9: Warrior gold factor in MDATA1.
TEST(engine_rtt040_warrior_gold_factor) {
  const GameData& gd = tsup::data();
  const GuildDef* w = guildNamed(gd, "Warrior");
  CHECK(w != nullptr);
  if (w) CHECK_EQ(w->goldFactor, 1);
}

// B_0061#20: Thief gold factor in MDATA1.
TEST(engine_rtt040_thief_gold_factor) {
  const GameData& gd = tsup::data();
  const GuildDef* t = guildNamed(gd, "Thief");
  CHECK(t != nullptr);
  if (t) CHECK_EQ(t->goldFactor, 9);
}

// SKIP B_0062#20: the Race / Guild Chart's expected comma-joined guild lists are not in the case, so the per-race match cannot be checked.
// SKIP B_0062#21: the mismatch list depends on the same Race / Guild Chart strings, which are not provided.
// SKIP B_0064#1: no guild-log wording renderer is declared (only GuildLogKind/GuildLogLine), so the rendered text cannot be inspected.
// SKIP B_0062#22: the page's guildmaster names and takeover levels are not provided, so the seeded comparison cannot be made.
// SKIP B_0062#23: the same page data is required for the mismatch count and is not provided.
