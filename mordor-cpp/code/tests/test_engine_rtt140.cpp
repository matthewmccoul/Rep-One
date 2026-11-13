#include "test_support.h"
#include <set>
#include "mordor/items1a.h"
#include "mordor/items1b.h"

using namespace mordor;

// B_0063#129: across the MDATA1 item classes, the distinct maximum values are exactly -1, 0, 1.
TEST(engine_rtt140_max_worn_of_class_distinct_values) {
  tsup::Env env;
  Ctx& ctx = env.ctx;
  std::set<i32> seen;
  for (i32 i = 0; i < (i32)ctx.data.itemDescriptions.size(); ++i)
    seen.insert(maxWornOfClass(ctx, i));
  CHECK_EQ(seen.size(), 3u);
  CHECK_EQ(seen.count(-1), 1u);
  CHECK_EQ(seen.count(0), 1u);
  CHECK_EQ(seen.count(1), 1u);
}

// B_0063#130: exactly 15 item classes have a maximum worn of 1.
TEST(engine_rtt140_max_worn_of_class_fifteen_with_max_one) {
  tsup::Env env;
  Ctx& ctx = env.ctx;
  i32 count = 0;
  for (i32 i = 0; i < (i32)ctx.data.itemDescriptions.size(); ++i)
    if (maxWornOfClass(ctx, i) == 1) ++count;
  CHECK_EQ(count, 15);
}

// SKIP B_0056#25: needs a Character with 18 in every stat, an Inventory with a dagger already
// equipped, and a second dagger tested; the Character/Inventory equip-state API is not in the
// declarations, so the pack setup cannot be expressed yet.
// SKIP B_0063#134: needs enumeration of the seven items in the maximum-minus-one class; there is
// no declared way to list MDATA items by wear class (WearClassOf is not declared).
// SKIP B_0063#44: needs WearRefusal (not declared) plus a slot with charges and known level for
// each findable cursed item.
