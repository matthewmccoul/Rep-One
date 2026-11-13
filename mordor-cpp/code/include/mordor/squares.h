#pragma once
// One checked square lookup on a floor: 1-based x and y, square = x + 30 * (y - 1) (SRD R19).
// Returns nullptr off the map. The const form is for read-only callers such as the view.
#include "mordor/state_dungeon.h"

namespace mordor {
inline SquareState* squareAt(FloorState& floor, i32 x, i32 y) {
    constexpr i32 kWidth = 30;
    if (x < 1 || y < 1 || x > kWidth) return nullptr;
    const i32 idx = (x - 1) + kWidth * (y - 1);
    if (idx < 0 || idx >= static_cast<i32>(floor.squares.size())) return nullptr;
    return &floor.squares[static_cast<u32>(idx)];
}
inline const SquareState* squareAt(const FloorState& floor, i32 x, i32 y) {
    return squareAt(const_cast<FloorState&>(floor), x, y);
}
}  // namespace mordor
