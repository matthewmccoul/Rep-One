#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Asmodeus {
    static constexpr int32_t MonsterId = 277;
    static bool Is(cs::Ref<Monster> m);
    inline static const cs::String Sound = "ASMO";
    static Tup_low_top_4e748 DropBand(cs::Ref<Monster> m);
    static cs::String Ending(bool inAParty);
    inline static const cs::String EndingTitle = "A dark realization...";
    static cs::String LogLine(cs::Ref<Character> c, bool inAParty);
};

}  // namespace mc
