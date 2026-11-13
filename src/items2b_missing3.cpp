#include "mordor/items2b.h"

namespace mordor {

// R22: Equipping applies the equip curse, which sets the slot curse flag.
// The flag is what blocks unequip (R21, R22).
// UNKNOWN: R22 does not say whether the equip curse is gated on item.cursed.
// The text reads as unconditional, so this sets the flag on every equip.
// ASSUMED: unconditional, following the literal wording of R22.
void applyEquipCurse(Item& item) {
    item.slotCursed = true;
}

}  // namespace mordor
