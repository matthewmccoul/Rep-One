#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct DeathStates {
    inline static cs::List<cs::String> Names = cs::List<cs::String>::Of({"Dead", "Stoned", "Destroyed", "Immolated", "Frozen", "Annihalated", "Melted", "Rock"});

    static cs::String Name(int32_t causeOfDeath);
};

}  // namespace mc
