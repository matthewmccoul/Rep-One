#include "core.h"

namespace mc {

cs::String Combining::Refusal(cs::List<cs::Ref<InventorySlot>> chosen, cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::List<int32_t> slotNumbers) {
    // FRMCTYS.BAS:647-653 - checked for every item in the list, before anything else
    for (int32_t i = 0; i < chosen.Count(); i++)
        if (chosen[i]->Known() != IdLevel::All)
            return "Item #" + cs::ToStr(slotNumbers[i] + 1) + " Not Completely Identified!";

    // FRMCTYS.BAS:654-657 - one item on its own is not an error, it just does nothing
    if (chosen.Count() < 2) return "";

    auto first = lookup(chosen[0]->ItemId);
    if (first == nullptr) return "";

    // FRMCTYS.BAS:662-668 - the eight kinds, and the kind alone
    if (!CombinableKind(first)) return "Item Class cannot be Combined!";
    return cs::String{};  // C# null
}

bool Combining::CombinableKind(cs::Ref<Item> it) {
    auto t = it->Type;
    return t == 23 || t == 24 || t == 25 || t == 26 || t == 27 || t == 28 || t == 30 || t == 31;
}

cs::List<Combining_Result> Combining::Combine(cs::Ref<Character> c, cs::List<cs::Ref<InventorySlot>> chosen,
                                              cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::List<int32_t> slotNumbers) {
    auto said = cs::List<Result>::New();
    auto target = chosen[0];
    auto first = lookup(target->ItemId);
    if (first == nullptr) return said;

    for (int32_t i = 1; i < chosen.Count(); i++) {
        auto slot = chosen[i];
        int32_t n = slotNumbers[i] + 1;
        // the original skips any repeat of the first slot number silently
        if (slot == target) continue;

        // a slot already poured in has Ref 0 (FRMCTYS.BAS:678), so naming it again reads item 0 -
        // the Hands - and is "not compatible class"
        auto it = lookup(slot->ItemId);
        if (slot->Known() != IdLevel::All)
            said.Add(Result("STORE: Item #" + cs::ToStr(n) + " is not completely identified.", false));
        else if (it == nullptr || it->Type != first->Type)
            said.Add(Result("STORE: Item #" + cs::ToStr(n) + " is not compatible class.", false));
        else if (slot->ItemId != target->ItemId)
            said.Add(Result("STORE: Item #" + cs::ToStr(n) + " is not of the same type.", false));
        else if (slot->Alignment != target->Alignment)
            said.Add(Result("STORE: Item #" + cs::ToStr(n) + " is not compatible Alignment.", false));
        else {
            said.Add(Result("STORE: Item #" + cs::ToStr(n) + " combined to Item #" + cs::ToStr(slotNumbers[0] + 1) + ".", true));
            int32_t sum = (int32_t)target->Charges + (int32_t)slot->Charges;
            if (sum > INT16_MAX || sum < INT16_MIN) {
                said.Add(Result("", false, true));
                return said;
            }
            target->Charges = (int16_t)sum;
            // "Chars(Me).Pack(l0214(l0212)).Ref = 0" (FRMCTYS.BAS:678) and nothing more: the slot
            // keeps its charges, alignment, identification and flags
            slot->ItemId = 0;
        }
    }
    return said;
}

}  // namespace mc
