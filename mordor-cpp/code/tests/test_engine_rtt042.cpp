#include "test_support.h"
#include "mordor/guilds1.h"

using namespace mordor;

// SKIP B_0059#57: QuestMonster takes an explicit monster list and class-range table and the
// expected pick is relational ("one before the first monster deeper than tier 2"). Our
// questMonster(Ctx&, tier) draws only from ctx.data, so the list cannot be injected here.

// SKIP B_0059#58: needs a ScriptedRng draw counter (Draws) and an injectable monster list.
// Neither is exposed by tsup::Env / ScriptedRng.

TEST(engine_rtt042_quest_monster_level_found_counts) {
  // B_0059#67: 4,000 draws over tiers 1-10 (400 per tier) from Random(20261018).
  tsup::Env env(static_cast<u64>(20261018));
  Ctx& ctx = env.ctx;
  i32 atOwnLevel = 0;
  bool idsValid = true;
  for (i32 t = 1; t <= 10; ++t) {
    for (i32 k = 0; k < 400; ++k) {
      i32 id = questMonster(ctx, t);
      if (id < 0 || id >= static_cast<i32>(ctx.data.monsters.size())) {
        idsValid = false;
        continue;
      }
      if (ctx.data.monsters[id].levelFound == t) ++atOwnLevel;
    }
  }
  CHECK(idsValid);
  // expected: At least 1.
  CHECK(atOwnLevel >= 1);
}
