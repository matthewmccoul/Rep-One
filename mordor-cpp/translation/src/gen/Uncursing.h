#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Uncursing {
    inline static const cs::String NotCursed = "Item is not cursed!";
    inline static const cs::String NotEquipped = "Item isn't equipped!";

    static cs::String Refusal(cs::Ref<InventorySlot> slot);
};

}  // namespace mc
