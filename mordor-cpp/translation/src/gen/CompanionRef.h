#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionRef {
    static cs::Ref<Monster> At(cs::List<cs::Ref<Monster>> table, int32_t r);
    static int32_t IndexOf(cs::List<cs::Ref<Monster>> table, cs::Ref<Monster> m);
    static cs::Ref<Monster> Def(cs::Ref<Companion> c, cs::List<cs::Ref<Monster>> table);
    static cs::Ref<Companion> Of(cs::Ref<Monster> def);
    static int32_t Resolve(cs::Ref<Companion> c, cs::List<cs::Ref<Monster>> table);
    static int32_t FromId(cs::List<cs::Ref<Monster>> table, int16_t id, ItemAlignment alignment);
    static int32_t FirstWithId(cs::List<cs::Ref<Monster>> table, int16_t id);
    static int32_t Settle(cs::Ref<CompanionSlots> slots, cs::List<cs::Ref<Monster>> table);
};

}  // namespace mc
