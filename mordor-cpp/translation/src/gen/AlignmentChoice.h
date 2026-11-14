#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct AlignmentChoice_Offer {
    cs::List<bool> Enabled;
    int32_t Stocked = 0;
    int32_t Last = 0;

    AlignmentChoice_Offer() = default;
    AlignmentChoice_Offer(cs::List<bool> Enabled, int32_t Stocked, int32_t Last) : Enabled(Enabled), Stocked(Stocked), Last(Last) {}

    bool Asks();
    int32_t Chosen(int32_t before);

    bool operator==(const AlignmentChoice_Offer& o) const { return Enabled == o.Enabled && Stocked == o.Stocked && Last == o.Last; }
    bool operator!=(const AlignmentChoice_Offer& o) const { return !(*this == o); }
};

struct AlignmentChoice {
    using Offer = AlignmentChoice_Offer;

    static AlignmentChoice_Offer For(cs::List<int32_t> counts);
};

}  // namespace mc
