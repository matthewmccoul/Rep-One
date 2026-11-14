#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GoldShare {
    static cs::String Line(int64_t foundPlusRecovered, int64_t recoveredByViewer, int32_t partySize);
    static VbCurrency Shown(int64_t pot, int32_t partySize);
    static int64_t Share(int64_t pot, int32_t partySize);

    inline static const cs::String DeadNoGold = "You didn't find any Gold!";
    inline static const cs::String DeadOnlyRecovered = "You didn't find any gold, but {0} Gold was recovered!";
};

}  // namespace mc
