#include "core.h"

namespace mc {

cs::Ref<Monster> CompanionRef::At(cs::List<cs::Ref<Monster>> table, int32_t r) {
    if (r < 0) return nullptr;
    if (r < table.Count() && (table[r]->Ref == r || table[r]->Ref < 0)) return table[r];
    for (auto& m : table) if (m->Ref == r) return m;
    return nullptr;
}

int32_t CompanionRef::IndexOf(cs::List<cs::Ref<Monster>> table, cs::Ref<Monster> m) {
    if (m->Ref >= 0) return m->Ref;
    for (int32_t i = 0; i < table.Count(); i++) if (table[i] == m) return i;
    return -1;
}

cs::Ref<Monster> CompanionRef::Def(cs::Ref<Companion> c, cs::List<cs::Ref<Monster>> table) {
    return At(table, Resolve(c, table));
}

cs::Ref<Companion> CompanionRef::Of(cs::Ref<Monster> def) {
    return [&] { auto _o = cs::New<Companion>(); _o->Ref = def->Ref; _o->MonsterId = def->Id; return _o; }();
}

int32_t CompanionRef::Resolve(cs::Ref<Companion> c, cs::List<cs::Ref<Monster>> table) {
    if (c->Ref >= 0) {
        auto at = At(table, c->Ref);
        if (at != nullptr && at->Id == c->MonsterId) return c->Ref;
        // a record given its Ref and no Id at all: the Ref stands rather than being sent to the creature of Id 0
        if (at != nullptr && c->MonsterId == 0) return c->Ref;
        int32_t first = FirstWithId(table, c->MonsterId);
        return first >= 0 ? first : c->Ref;
    }
    return FromId(table, c->MonsterId, c->Alignment);
}

int32_t CompanionRef::FromId(cs::List<cs::Ref<Monster>> table, int16_t id, ItemAlignment alignment) {
    auto withId = cs::List<int32_t>::New();
    for (int32_t i = 0; i < table.Count(); i++)
        if (table[i]->Id == id) withId.Add(IndexOf(table, table[i]));
    if (withId.Count() <= 1) return withId.Count() == 1 ? withId[0] : -1;
    if (alignment != ItemAlignment::Unaligned) {
        auto same = withId.Where([&](auto& r) {
            auto m = At(table, r);
            return m != nullptr && Confinement::AlignmentOf(m) == alignment;
        }).ToList();
        if (same.Count() == 1) return same[0];
    }
    return withId[0];
}

int32_t CompanionRef::FirstWithId(cs::List<cs::Ref<Monster>> table, int16_t id) {
    for (int32_t i = 0; i < table.Count(); i++)
        if (table[i]->Id == id) return IndexOf(table, table[i]);
    return -1;
}

int32_t CompanionRef::Settle(cs::Ref<CompanionSlots> slots, cs::List<cs::Ref<Monster>> table) {
    int32_t n = 0;
    for (auto& _t : slots->Occupied()) {
        auto c = _t.Companion;
        bool changed = false;
        int32_t r = Resolve(c, table);
        if (r != c->Ref) { c->Ref = r; changed = true; }
        if (c->Alignment == ItemAlignment::Unaligned) {
            auto def = At(table, c->Ref);
            if (def != nullptr) {
                c->Alignment = Confinement::AlignmentOf(def);
                changed = true;
            }
        }
        if (changed) n++;
    }
    return n;
}

}  // namespace mc
