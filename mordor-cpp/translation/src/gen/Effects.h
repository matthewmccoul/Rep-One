#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Effects {
    static constexpr int32_t ResistanceKinds = 11;

    static constexpr int32_t EffectBits = 6;

    static constexpr int32_t GearResistance = 90;

    static constexpr int32_t Levitate = 0;
    static constexpr int32_t Invisible = 1;
    static constexpr int32_t Protect = 2;
    static constexpr int32_t SeeInvisible = 3;
    static constexpr int32_t CriticalHit = 4;
    static constexpr int32_t Backstab = 5;

    static constexpr int32_t SpellLevitate = 3;
    static constexpr int32_t SpellInvisible = 4;
    static constexpr int32_t SpellSeeInvisible = 5;
    static constexpr int32_t SpellProtect = 6;

    static bool Counts(cs::Ref<Character> c, cs::Ref<InventorySlot> slot);
    static int32_t GearEffects(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item);
    static int32_t ResistancePercent(cs::Ref<Character> c, int32_t kind, cs::Func<cs::Ref<Item>(int16_t)> item);
    static void Recompute(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item);
    static void ClearSpellEffects(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item);
};

}  // namespace mc
