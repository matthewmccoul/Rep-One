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

// ---- B_0055#133 ----------------------------------------------------------
// Pool & Deposit with A (command, 5 on hand, bank MaxValue-10) and B (100, 0).
// Overflow rolls back every balance touched.
TEST(engine_B_0055_133_pool_deposit_overflow) {
    tsup::Env env(1);
    Party party;
    party.size = 2;
    seat(party, 0, g(5), kMaxV - 10);   // A
    seat(party, 1, g(100), Currency(0)); // B
    party.selectedIndex = 0;

    BankPartyOutcome out =
        bankPartyOption(env.ctx, party, BankPartyOption::PartyPoolAndDeposit);
    CHECK(out == BankPartyOutcome::RolledBack);
    CHECK_EQ(party.members[0].wallet.goldOnHand, g(5));
    CHECK_EQ(party.members[0].wallet.goldInBank, kMaxV - 10);
    CHECK_EQ(party.members[1].wallet.goldOnHand, g(100));
    CHECK_EQ(party.members[1].wallet.goldInBank, Currency(0));
}

// ---- B_0055#134 ----------------------------------------------------------
// Pool to bank with B leading and A second; overflow -> False, balances kept.
TEST(engine_B_0055_134_pool_to_bank_overflow_b_first) {
    tsup::Env env(1);
    Party party;
    party.size = 2;
    seat(party, 0, g(100), Currency(0)); // B (leader)
    seat(party, 1, g(5), kMaxV - 10);    // A
    party.selectedIndex = 1;

    bool ok = poolGoldToBank(env.ctx, party);
    CHECK(!ok);
    CHECK_EQ(party.members[1].wallet.goldOnHand, g(5));
    CHECK_EQ(party.members[1].wallet.goldInBank, kMaxV - 10);
    CHECK_EQ(party.members[0].wallet.goldOnHand, g(100));
    CHECK_EQ(party.members[0].wallet.goldInBank, Currency(0));
}

// ---- B_0055#135 ----------------------------------------------------------
// Same pool, A banked at 0, A first: succeeds; A banks 105, B empties.
TEST(engine_B_0055_135_pool_to_bank_ok_a_first) {
    tsup::Env env(1);
    Party party;
    party.size = 2;
    seat(party, 0, g(5), Currency(0));   // A (leader)
    seat(party, 1, g(100), Currency(0)); // B
    party.selectedIndex = 0;

    bool ok = poolGoldToBank(env.ctx, party);
    CHECK(ok);
    CHECK_EQ(party.members[0].wallet.goldOnHand, Currency(0));
    CHECK_EQ(party.members[0].wallet.goldInBank, g(105));
    CHECK_EQ(party.members[1].wallet.goldOnHand, Currency(0));
    CHECK_EQ(party.members[1].wallet.goldInBank, Currency(0));
}

// ---- B_0055#136 ----------------------------------------------------------
// Alt+P (pool to selected) in party ch, A, B, C. ch is near MaxValue, so only
// A's 10 fits before the pool stops on overflow.
TEST(engine_B_0055_136_alt_p_pool_stops_on_overflow) {
    tsup::Env env(1);
    Party party;
    party.size = 4;
    seat(party, 0, kMaxV - g(10), Currency(0)); // ch: headroom exactly g(10)
    seat(party, 1, g(10), Currency(0));         // A
    seat(party, 2, g(100), Currency(0));        // B
    seat(party, 3, g(1), Currency(0));          // C
    party.selectedIndex = 0;

    poolPartyGold(env.ctx, party, PartyPoolKind::ToSelected, false);

    // Pooled amount = ch's gain.
    CHECK_EQ(party.members[0].wallet.goldOnHand - (kMaxV - g(10)), g(10));
    CHECK(party.members[0].wallet.goldOnHand == kMaxV);
    CHECK_EQ(party.members[1].wallet.goldOnHand, Currency(0));
    CHECK_EQ(party.members[2].wallet.goldOnHand, g(100));
    CHECK_EQ(party.members[3].wallet.goldOnHand, g(1));
}

// ---- B_0055#137 ----------------------------------------------------------
// Exchange 5 from A (on hand MaxValue, banked 1, town) to B (100 on hand).
// Expected B on hand stays 100. The return text is not representable as the
// i32 returned by transferGold, so only the balance is asserted.
TEST(engine_B_0055_137_exchange_balance_b) {
    tsup::Env env(1);
    Party party;
    party.size = 2;
    seat(party, 0, kMaxV, g(1));         // A
    seat(party, 1, g(100), Currency(0)); // B
    party.dungeonLevel = 0;

    transferGold(env.ctx, party.members[0], party.members[1], g(5), false);
    CHECK_EQ(party.members[1].wallet.goldOnHand, g(100));
}

// ---- B_0055#138 ----------------------------------------------------------
// Exchange 5 from A (MaxValue, banked 0) to B (MaxValue): A keeps MaxValue.
// Return text (empty) is not representable as the i32 from transferGold, so
// only the A on-hand check is asserted.
TEST(engine_B_0055_138_exchange_overflow_keeps_a) {
    tsup::Env env(1);
    Party party;
    party.size = 2;
    seat(party, 0, kMaxV, Currency(0)); // A
    // Receiver B must sit at the R24 balance cap so that +5 gold would overflow it.
    seat(party, 1, kBalanceCap, Currency(0)); // B
    party.dungeonLevel = 0;

    transferGold(env.ctx, party.members[0], party.members[1], g(5), false);
    CHECK(party.members[0].wallet.goldOnHand == kMaxV);
}

// ---- B_0055#139 ----------------------------------------------------------
// Give 10 from A (50 on hand) to B (on hand MaxValue). A keeps 50.
// The number of lines said is not checked: the EventLog API is not in the
// headers shown, so only A's on-hand figure is asserted.
TEST(engine_B_0055_139_give_refused_a_keeps_50) {
    tsup::Env env(1);
    Party party;
    party.size = 2;
    seat(party, 0, g(50), Currency(0)); // A
    seat(party, 1, kMaxV, Currency(0)); // B

    giveGoldTransfer(env.ctx, party.members[0], party.members[1], g(10));
    CHECK_EQ(party.members[0].wallet.goldOnHand, g(50));
}

// SKIP B_0056#30: LookAround.ListedParty.Sync has no equivalent in the headers (UI look page).
// SKIP B_0056#31: LookAround.ListedParty.Sync has no equivalent in the headers (UI look page).
// SKIP B_0056#32: LookAround.ListedParty.Sync has no equivalent in the headers (UI look page).
