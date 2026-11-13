// items1a_d.cpp: SRD c13p1 half d (R68-R92).
// Slime and monster destroy, theft, quicksand slot, Seer carrier and take,
// identify/recognise/rename chances, gear effect bits, and item prices.
#include <cstddef>
#include <string>

#include "mordor/items1a.h"
#include "mordor/store_prices.h"
#include "mordor/items1b.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

// UNKNOWN: destroy chance formula is not in the SRD (R71 says "checked against the destroy chance", R90 says Formulas). Placeholder 50 percent.
constexpr i32 kDestroyChancePlaceholder = 50;

}  // namespace

// R92 prices are Longs rounded half to even, not floored. The formulas are not given.
// UNKNOWN: no price formula in the SRD part. Placeholder 0 gold, passed through roundHalfEven as the rule requires.
namespace {
}  // namespace

Currency identifyFee(Ctx& ctx, const Item& item) {
    return storeIdentifyFee(ctx.data, item, 1);   // store-less default; the game layer passes the shelf count
}

Currency sellPrice(Ctx& ctx, const Item& item) {
    return storeSellPrice(ctx.data, item, 1, true);   // store-less default; the game layer passes the shelf counts
}

// FIXED: list-price overload used by uncurseFee below. Defined in items2b_2.cpp; it is not in any header in the API index.
Currency uncurseFee(Currency listPrice);

}  // namespace mordor
