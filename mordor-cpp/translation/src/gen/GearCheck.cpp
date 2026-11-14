#include "core.h"

namespace mc {

cs::String GearCheck::NoLongerUsable(cs::Ref<Character> who, cs::String itemName, bool inAParty) {
    return inAParty ? "\"" + cs::Str::TrimEnd(who->Name) + "\" can no longer use their " + itemName + "!"
                    : "You can no longer use your " + itemName + "!";
}

cs::List<cs::String> GearCheck::Run(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> lookup, bool inAParty, bool announce) {
    auto said = cs::List<cs::String>::New();
    for (int32_t i = 1; i < Character::PackSize; i++) {
        cs::Ref<InventorySlot> slot = c->Pack()[i];
        if (slot == nullptr || !slot->Equipped || slot->ItemId == 0) continue;
        auto it = lookup(slot->ItemId);
        if (it == nullptr || c->EquipProblem(it).empty()) continue;

        // sub0F74 called with a positive slot index: a cursed item refuses to come off,
        // but the announcement is still made (MODULE6.BAS:1728). Reproduced.
        if (!slot->SlotCursed) slot->Equipped = false;
        if (announce) said.Add(NoLongerUsable(c, Loot::NameFor(it, slot->Known()), inAParty));
    }
    return said;
}

bool GearCheck::Any(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> lookup) {
    bool any = false;
    for (int32_t i = 1; i < Character::PackSize; i++) {
        cs::Ref<InventorySlot> slot = c->Pack()[i];
        if (slot == nullptr || !slot->Equipped || slot->ItemId == 0) continue;
        auto it = lookup(slot->ItemId);
        if (it == nullptr || c->EquipProblem(it).empty()) continue;
        if (!slot->SlotCursed) slot->Equipped = false;
        any = true;   // l018E is set whether or not the item actually came off
    }
    return any;
}

}  // namespace mc
