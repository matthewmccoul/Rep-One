#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct RoomRoster {
    static cs::String DeadBodies(int32_t count);
    static cs::String Strangers(int32_t count);
    static cs::List<cs::String> Lines(int32_t living, int32_t dead, int32_t carrying, int32_t partySize);
};

}  // namespace mc
