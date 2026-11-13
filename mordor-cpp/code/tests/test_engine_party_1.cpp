#include "test_support.h"
#include "mordor/partya.h"
#include "mordor/partyb.h"

using namespace mordor;

namespace {

// Currency = gold * 10000.
Character withGold(Currency hand, Currency bank) {
    Character c{};
    c.wallet.goldOnHand = hand;
    c.wallet.goldInBank = bank;
    return c;
}

Party twoMembers(const Character& a, const Character& b) {
    Party p{};
    p.members[0] = a;
    p.members[1] = b;
    p.size = 2;
    return p;
}

}  // namespace

// SKIP B_0055#103: GoldFromTag parses a text tag; no equivalent function in the headers.

TEST(engine_B_0055_109_pool_gold_alt_p) {
    tsup::Env env({0.5, 0.9});
    Party p = twoMembers(withGold(0, 0),    // ch, 0 on hand
                         withGold(1, 0));   // other, 0.0001 on hand
    p.selectedIndex = 0;

    poolGoldToSelected(env.ctx, p);

    CHECK_EQ(p.members[0].wallet.goldOnHand, 1);  // ch on-hand 0.0001
    CHECK_EQ(p.members[1].wallet.goldOnHand, 0);  // other emptied; pooled 0.0001
}

