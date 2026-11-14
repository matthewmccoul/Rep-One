#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SwapCommand {
    static cs::Nullable<Tup_A_B_cbb7a> Parse(cs::String typed);

    inline static const cs::String Done = "Item swap completed!";
};

}  // namespace mc
