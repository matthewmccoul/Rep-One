#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct PlayClock {
    static cs::Ref<PlayClock> Shared;

    cs::List<float> _started = cs::List<float>::New();   // gv1B1E

    static float Timer(cs::DateTime now);

    void Tick(cs::List<cs::Ref<Character>> party, float timer);
};

}  // namespace mc
