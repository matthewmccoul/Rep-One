#include "test_support.h"
#include "mordor/items1b.h"
#include "mordor/partya.h"
#include "mordor/partyb.h"
#include "mordor/state_party.h"
#include "mordor/state_character.h"

using namespace mordor;

namespace {

constexpr Currency kMaxV = 9223372036854775807LL;  // MaxValue in Currency units

// FIXED: kBalanceCap is not declared in any header; use the closest existing value (MaxValue).
constexpr Currency kBalanceCap = kMaxV;

// Gold (whole units) to Currency (1/10000).
Currency g(i64 gold) {
    return gold * 10000;
}

// Set one member's purse and make them able to act.
void seat(Party& p, i32 idx, Currency hand, Currency bank) {
    Character& c = p.members[idx];
    c.wallet.goldOnHand = hand;
    c.wallet.goldInBank = bank;
    c.vitals.curHits = 10;
    c.vitals.maxHits = 10;
}

}  // namespace

// SKIP B_0056#30: LookAround.ListedParty.Sync has no equivalent in the headers (UI look page).
// SKIP B_0056#31: LookAround.ListedParty.Sync has no equivalent in the headers (UI look page).
// SKIP B_0056#32: LookAround.ListedParty.Sync has no equivalent in the headers (UI look page).
