// items1a_c.cpp: implementation of the SRD c13p1 (half c) item rules.
// Functions defined here: swapBufferSlots, giveRead, lootReadOnReceive,
// storeCombine, storeBuy, realignCost, realignMultiplier, uncurseItem,
// isGuildCrest, sellQuote, parseTypedAmount, bankDepositAllowed,
// bankWithdrawAllowed, depositItem, retrieveFromBank,
// bankRetrievalReidentifies, slimeRollReaches, pickItemSlot, isDestructibleItem.

#include "mordor/items1a.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {
// UNKNOWN: VB Integer limit (32767) assumed as the charge overflow point (R56).
constexpr i64 kChargeSumLimit = 32767;
}  // namespace

// R62 guild crest test.
// UNKNOWN: crest identification rule is not in this SRD half. Crests are assumed to be
// items whose id lies in (kCrestItemIdBase, kCrestItemIdBase + kGuildCount].
bool isGuildCrest(const Item& item) {
    return item.id > kCrestItemIdBase && item.id <= kCrestItemIdBase + kGuildCount;
}

// R62 sell quote: crests are refused before a price is quoted.
bool sellQuote(Ctx& ctx, const Item& item, Currency& quote) {
    if (isGuildCrest(item)) {
        return false;
    }
    quote = sellPrice(ctx, item);
    return true;
}

}  // namespace mordor
