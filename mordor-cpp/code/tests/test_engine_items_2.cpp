#include "test_support.h"
#include "mordor/items2a.h"

using namespace mordor;

// SKIP B_0059#1: Source-code ordering check (UPDATE call before Hands test in FRMCITYB.BAS); not testable through the new API.
// SKIP B_0059#22: GiveCommand.ItemAnswer has no equivalent in the headers; giveItem takes a resolved slot, not a typed item-number tag.
// SKIP B_0059#23: GiveCommand.ItemAnswer refusal text ("No Item to give!") has no equivalent; headers return GiveResult enums, not strings.
// SKIP B_0059#24: GiveCommand.ItemAnswer tag "2" has no equivalent; slotFromAnswer's input/output semantics do not match the SlotFromTag contract.
// SKIP B_0059#25: GiveCommand.ItemAnswer refusal "Item is Equipped!" has no string-returning equivalent; giveItem returns GiveResult::Equipped only for a resolved slot.
// SKIP B_0059#26: GiveCommand.ItemAnswer refusal for empty slot has no equivalent; see B_0059#22.
// SKIP B_0059#27: GiveCommand.ItemAnswer refusal for out-of-pack item number has no equivalent; see B_0059#22.
// SKIP B_0059#28: GiveCommand.SlotFromTag has no direct equivalent; slotFromAnswer is not confirmed to implement the same tag-to-slot mapping.

// B_0061#90: special type 1 changes stats. Mapped to specialAppliesStatMods(1), which lists types 1, 13, 14.
TEST(engine_B_0061_90_special_type1_changes_stats) {
    CHECK(specialAppliesStatMods(1));
}

// B_0061#91: special type 14 changes stats. Mapped to specialAppliesStatMods(14).
TEST(engine_B_0061_91_special_type14_changes_stats) {
    CHECK(specialAppliesStatMods(14));
}
