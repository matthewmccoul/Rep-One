#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct PackRow {
    static bool Bold(cs::Ref<Item> it, cs::Ref<InventorySlot> slot);
    static bool Italic(cs::Ref<Item> it, cs::Ref<InventorySlot> slot);
};

}  // namespace mc
