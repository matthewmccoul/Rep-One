// partyb_missing2.cpp
// Implements transferGold from partyb.h.
#include "mordor/partyb.h"

namespace mordor {

// Moves gold from one character to another.
// The SRD gives no rule number for this formula; it is the preserved gold-transfer block.
// Return codes (UNKNOWN: the SRD does not define a return value; these are ASSUMED):
//   0  = nothing moved (amount == 0)
//   1  = moved
//  -1  = refused ("Too Much!")
i32 transferGold(Ctx& ctx,
                 Character& from,
                 Character& to,
                 Currency amount,
                 bool inDungeon) {
    // FIXED: kBalanceCap is not declared in any header. Defined locally as the
    // 32-bit signed maximum (ASSUMED value; the reference cap is not in the index).
    constexpr Currency kBalanceCap = static_cast<Currency>(2147483647);

    // if amount == 0: return
    if (amount == 0) {
        return 0;
    }

    // purse = onhand + bank
    // available = (dungeon ? onhand : purse)
    Currency purse = from.purse();
    Currency available = inDungeon ? from.wallet.goldOnHand : purse;

    // if amount > available: "Too Much!"
    if (amount > available) {
        ctx.say("partyb.gold.tooMuch", "Too Much!", {}, {});
        return -1;
    }

    // R24 / R64: receiver balance cap. The reference does `to.GoldOnHand += amount` first;
    // on overflow the trap resumes silently with nothing moved on either side.
    // Checked before the sender is touched, so no partial transfer.
    // CONFLICT-free for Give (R64); for Exchange the SRD is silent, see erratum ch14.
    if (amount > 0 && to.wallet.goldOnHand > kBalanceCap - amount) {
        return 0;
    }

    // to.GoldOnHand += amount
    // from.GoldOnHand -= amount
    // QUIRK (kept on purpose): a negative amount skips the check above, so the
    // recipient pays and may go below zero, and the sender gains gold. Only the
    // sender is clamped below.
    to.wallet.goldOnHand += amount;
    from.wallet.goldOnHand -= amount;

    // if from.GoldOnHand < 0: from.GoldInBank += from.GoldOnHand; from.GoldOnHand = 0
    if (from.wallet.goldOnHand < 0) {
        from.wallet.goldInBank += from.wallet.goldOnHand;
        from.wallet.goldOnHand = 0;
    }

    return 1;
}

}  // namespace mordor
