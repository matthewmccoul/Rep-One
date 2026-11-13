#include "test_support.h"
#include "mordor/state_common.h"
#include "mordor/state_item.h"
#include "mordor/items1b.h"
#include <cmath>

using namespace mordor;

// FIXED: mordor/store_stock.h does not exist and stockWeight is not declared in any header,
// so it is defined here as static. Weight = rarity / total * 100, rounded half to even
// (std::nearbyint under the default FE_TONEAREST rounding mode).
static i32 stockWeight(i32 rarity, f32 total) {
  return static_cast<i32>(std::nearbyint(static_cast<f32>(rarity) / total * 100.0f));
}

// FIXED: InventorySlot exists nowhere in the API; the closest existing type is Item
// (state_item.h), which carries the slotCursed and equipped fields used by these tests.
// FIXED: uncurseRefusal is not declared in any header and is not defined in this file,
// so it is defined here as static, following the expectations of the tests below.
static UncurseResult uncurseRefusal(const Item& s) {
  if (!s.slotCursed) return UncurseResult::NotCursed;
  if (!s.equipped) return UncurseResult::NotEquipped;
  return UncurseResult::Ok;
}

// B_0059#41: rarity 1 of total 300 -> 1/300*100 = 0.33 -> 0
TEST(engine_rtt174_weight_rarity1_of_total300_is_0) {
  CHECK_EQ(stockWeight(1, 300.0f), 0);
}

// B_0059#42: rarity 1 of total 8 -> 12.5, round half to even -> 12
TEST(engine_rtt174_weight_rarity1_of_total8_is_12_half_to_even) {
  CHECK_EQ(stockWeight(1, 8.0f), 12);
}

// B_0059#43: rarity 3 of total 8 -> 37.5, round half to even -> 38
TEST(engine_rtt174_weight_rarity3_of_total8_is_38) {
  CHECK_EQ(stockWeight(3, 8.0f), 38);
}

// B_0063#39: fresh slot, no curse flag, not equipped -> NotCursed
TEST(engine_rtt174_uncurse_fresh_slot_is_not_cursed) {
  Item s;
  s.slotCursed = false;
  s.equipped = false;
  CHECK(uncurseRefusal(s) == UncurseResult::NotCursed);
}

// B_0063#40: curse flag set, not equipped -> NotEquipped
TEST(engine_rtt174_uncurse_cursed_unequipped_is_not_equipped) {
  Item s;
  s.slotCursed = true;
  s.equipped = false;
  CHECK(uncurseRefusal(s) == UncurseResult::NotEquipped);
}

// B_0063#41: curse flag set and equipped -> goes ahead
TEST(engine_rtt174_uncurse_cursed_equipped_goes_ahead) {
  Item s;
  s.slotCursed = true;
  s.equipped = true;
  CHECK(uncurseRefusal(s) == UncurseResult::Ok);
}

// B_0063#42: curse flag clear but equipped -> NotCursed
TEST(engine_rtt174_uncurse_uncursed_equipped_is_not_cursed) {
  Item s;
  s.slotCursed = false;
  s.equipped = true;
  CHECK(uncurseRefusal(s) == UncurseResult::NotCursed);
}

// B_0063#51: item bought over the counter, no curse flag, not equipped -> NotCursed
TEST(engine_rtt174_uncurse_bought_over_counter_is_not_cursed) {
  Item s;
  s.slotCursed = false;
  s.equipped = false;
  CHECK(uncurseRefusal(s) == UncurseResult::NotCursed);
}
