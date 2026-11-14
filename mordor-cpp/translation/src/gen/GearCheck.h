#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GearCheck {
    inline static const cs::String UnequippedOnJoining = "GUILD: One or more items were unequipped when you joined...";

    static cs::String NoLongerUsable(cs::Ref<Character> who, cs::String itemName, bool inAParty);
    static cs::List<cs::String> Run(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> lookup, bool inAParty, bool announce);
    static bool Any(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> lookup);
};

}  // namespace mc
