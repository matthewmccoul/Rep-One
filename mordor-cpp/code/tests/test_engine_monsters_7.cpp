#include "test_support.h"
#include "mordor/monsters1a.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"

using namespace mordor;

namespace {

Arr<Companion, kCompanionSlots> fourOccupiedCompanions() {
    Arr<Companion, kCompanionSlots> comps{};
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        comps[i].occupied = true;
    }
    return comps;
}

}  // namespace

// B_0063#10: drain monster stat drop count over 1500 rounds (Strength)
// SKIP B_0063#10: needs a multi-round combat simulation with MDATA drain monster setup; no single stat-drain function in headers
// B_0063#11: stat index 1 drop count over 1500 rounds
// SKIP B_0063#11: needs a multi-round combat simulation with MDATA drain monster setup; no single stat-drain function in headers
// B_0063#12: stat index 2 drop count over 1500 rounds
// SKIP B_0063#12: needs a multi-round combat simulation with MDATA drain monster setup; no single stat-drain function in headers

