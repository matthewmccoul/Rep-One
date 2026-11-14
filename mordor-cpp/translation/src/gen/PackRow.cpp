#include "core.h"

namespace mc {

bool PackRow::Bold(cs::Ref<Item> it, cs::Ref<InventorySlot> slot) {
    return it != nullptr && cs::deref(slot).Known() == IdLevel::All && (it->Charges > 0 || slot->Charges > 0);
}

bool PackRow::Italic(cs::Ref<Item> it, cs::Ref<InventorySlot> slot) {
    return it != nullptr && cs::deref(slot).Known() == IdLevel::All && it->Cursed != 0;
}

}  // namespace mc
