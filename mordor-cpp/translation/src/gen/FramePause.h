#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct FramePause {
    using Change = FramePause_Change;
    inline static bool Paused = false;
    static FramePause_Change Transition(bool wasMinimised, bool isMinimised);
};

}  // namespace mc
