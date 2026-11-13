#include "test_support.h"
#include "mordor/ref_bankalltext.h"

// B_0055#78: Party.BankAllText applied to 89.5 on hand -> "89.5".
TEST(engine_ref_bankalltext_on_hand_89_5) {
    mordor::Str r = mordor::bankalltext(895000);
    CHECK(r == mordor::Str("89.5"));
}

// B_0055#79: Party.BankAllText applied to a balance of 0.25 -> ".25".
TEST(engine_ref_bankalltext_balance_0_25) {
    mordor::Str r = mordor::bankalltext(2500);
    CHECK(r == mordor::Str(".25"));
}

// B_0055#80: SKIP B_0055#80: the case deposits through Party.BankTyped and reports BankAnswer.Moved with balances
// 0/100. The BankTyped source and the BankAnswer enum are not in the provided reference excerpt or our types, so the
// deposit outcome and balance mutation cannot be expressed yet.
