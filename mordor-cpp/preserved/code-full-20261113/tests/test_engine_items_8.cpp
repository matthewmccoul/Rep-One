#include "test_support.h"

#include "mordor/types.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/items2a.h"

using namespace mordor;

// B_0063#138: class with max worn 1, one already worn -> second refused.
TEST(engine_B_0063_138_second_item_of_capped_class_refused) {
    CHECK(wearClassFull(1, 1));
}

// SKIP B_0064#6: needs loading MDATA3.MDR and scanning guild indices against item ids; no loader in headers.

// B_0064#9: identification levels. IdLevel is {Unknown, Barely, Partial, Mostly, All};
// the levels excluding Unknown span All - Unknown = 4.
TEST(engine_B_0064_9_identification_level_count) {
    CHECK_EQ(static_cast<i32>(All) - static_cast<i32>(Unknown), 4);
}

// SKIP B_0064#13: needs MDATA3.MDR loaded and per-item LevelScale fields; no loader in headers.
// SKIP B_0064#14: needs MDATA3.MDR loaded and per-item LevelScale range; no loader in headers.
// SKIP B_0064#15: needs MDATA3.MDR record positions; no loader or record-index API in headers.
// SKIP B_0064#16: needs the full MDATA3.MDR item set and a bulk readable-level comparison; not expressible from headers.
// SKIP B_0064#17: needs a specific file-order item from MDATA3.MDR and the Loot.OnReceived slot flow; data selection not specified.
// SKIP B_0064#18: depends on the handed-over slot from B_0064#17 and the file-wide item selection.

// B_0065#110: character with nothing worn, Char page Item A/D = 0/0.
// Closest equivalent: a default Character has zero attack/defence ratings.
// FIXED: Character has no inventories_worn_size_placeholder_unused member; check combat.attack and combat.defence instead.
TEST(engine_B_0065_110_nothing_worn_ad_zero) {
    Character c{};
    CHECK_EQ(c.combat.attack, 0);
    CHECK_EQ(c.combat.defence, 0);
}
