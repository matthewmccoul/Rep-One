#include "core.h"

namespace mc {

cs::String Uncursing::Refusal(cs::Ref<InventorySlot> slot) {
    // C# returns null when the item may be uncursed; a null string is "" here
    return !slot->SlotCursed ? NotCursed : !slot->Equipped ? NotEquipped : cs::String();
}

}  // namespace mc
