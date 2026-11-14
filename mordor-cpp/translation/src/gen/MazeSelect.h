#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct MazeSelect_View {
    cs::List<int32_t> V = cs::List<int32_t>::New(26);
    cs::List<int32_t> Slot = cs::List<int32_t>::New(9);
    cs::List<int32_t> Bg = cs::List<int32_t>::New(2);
    int32_t X = 0;
    int32_t Y = 0;
    int32_t Facing = 0;
    int32_t Level = 0;
    cs::Func<int32_t(int32_t)> Walls = [](int32_t) { return 0; };
    int32_t Square();
};

}  // namespace mc
