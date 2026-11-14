#include "core.h"

namespace mc {

int32_t SpellBuffers::ForItem(int32_t packSlot) { return packSlot; }
int32_t SpellBuffers::ForSpell(int32_t spellIndex) { return PackSlots + spellIndex; }

bool SpellBuffers::IsEmpty(int32_t buffer) { return buffer < 0; }
bool SpellBuffers::IsItem(int32_t buffer) { return buffer >= 0 && buffer < PackSlots; }
bool SpellBuffers::IsSpell(int32_t buffer) { return buffer >= PackSlots; }

int32_t SpellBuffers::PackSlotOf(int32_t buffer) { return buffer; }
int32_t SpellBuffers::SpellIndexOf(int32_t buffer) { return buffer - PackSlots; }

void SpellBuffers::PackSlotEmptied(cs::Ref<Character> c, int32_t slot) {
    for (int32_t i = 0; i < Count && i < c->Buffers.Length(); i++)
        if (IsItem(c->Buffers[i]) && PackSlotOf(c->Buffers[i]) == slot) c->Buffers[i] = Empty;
}

void SpellBuffers::PackSlotsSwapped(cs::Ref<Character> c, int32_t a, int32_t b) {
    for (int32_t i = 0; i < Count && i < c->Buffers.Length(); i++) {
        int32_t buf = c->Buffers[i];
        if (!IsItem(buf)) continue;
        int32_t slot = PackSlotOf(buf);
        if (slot == a) c->Buffers[i] = ForItem(b);
        else if (slot == b) c->Buffers[i] = ForItem(a);
    }
}

cs::String SpellBuffers::Describe(cs::Ref<Character> c, int32_t slot, cs::List<cs::Ref<Spell>> spells, cs::Func<cs::Ref<Item>(int16_t)> item) {
    if (slot < 0 || slot >= Count) return "(Nothing)";
    int32_t b = c->Buffers[slot];

    // a spell past the end of the table shows as nothing but is not cleared (4639-4641)
    if (IsSpell(b) && (spells == nullptr || SpellIndexOf(b) >= spells.Count())) return "(Nothing)";
    if (IsEmpty(b)) return "(Nothing)";

    if (IsItem(b)) {
        // A buffer holds a pack slot (FRMCHAR1.BAS:220, Source.Index)
        if (b <= 0 || b >= Character::PackSize) { c->Buffers[slot] = Empty; return "(Nothing)"; }
        auto held = c->Pack()[b];
        if (held == nullptr || held->ItemId == 0) { c->Buffers[slot] = Empty; return "(Nothing)"; }
        auto it = item(held->ItemId);
        if (it == nullptr) { c->Buffers[slot] = Empty; return "(Nothing)"; }
        return "Item #" + cs::ToStr(b + 1) + ". " + Loot::NameFor(it, held->Known(), held->Alignment, held->Charges);
    }

    return "\"" + cs::ToStr(spells[SpellIndexOf(b)]->Name) + "\"";
}

cs::String SpellBuffers::CostText(cs::Ref<Character> c, int32_t slot, cs::List<cs::Ref<Spell>> spells) {
    if (slot < 0 || slot >= Count) return "";
    int32_t b = c->Buffers[slot];
    if (!IsSpell(b) || spells == nullptr || SpellIndexOf(b) >= spells.Count()) return "";
    int32_t cost = Spellbook::Cost(spells[SpellIndexOf(b)], c);
    return cost == 0 ? cs::String("?") : cs::ToStr(cost);
}

}  // namespace mc
