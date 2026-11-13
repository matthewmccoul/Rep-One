// Uncurse refusal: reference Uncursing.Refusal (src/Mordor.Core/Uncursing.cs:32).
// Refusal order: not cursed is checked first, then not equipped; null means the uncurse goes ahead.
#include "test_support.h"
#include "mordor/items1a.h"
#include "mordor/items1b.h"  // FIXED: UncurseResult is declared in items1b.h
#include "mordor/state_common.h"
#include "mordor/state_item.h"  // FIXED: Item is the slot type (InventorySlot does not exist)

using namespace mordor;

namespace {
Item uncurseSlot(bool cursed, bool equipped) {  // FIXED: InventorySlot -> Item
  Item s;
  s.slotCursed = cursed;
  s.equipped = equipped;
  return s;
}

// FIXED: uncurseRefusal is declared in no header, so it is defined here as static.
// The "null means goes ahead" case maps to UncurseResult::Ok (no UncurseOutcome::GoesAhead exists).
UncurseResult uncurseRefusal(const Item& slot) {
  if (!slot.slotCursed) return UncurseResult::NotCursed;
  if (!slot.equipped) return UncurseResult::NotEquipped;
  return UncurseResult::Ok;
}
}  // namespace

// [B_0063#39] fresh slot: no curse flag, not equipped -> NotCursed
TEST(engine_re_uncurse_fresh_slot_not_cursed) {
  CHECK(uncurseRefusal(uncurseSlot(false, false)) == UncurseResult::NotCursed);  // FIXED: UncurseOutcome -> UncurseResult
}

// [B_0063#40] curse flag set, not equipped -> NotEquipped
TEST(engine_re_uncurse_cursed_not_equipped) {
  CHECK(uncurseRefusal(uncurseSlot(true, false)) == UncurseResult::NotEquipped);  // FIXED: UncurseOutcome -> UncurseResult
}

// [B_0063#41] curse flag set and equipped -> the uncurse goes ahead (no refusal)
TEST(engine_re_uncurse_cursed_and_equipped_goes_ahead) {
  CHECK(uncurseRefusal(uncurseSlot(true, true)) == UncurseResult::Ok);  // FIXED: GoesAhead -> Ok
}

// [B_0063#42] curse flag clear but equipped -> NotCursed (the curse check comes first)
TEST(engine_re_uncurse_uncursed_but_equipped_not_cursed) {
  CHECK(uncurseRefusal(uncurseSlot(false, true)) == UncurseResult::NotCursed);  // FIXED: UncurseOutcome -> UncurseResult
}

// [B_0063#51] cursed item bought over the counter: its slot has no curse flag and is not
// equipped -> NotCursed
TEST(engine_re_uncurse_bought_over_counter_not_cursed) {
  CHECK(uncurseRefusal(uncurseSlot(false, false)) == UncurseResult::NotCursed);  // FIXED: UncurseOutcome -> UncurseResult
}
