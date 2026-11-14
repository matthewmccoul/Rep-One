#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct HelpCry {
    static bool Needed(cs::Ref<Character> c);
    static cs::String Line(cs::Ref<Character> c);
    static cs::String Paralysis(cs::Ref<Character> c);
    inline static const cs::String ParalysisWatched = "You have been Paralyzed!";
    inline static const cs::String Sound = "HELP";
};

}  // namespace mc
