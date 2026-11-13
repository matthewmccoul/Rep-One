#include "test_support.h"
#include "mordor/items2b.h"

using namespace mordor;

// SKIP B_0063#46: lists item names from MDATA3.MDR data; no equivalent function in the headers
// SKIP B_0063#47: counts Cursed values across MDATA3.MDR data; no equivalent function in the headers
// SKIP B_0063#48: needs the first Cursed=1 item from MDATA3.MDR; no data lookup function in the headers
// SKIP B_0063#49: needs the first Cursed=2 item from MDATA3.MDR; no data lookup function in the headers
// SKIP B_0063#55: needs a Ctx for applyEquipCurse(ctx, slot, wearer); mordor::Ctx has no default constructor and its constructor arguments are not in the API index
// SKIP B_0063#57: counts items by Floor from MDATA3.MDR data; no equivalent function in the headers

TEST(engine_B_0063_50_uncurse_not_equipped) {
    // Cursed item found in a chest: curse flag set at pickup, item not equipped.
    Item slot;
    slot.cursed = CurseKind1;
    applyChestDropCurse(slot);
    CHECK(checkUncurse(slot) == UncurseResult::NotEquipped);
}

TEST(engine_B_0063_52_equip_curse_sets_flag) {
    // Cursed item equipped; ApplyEquipCurse sets the slot's curse flag.
    Item slot;
    slot.cursed = CurseKind2;
    slot.equipped = true;
    applyEquipCurse(slot);
    CHECK(slot.slotCursed == true);
}

TEST(engine_B_0063_53_cursed_slot_cannot_unequip) {
    // Continuing from a cursed-and-equipped slot: it may not come off.
    Item slot;
    slot.cursed = CurseKind2;
    slot.equipped = true;
    applyEquipCurse(slot);
    CHECK(canUnequip(slot) == false);
}

TEST(engine_B_0063_54_cleared_slot_can_unequip) {
    // Same slot with its curse flag cleared: it may come off.
    Item slot;
    slot.cursed = CurseKind2;
    slot.equipped = true;
    applyEquipCurse(slot);
    slot.slotCursed = false;
    CHECK(canUnequip(slot) == true);
}
