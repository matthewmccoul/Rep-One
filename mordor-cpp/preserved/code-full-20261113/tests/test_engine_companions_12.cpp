#include "test_support.h"
#include "mordor/companions1b.h"
#include "mordor/companions2a.h"

using namespace mordor;

// SKIP B_0065#58: needs a multi-round seeded simulation of two companions (counting same-round casts)
// SKIP B_0065#59: needs a multi-round seeded simulation counting cast rounds
// SKIP B_0065#61: needs a multi-round seeded simulation of run-out messages on odd runs
// SKIP B_0065#62: needs a multi-round simulation checking casts/Fight/hold state across run-out rounds
// SKIP B_0065#64: needs a 300-round seeded simulation counting failed heal reports (spellHealsNoOne covers only the M5B1D=0 rule)
// SKIP B_0065#65: needs a 600-round seeded simulation of mishap victims with party and companion layout
// SKIP B_0065#66: relational count over a 600-round simulation of mishap lines
// SKIP B_0065#67: needs a 600-round seeded simulation of mishaps hitting the companion

TEST(engine_B_0065_60_each_cast_takes_one_charge) {
    tsup::Env env({0.5, 0.9});
    Inventory inv{};
    inv.pack[0].charges = 5;
    companionTakeCastCharge(env.ctx, inv, 0);
    CHECK_EQ(inv.pack[0].charges, 4);
}

TEST(engine_B_0065_63_empty_item_order_flips_start_orders) {
    tsup::Env env({0.5, 0.9});
    Character owner{};

    owner.order.kind = CombatOrderKind::None;
    ownerOrderAfterEmptyItem(env.ctx, owner, 0);
    CHECK(owner.order.kind == CombatOrderKind::Defend);

    owner.order.kind = CombatOrderKind::Fight;
    ownerOrderAfterEmptyItem(env.ctx, owner, 0);
    CHECK(owner.order.kind == CombatOrderKind::Defend);

    owner.order.kind = CombatOrderKind::Defend;
    ownerOrderAfterEmptyItem(env.ctx, owner, 0);
    CHECK(owner.order.kind == CombatOrderKind::Fight);
}
