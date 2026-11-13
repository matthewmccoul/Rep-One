#include "test_support.h"
#include "mordor/companions1b.h"

using namespace mordor;

// SKIP B_0064#99: runCarryChoice takes no owner name or on-screen flag, so the "Ann ... dropped" message text cannot be set up or checked.

TEST(engine_B_0064_100_carry_kept_after_too_big) {
    tsup::Env env({0.5, 0.9});
    Arr<Companion, kCompanionSlots> slots{};
    slots[0].occupied = true;
    slots[0].hits = 0;
    slots[0].maxHits = 5;
    slots[0].name = "Companion #1";
    slots[2].occupied = true;
    slots[2].hits = 0;
    slots[2].maxHits = 50;
    slots[2].name = "Companion #3";
    Inventory inv{};
    inv.carryIndex = 0;
    Arr<i32, kCompanionSlots> bodySizes{};
    bodySizes[0] = 1;
    bodySizes[2] = 4;
    runCarryChoice(env.ctx, slots, inv, bodySizes, 1);
    CHECK_EQ(inv.carryIndex, 0);
}

// SKIP B_0064#101: the "0,-,-,-" notation for slot contents is not defined by the headers; cannot tell what it asserts.

// SKIP B_0064#102: the "Ann, you are now carrying" message needs the owner name and off-screen state, which runCarryChoice does not take.

TEST(engine_B_0064_103_carry_index_after_pickup) {
    tsup::Env env({0.5, 0.9});
    Arr<Companion, kCompanionSlots> slots{};
    slots[1].occupied = true;
    slots[1].hits = 0;
    slots[1].maxHits = 5;
    slots[1].name = "Companion #2";
    Inventory inv{};
    inv.carryIndex = kCarryNone;
    Arr<i32, kCompanionSlots> bodySizes{};
    bodySizes[1] = 1;
    runCarryChoice(env.ctx, slots, inv, bodySizes, 1);
    CHECK_EQ(inv.carryIndex, 1);
}

TEST(engine_B_0064_104_slot_name_after_release) {
    Companion fido{};
    fido.name = "Fido";
    fido.occupied = false;
    CHECK(keepSlotName(fido) == "Fido");
}

TEST(engine_B_0064_105_slot_keeps_name_on_new_companion) {
    tsup::Env env({0.5, 0.9});
    Companion prev{};
    prev.name = "Fido";
    prev.occupied = false;
    prev.slot = 0;
    Arr<Companion, kCompanionSlots> slots{};
    slots[0] = prev;
    Companion incoming{};
    incoming.name = "";
    incoming.occupied = true;
    placeNewCompanion(env.ctx, slots, incoming);
    CHECK(slots[0].name == "Fido");
}

TEST(engine_B_0064_106_blank_slot_name_defaults) {
    tsup::Env env({0.5, 0.9});
    Companion prev{};
    prev.name = "";
    prev.occupied = false;
    prev.slot = 0;
    Arr<Companion, kCompanionSlots> slots{};
    slots[0] = prev;
    Companion incoming{};
    incoming.name = "";
    incoming.occupied = true;
    placeNewCompanion(env.ctx, slots, incoming);
    CHECK(slots[0].name == "Companion #1");
}

// SKIP B_0064#107: needs a 3000-round combat simulation; no single function in the headers covers it.

// SKIP B_0064#108: needs a 3000-round combat simulation; no single function in the headers covers it.
