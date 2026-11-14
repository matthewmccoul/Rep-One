#include "core.h"

namespace mc {

double WholeRows::Tail(double viewport, double topMargin, double rowHeight) {
    if (rowHeight <= 0 || viewport <= 0) return 0;
    double tail = std::fmod(viewport - topMargin, rowHeight);
    if (tail < 0) tail += rowHeight;
    // a hair short of a row is a whole row, not a row's worth of room
    return rowHeight - tail < 0.01 ? 0 : cs::Math::Round(tail, 2);
}

double WholeRows::Snap(double offset, double rowHeight, double max) {
    if (rowHeight <= 0) return offset;
    return cs::Math::Clamp(cs::Math::Round(offset / rowHeight) * rowHeight, 0.0, cs::Math::Max(0.0, max));
}

}  // namespace mc
