#include "core.h"

namespace mc {

bool Effects::Counts(cs::Ref<Character> c, cs::Ref<InventorySlot> slot) { return slot->Equipped && c->ItemHelps(slot); }

int32_t Effects::GearEffects(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item) {
    int32_t mask = 0;
    for (auto& slot : c->Inventory()) {
        if (!Counts(c, slot)) continue;
        auto it = item(slot->ItemId);
        if (it == nullptr || it->Abilities <= 0) continue;
        for (int32_t bit = 0; bit < EffectBits; bit++)
            if ((it->Abilities & (1 << bit)) != 0) mask |= 1 << bit;
    }
    return mask;
}

int32_t Effects::ResistancePercent(cs::Ref<Character> c, int32_t kind, cs::Func<cs::Ref<Item>(int16_t)> item) {
    auto _race = c->RaceDef();
    auto baseline = _race ? _race->Resistances : decltype(_race->Resistances){};
    int32_t value = (baseline != nullptr && kind < baseline.Length()) ? baseline[kind] : 0;

    for (auto& slot : c->Inventory()) {
        if (!Counts(c, slot)) continue;
        auto it = item(slot->ItemId);
        if (it != nullptr && (it->ResistanceFlags & (1 << kind)) != 0) { value = GearResistance; break; }
    }

    if (kind < c->SpellResist.Length() && c->SpellResist[kind] > value) value = c->SpellResist[kind];
    return value;
}

void Effects::Recompute(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item) {
    c->Levitating = false;
    c->InvisibleNow = false;
    c->SeeingInvisible = false;
    c->Protection = 0;

    int32_t gear = GearEffects(c, item);
    int32_t spell = c->SpellEffects;

    for (int32_t bit = 0; bit < EffectBits; bit++) {
        if ((gear & (1 << bit)) == 0 && (spell & (1 << bit)) == 0) continue;
        switch (bit) {
            case Levitate: c->Levitating = true; break;
            case Invisible: c->InvisibleNow = true; break;
            case Protect: c->Protection = c->Guild()->Level / 2 + 1; break;
            case SeeInvisible: c->SeeingInvisible = true; break;
            case CriticalHit: break;  // an empty case in the original
            default: break;
        }
    }

    if ((spell & (1 << SpellLevitate)) != 0) c->Levitating = true;
    if ((spell & (1 << SpellInvisible)) != 0) c->InvisibleNow = true;
    if ((spell & (1 << SpellProtect)) != 0) c->Protection = c->Guild()->Level / 2 + 1;
    if ((spell & (1 << SpellSeeInvisible)) != 0) c->SeeingInvisible = true;
}

void Effects::ClearSpellEffects(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item) {
    c->SpellEffects = 0;
    for (int32_t i = 0; i < c->SpellResist.Length(); i++) c->SpellResist[i] = 0;
    Recompute(c, item);
}

}  // namespace mc
