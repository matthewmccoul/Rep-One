#include "core.h"

namespace mc {

cs::Ref<Companion> CompanionSlots::Blank() {
    return [&] { auto _o = cs::New<Companion>(); _o->Name = " "; return _o; }();
}

cs::Ref<Companion> CompanionSlots::Index(int32_t slot) {
    return slot >= 0 && slot < Slots ? _slots[slot] : nullptr;
}

void CompanionSlots::SetIndex(int32_t slot, cs::Ref<Companion> value) {
    if (slot < 0 || slot >= Slots) return;
    if (value == nullptr) Release(slot); else _slots[slot] = value;
}

cs::Ref<Companion> CompanionSlots::Left(int32_t slot) {
    auto s = _slots[slot];
    return s ? s : _left[slot];
}

cs::Ref<Companion> CompanionSlots::Remains(int32_t slot) {
    if (slot < 0 || slot >= Slots || _slots[slot] != nullptr) return nullptr;
    auto r = _left[slot];
    bool blank = r->Name == " " && r->Hits == 0 && r->MaxHits == 0 && r->MonsterId == 0 && r->Ref == -1
                 && r->IdLevel == 0 && r->BindingLevel == 0 && r->Alignment == ItemAlignment{};
    return blank ? nullptr : r;
}

void CompanionSlots::SetRemains(int32_t slot, cs::Ref<Companion> record) {
    if (slot < 0 || slot >= Slots || _slots[slot] != nullptr) return;
    record->Vacant = false;
    _left[slot] = record;
}

void CompanionSlots::BlankEmptyNames() {
    for (int32_t i = 0; i < Slots; i++) if (_slots[i] == nullptr) _left[i]->Name = " ";
}

cs::List<Tup_Slot_Companion_d8dfb> CompanionSlots::Occupied() {
    cs::List<Tup_Slot_Companion_d8dfb> _result = cs::List<Tup_Slot_Companion_d8dfb>::New();
    for (int32_t i = 0; i < Slots; i++) {
        auto c = _slots[i];
        if (c) _result.Add(Tup_Slot_Companion_d8dfb{i, c});
    }
    return _result;
}

cs::List<cs::Ref<Companion>> CompanionSlots::All() {
    return Occupied().Select([&](auto& o) { return o.Companion; });
}

int32_t CompanionSlots::OccupiedCount() {
    return Slots - FreeSlots();
}

bool CompanionSlots::Any() {
    return FreeSlots() < Slots;
}

int32_t CompanionSlots::FreeSlots() {
    int32_t n = 0;
    for (int32_t i = 0; i < Slots; i++) if (_slots[i] == nullptr) n++;
    return n;
}

int32_t CompanionSlots::FirstFree() {
    for (int32_t i = 0; i < Slots; i++) if (_slots[i] == nullptr) return i;
    return -1;
}

int32_t CompanionSlots::Add(cs::Ref<Companion> c, bool keepSlotName) {
    int32_t at = FirstFree();
    if (at < 0) return -1;
    if (keepSlotName) c->Name = _left[at]->Name;
    _slots[at] = c;
    return at;
}

void CompanionSlots::Release(int32_t slot) {
    if (slot < 0 || slot >= Slots) return;
    auto was = _slots[slot];
    if (!was) return;
    _left[slot] = was;          // the record stays, name and hits
    _slots[slot] = nullptr;
}

void CompanionSlots::ReleaseAll() {
    for (int32_t i = 0; i < Slots; i++) Release(i);
}

cs::String CompanionSlots::Rearrange(int32_t onto, int32_t from) {
    if (onto == from) return {};
    auto there = Index(onto);
    if (!there) return {};
    auto moving = Index(from);
    if (!moving) return {};
    if (there->IsDead() || moving->IsDead()) return NoRearrangingTheDead;
    _slots[onto] = moving;
    _slots[from] = there;
    return {};
}

int32_t CompanionSlots::Settle(cs::List<cs::Ref<Monster>> monsters) {
    return CompanionRef::Settle(shared_from_this(), monsters);
}

int32_t CompanionSlots::SlotOf(cs::Ref<Companion> c) {
    for (int32_t i = 0; i < Slots; i++) if (_slots[i] == c) return i;
    return -1;
}

cs::Ref<CompanionSlots> CompanionSlotsJson::Read(cs::Utf8JsonReader& reader, cs::Type typeToConvert, cs::Ref<cs::JsonSerializerOptions> options) {
    throw cs::NotSupportedException("not translated: System.Text.Json converter Read");
}

void CompanionSlotsJson::Write(cs::Utf8JsonWriter writer, cs::Ref<CompanionSlots> value, cs::Ref<cs::JsonSerializerOptions> options) {
    throw cs::NotSupportedException("not translated: System.Text.Json converter Write");
}

}  // namespace mc
