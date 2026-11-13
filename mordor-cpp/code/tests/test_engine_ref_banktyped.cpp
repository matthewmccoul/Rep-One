#include "test_support.h"
#include "mordor/ref_banktyped.h"

using namespace mordor;

namespace {

// Currency is 1/10000 gold: 1 gold = 10000, 0.25 = 2500, 99.75 = 997500, 100 = 1000000.
Character withPurse(Currency hand, Currency bank) {
    Character c;
    c.wallet.goldOnHand = hand;
    c.wallet.goldInBank = bank;
    return c;
}

}  // namespace

