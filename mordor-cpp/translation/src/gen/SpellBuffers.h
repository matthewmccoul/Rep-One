#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SpellBuffers {
    static constexpr int32_t Count = 10;
    static constexpr int32_t PackSlots = 40;
    static constexpr int32_t Empty = -1;

    static int32_t ForItem(int32_t packSlot);
    static int32_t ForSpell(int32_t spellIndex);
    static bool IsEmpty(int32_t buffer);
    static bool IsItem(int32_t buffer);
    static bool IsSpell(int32_t buffer);
    static int32_t PackSlotOf(int32_t buffer);
    static int32_t SpellIndexOf(int32_t buffer);
    static void PackSlotEmptied(cs::Ref<Character> c, int32_t slot);
    static void PackSlotsSwapped(cs::Ref<Character> c, int32_t a, int32_t b);
    static cs::String Describe(cs::Ref<Character> c, int32_t slot, cs::List<cs::Ref<Spell>> spells, cs::Func<cs::Ref<Item>(int16_t)> item);
    static cs::String CostText(cs::Ref<Character> c, int32_t slot, cs::List<cs::Ref<Spell>> spells);
};

}  // namespace mc
