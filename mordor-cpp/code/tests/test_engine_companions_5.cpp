#include "test_support.h"
#include "mordor/spells1a.h"

#include <cmath>
#include <cstdlib>

#include "mordor/companions1a.h"
#include "mordor/companions2a.h"
#include "mordor/state_party.h"

using namespace mordor;

namespace {

struct MishapTally {
    i32 none = 0;  // draws with no mishap
    i32 v0 = 0;    // mishaps landing on member 0
    i32 v1 = 0;    // mishaps landing on member 1 (owner)
    i32 v2 = 0;    // mishaps landing on member 2
};

i32 expectedBreathRange(i32 attackerLevel, i32 targetLevel) {
    const f64 a = static_cast<f64>(attackerLevel + 1);
    const f64 v = (std::log(a) + a / kBreathSeedDivisor) / std::log(kBreathLogBase)
                  - static_cast<f64>(targetLevel) / 10.0 + 1.0;
    return static_cast<i32>(vbInt(v));
}

}  // namespace

