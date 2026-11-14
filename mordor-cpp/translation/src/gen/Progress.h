#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Progress {
    using NeedState = Progress_NeedState;
    using QuestState = Progress_QuestState;

    static Tup_State_Amount_0a0cd ToolbarNeed(cs::Ref<Character> c);
    static cs::String NextLevel(cs::Ref<Character> c);
    static cs::String Quest(cs::Ref<Character> c, cs::Func<cs::Ref<Monster>(int16_t)> monster, cs::Func<cs::Ref<Item>(int16_t)> item);
    static Progress_QuestState QuestStateOf(cs::Ref<Character> c);
};

}  // namespace mc
