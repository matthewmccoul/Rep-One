#include "mordor/city2b.h"
#include "mordor/city1.h"
#include "mordor/ctx.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {
// T-L Fee base (seer)
constexpr double kSeerFeeBase = 132567.52;
// T-L Level gate (original) 12, FRMSEEK.BAS:90, 237
constexpr i32 kSeerLevelGate = 12;
// T-J Buy base floor: max(BaseValue, 10)
constexpr i32 kBuyFloor = 10;
// T-R companion price band table length
constexpr i32 kCompanionBandCount =
    static_cast<i32>(sizeof(kCompanionPriceBands) / sizeof(kCompanionPriceBands[0]));
}  // namespace

}  // namespace mordor
