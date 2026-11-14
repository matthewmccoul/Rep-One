#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct MazeSelect {
    using View = MazeSelect_View;

    static void Prune(cs::Ref<MazeSelect_View> v);
    static void Select(cs::Ref<MazeSelect_View> v);

    // const values are declared in MazeSelect.cs; defined in that file's .cpp
    static constexpr int32_t MapWidth = 30;   // MazeSelect.cs
    static constexpr int32_t mc00D2 = 1;   // MazeSelect.cs
    static constexpr int32_t mc00D4 = 2;   // MazeSelect.cs
    static constexpr int32_t mc00D6 = 3;   // MazeSelect.cs
    static constexpr int32_t mc00D8 = 4;   // MazeSelect.cs
    static constexpr int32_t mc022C = 1;   // MazeSelect.cs
    static constexpr int32_t mc022E = 2;   // MazeSelect.cs
    static constexpr int32_t gv1D16 = 0;   // MazeSelect.cs
    static constexpr int32_t gc1D18 = 1;   // MazeSelect.cs
    static constexpr int32_t gc1D1A = 2;   // MazeSelect.cs
    static constexpr int32_t gc1D1C = 3;   // MazeSelect.cs
    static constexpr int32_t gc1D1E = 4;   // MazeSelect.cs
    static constexpr int32_t gc1D20 = 5;   // MazeSelect.cs
    static const int32_t SlotLeft;
    static const int32_t SlotRight;

    static int32_t Bit(int32_t bit, int32_t word);
    static int32_t SetBit(int32_t bit, int32_t word, int32_t set);
    static int32_t WallsAt(cs::Ref<MazeSelect_View> v, int32_t square);
    static cs::Ref<MazeSelect_View> For(int32_t x, int32_t y, int32_t facing, int32_t level, cs::Func<int32_t(int32_t)> walls);
};

}  // namespace mc
