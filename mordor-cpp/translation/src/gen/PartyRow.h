#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct PartyRow {
    static cs::String Status(cs::Ref<Character> c);
    static bool StatusIsWell(cs::Ref<Character> c);
};

}  // namespace mc
