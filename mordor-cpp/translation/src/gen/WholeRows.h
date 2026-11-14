#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct WholeRows {
    static double Tail(double viewport, double topMargin, double rowHeight);
    static double Snap(double offset, double rowHeight, double max);
};

}  // namespace mc
