#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CompanionSpit {
    static constexpr int32_t SlotsWalked = 4;

    static int32_t Slot(cs::Ref<cs::Random> rng, cs::Ref<Character> of);
};

}  // namespace mc
