#include "core.h"

namespace mc {

FramePause_Change FramePause::Transition(bool wasMinimised, bool isMinimised) {
    if (isMinimised && !wasMinimised) { Paused = true; return FramePause_Change::Paused; }
    if (wasMinimised && !isMinimised) { Paused = false; return FramePause_Change::Resumed; }
    return FramePause_Change::None;
}

}  // namespace mc
