#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/items1b.h"

using namespace mordor;

// Wear-class counts run over every class in the item-description table.
// These expectations come from the full game's MDATA1, so skip on the shareware data.

TEST(engine_rtt141_max_worn_zero_class_count) {
  if (tsup::fullGameOnly("engine_rtt141_max_worn_zero_class_count")) return;
  tsup::Env env;
  const i32 n = static_cast<i32>(env.ctx.data.itemDescriptions.size());
  i32 zeros = 0;
  for (i32 c = 0; c < n; ++c)
    if (maxWornOfClass(env.ctx, c) == 0) ++zeros;
  CHECK_EQ(zeros, 8);
}

TEST(engine_rtt141_max_worn_minus_one_class_count) {
  if (tsup::fullGameOnly("engine_rtt141_max_worn_minus_one_class_count")) return;
  tsup::Env env;
  const i32 n = static_cast<i32>(env.ctx.data.itemDescriptions.size());
  i32 minusOnes = 0;
  for (i32 c = 0; c < n; ++c)
    if (maxWornOfClass(env.ctx, c) == -1) ++minusOnes;
  CHECK_EQ(minusOnes, 1);
}

TEST(engine_rtt141_max_worn_minus_one_value) {
  if (tsup::fullGameOnly("engine_rtt141_max_worn_minus_one_value")) return;
  tsup::Env env;
  const i32 n = static_cast<i32>(env.ctx.data.itemDescriptions.size());
  bool found = false;
  for (i32 c = 0; c < n; ++c) {
    if (maxWornOfClass(env.ctx, c) == -1) {
      found = true;
      CHECK_EQ(maxWornOfClass(env.ctx, c), -1);
    }
  }
  CHECK(found);
}

// SKIP B_0062#65: needs the MDATA3 item table (GameData item list) and wearClassOf to collect
//   distinct wear-class maxima; neither is in the declarations yet (see NEED).
// SKIP B_0062#66: needs the MDATA3 item table and wearClassOf to find a non-zero-id item with max -1 (see NEED).
// SKIP B_0062#68: needs the Medusa's Head lookup in the MDATA3 item table and wearClassOf (see NEED).
// SKIP B_0062#69: needs the MDATA3 item table and wearClassOf to find a non-zero-id item with max 0 (see NEED).
// SKIP B_0067#16: needs Character::swapSlots with buffersFollow and SpellBuffers::packSlotOf, which are not declared (see NEED).

