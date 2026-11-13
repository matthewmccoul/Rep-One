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

// B_0055#77: deposit 10.5 with 100 on hand -> Moved 89.5/10.5
TEST(engine_ref_banktyped_deposit_10_5) {
    Character c = withPurse(1000000, 0);
    BankAnswer r = bankTyped(c, Str("10.5"), true);
    CHECK(r == BankAnswer::Moved);
    CHECK_EQ(c.wallet.goldOnHand, 895000);
    CHECK_EQ(c.wallet.goldInBank, 105000);
}

// B_0055#81: withdraw .25 from 100 banked -> Moved 0.25/99.75
TEST(engine_ref_banktyped_withdraw_quarter) {
    Character c = withPurse(0, 1000000);
    BankAnswer r = bankTyped(c, Str(".25"), false);
    CHECK(r == BankAnswer::Moved);
    CHECK_EQ(c.wallet.goldOnHand, 2500);
    CHECK_EQ(c.wallet.goldInBank, 997500);
}

// B_0055#82: withdraw 0.00004 rounds to zero -> Ignored 0.25/99.75
TEST(engine_ref_banktyped_withdraw_rounds_to_zero) {
    Character c = withPurse(2500, 997500);
    BankAnswer r = bankTyped(c, Str("0.00004"), false);
    CHECK(r == BankAnswer::Ignored);
    CHECK_EQ(c.wallet.goldOnHand, 2500);
    CHECK_EQ(c.wallet.goldInBank, 997500);
}

// B_0055#83: non-numeric text as a deposit -> Ignored 0.25/99.75
TEST(engine_ref_banktyped_deposit_text_ignored) {
    Character c = withPurse(2500, 997500);
    BankAnswer r = bankTyped(c, Str("lots"), true);
    CHECK(r == BankAnswer::Ignored);
    CHECK_EQ(c.wallet.goldOnHand, 2500);
    CHECK_EQ(c.wallet.goldInBank, 997500);
}

// B_0055#84: deposit 0.2501 with 0.25 on hand -> Beep 0.25/99.75
TEST(engine_ref_banktyped_deposit_over_hand_beeps) {
    Character c = withPurse(2500, 997500);
    BankAnswer r = bankTyped(c, Str("0.2501"), true);
    CHECK(r == BankAnswer::Beep);
    CHECK_EQ(c.wallet.goldOnHand, 2500);
    CHECK_EQ(c.wallet.goldInBank, 997500);
}

// B_0055#85: deposit -5 -> Beep 0.25/99.75
TEST(engine_ref_banktyped_deposit_negative_beeps) {
    Character c = withPurse(2500, 997500);
    BankAnswer r = bankTyped(c, Str("-5"), true);
    CHECK(r == BankAnswer::Beep);
    CHECK_EQ(c.wallet.goldOnHand, 2500);
    CHECK_EQ(c.wallet.goldInBank, 997500);
}

// B_0055#86: unchecked withdrawal of -5 -> Moved -4.75/104.75
TEST(engine_ref_banktyped_withdraw_negative_unchecked) {
    Character c = withPurse(2500, 997500);
    BankAnswer r = bankTyped(c, Str("-5"), false);
    CHECK(r == BankAnswer::Moved);
    CHECK_EQ(c.wallet.goldOnHand, -47500);
    CHECK_EQ(c.wallet.goldInBank, 1047500);
}

// B_0055#87: withdraw 1, then set bank to -1, then withdraw 1 -> (Moved, True, Beep)
TEST(engine_ref_banktyped_withdraw_after_bank_negative) {
    Character c = withPurse(-47500, 1047500);
    BankAnswer first = bankTyped(c, Str("1"), false);
    CHECK(first == BankAnswer::Moved);
    c.wallet.goldInBank = -10000;  // set banked figure to -1
    CHECK(c.wallet.goldInBank == -10000);
    BankAnswer second = bankTyped(c, Str("1"), false);
    CHECK(second == BankAnswer::Beep);
}

// B_0055#128: unchecked withdrawal of -100 at the bank limit. Exceptions cannot escape
// in this port, so only the balances are checked.
// Expected: False -100/922337203685467.5807
TEST(engine_ref_banktyped_withdraw_overflow_keeps_bank) {
    Character c = withPurse(0, 9223372036854675807LL);
    bankTyped(c, Str("-100"), false);
    CHECK_EQ(c.wallet.goldOnHand, -1000000);
    CHECK_EQ(c.wallet.goldInBank, 9223372036854675807LL);
}

// B_0055#129: deposit 50 into a bank already at MaxValue -> Moved 50/922337203685477.5807
TEST(engine_ref_banktyped_deposit_into_full_bank) {
    Character c = withPurse(1000000, 9223372036854775807LL);
    BankAnswer r = bankTyped(c, Str("50"), true);
    CHECK(r == BankAnswer::Moved);
    CHECK_EQ(c.wallet.goldOnHand, 500000);
    CHECK_EQ(c.wallet.goldInBank, 9223372036854775807LL);
}
