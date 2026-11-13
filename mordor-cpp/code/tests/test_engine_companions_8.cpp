#include "test_support.h"
#include "mordor/companions1b.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"

using namespace mordor;

// SKIP B_0064_88: depends on the earlier setup of case B_0064#87 (not provided); carry state before this case is unknown.
// SKIP B_0064_89: depends on the earlier setup of case B_0064#87 (not provided); slot state before this case is unknown.
// SKIP B_0064_90: asserts emitted message text; EventLog accessors are not in the headers, and owner visibility has no parameter.
// SKIP B_0064_93: asserts message count; EventLog accessors are not in the headers, and a morgue-body carry has no header representation.
// SKIP B_0064_94: morgue-body carry state and the meaning of "held" for kept slots are not representable with the given headers.
// SKIP B_0064_95: a morgue-body carry (carried name "Bob") has no header field that can be asserted reliably.
// SKIP B_0064_96: asserts emitted message text joined by '|'; EventLog accessors are not in the headers.

TEST(engine_B_0064_92_too_big_not_carried) {
    tsup::Env env({0.5, 0.9});
    Arr<Companion, kCompanionSlots> slots{};
    Arr<i32, kCompanionSlots> bodySizes{};
    Inventory inv{};
    // Huge fallen companion (size 4, hits 0, max 50) in slot 3; nothing carried.
    slots[3].occupied = true;
    slots[3].hits = 0;
    slots[3].maxHits = 50;
    slots[3].slot = 3;
    bodySizes[3] = 4;
    inv.carryIndex = kCarryNone;
    // Carrier size 2 is below the huge body size 4, so the pickup is refused.
    runCarryChoice(env.ctx, slots, inv, bodySizes, 2);
    CHECK_EQ(inv.carryIndex, -1);
}

TEST(engine_B_0064_97_swap_carries_companion_3) {
    tsup::Env env({0.5, 0.9});
    Arr<Companion, kCompanionSlots> slots{};
    Arr<i32, kCompanionSlots> bodySizes{};
    Inventory inv{};
    // Companion #1: small, carried, hits 0, max 5 (slot 0).
    slots[0].occupied = true;
    slots[0].hits = 0;
    slots[0].maxHits = 5;
    slots[0].slot = 0;
    bodySizes[0] = 1;
    // Companion #3: small, on the ground, hits -3, max 50 (slot 2).
    slots[2].occupied = true;
    slots[2].hits = -3;
    slots[2].maxHits = 50;
    slots[2].slot = 2;
    bodySizes[2] = 1;
    inv.carryIndex = 0;
    runCarryChoice(env.ctx, slots, inv, bodySizes, 2);
    CHECK_EQ(inv.carryIndex, 2);
}

TEST(engine_B_0064_98_swap_slots_afterwards) {
    tsup::Env env({0.5, 0.9});
    Arr<Companion, kCompanionSlots> slots{};
    Arr<i32, kCompanionSlots> bodySizes{};
    Inventory inv{};
    // Same setup as B_0064#97.
    slots[0].occupied = true;
    slots[0].hits = 0;
    slots[0].maxHits = 5;
    slots[0].slot = 0;
    bodySizes[0] = 1;
    slots[2].occupied = true;
    slots[2].hits = -3;
    slots[2].maxHits = 50;
    slots[2].slot = 2;
    bodySizes[2] = 1;
    inv.carryIndex = 0;
    runCarryChoice(env.ctx, slots, inv, bodySizes, 2);
    // Expected slot display: -,-,0,-
    CHECK(!slots[0].occupied);
    CHECK(!slots[1].occupied);
    CHECK(slots[2].occupied);
    CHECK_EQ(slots[2].hits, 0);
    CHECK(!slots[3].occupied);
}
