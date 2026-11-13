// partyb.cpp
// Party roster rules: join, leave, pooled gold, selection, combat settings,
// autoload, city join, bodies in the city, give-gold, and monster target choice.
// SRD references (c10..c59) are the source-note rule numbers from the engine notes.
#include "mordor/partyb.h"

#include "mordor/math_util.h"
#include "mordor/rng.h"

// HEADER BUG (partyb.h): the declaration
//     GoldTransferOutcome transferGold(Ctx&, Character&, Character&, Currency, bool);
// uses a type that is not declared in partyb.h, the state headers, types.h, ctx.h
// or the API index. A function with an undeclared return type cannot be defined, so
// transferGold is NOT defined in this file. Once the type is supplied (or the header
// is corrected), add the definition using the SRD logic below (c16, Exchange/transfer):
//     if amount == 0: return
//     available = inDungeon ? from.onhand : (from.onhand + from.bank)
//     if amount > available: refuse "Too Much!"
//     to.onhand += amount; from.onhand -= amount
//     if from.onhand < 0: from.bank += from.onhand; from.onhand = 0
// No negative-amount check is in the SRD text, so a negative amount runs backwards
// (documented original quirk, c16).
//
// NOTE: giveItemToMember takes no Inventory, so the empty-slot and Equipped checks
// cannot be made here; they belong to the items module.

namespace mordor {

namespace {

constexpr i64 kCurrencyMax = 9223372036854775807LL;
constexpr i64 kCurrencyMin = -9223372036854775807LL - 1;

}  // namespace

// ---------------------------------------------------------------------------
// Help cry (c11)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Monster target choice (c14)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Joining and alignment (c16)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Pooling gold (c16, c43, c48)
// ---------------------------------------------------------------------------

bool poolGoldToSelected(Ctx& ctx, Party& party) {
    (void)ctx;
    // R24: pool each other member's on-hand gold, in index order, into the selected member.
    // Stop at the first member whose full amount would overflow the selected member's cap;
    // that member and all later members keep their gold.
    // UNKNOWN: SRD does not say whether a partial amount is moved before stopping; whole-member only.
    // FIXED: memberAt is not declared anywhere; use direct member indexing.
    Character& sel = party.members[party.selectedIndex];
    for (i32 i = 0; i < party.size; ++i) {
        if (i == party.selectedIndex) continue;
        Character& m = party.members[i];
        const Currency g = m.wallet.goldOnHand;
        if (g <= 0) continue;
        // FIXED: kBalanceCap is not declared anywhere; use the local kCurrencyMax cap.
        if (g > kCurrencyMax - sel.wallet.goldOnHand) {
            return false;  // overflow: stop pooling
        }
        sel.wallet.goldOnHand += g;
        m.wallet.goldOnHand -= g;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Roster changes
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Combat orders and settings (c54)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Autoload (c16)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// City join (c16, c48)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Give gold and items (c10, c29)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Leaving and joining details (c16, c43)
// ---------------------------------------------------------------------------

}  // namespace mordor
