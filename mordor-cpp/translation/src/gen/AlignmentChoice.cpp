#include "core.h"

namespace mc {

bool AlignmentChoice_Offer::Asks() { return Stocked > 1; }

int32_t AlignmentChoice_Offer::Chosen(int32_t before) { return Asks() ? -1 : Stocked == 1 ? Last : before; }

AlignmentChoice_Offer AlignmentChoice::For(cs::List<int32_t> counts) {
    auto enabled = cs::List<bool>::New(counts.Count());
    int32_t stocked = 0, last = -1;
    for (int32_t a = 0; a < counts.Count(); a++) {
        enabled[a] = counts[a] > 0;
        if (enabled[a]) { stocked++; last = a; }
    }
    return AlignmentChoice_Offer(enabled, stocked, last);
}

}  // namespace mc
