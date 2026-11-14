#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CharacterStates {
    inline static cs::List<cs::String> Names = cs::List<cs::String>::Of({
        "Poisoned", "Diseased", "Paralyzed", "Levitating", "Invisible", "See Invisible", "Protected",
    });

    static cs::List<cs::String> Showing(cs::Ref<Character> c);
};

}  // namespace mc
