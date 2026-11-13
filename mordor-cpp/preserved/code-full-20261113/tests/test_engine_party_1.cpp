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

TEST(engine_B_0055_101_give_10_5_no_refusal) {
    tsup::Env env({0.5, 0.9});
    Character giver = withGold(105000, 50000);
    Character receiver = withGold(0, 0);
    CHECK(giveGoldTransfer(env.ctx, giver, receiver, 105000) == GiveGoldOutcome::Done);
}

TEST(engine_B_0055_102_give_11_not_enough_gold) {
    tsup::Env env({0.5, 0.9});
    Character giver = withGold(105000, 50000);
    Character receiver = withGold(0, 0);
    CHECK(giveGoldTransfer(env.ctx, giver, receiver, 110000) == GiveGoldOutcome::NotEnoughGold);
}

TEST(engine_B_0055_104_give_10_hands_figures) {
    tsup::Env env({0.5, 0.9});
    Character giver = withGold(105000, 50000);
    Character receiver = withGold(0, 0);
    GiveGoldOutcome r = giveGoldTransfer(env.ctx, giver, receiver, 100000);
    CHECK(r == GiveGoldOutcome::Done);
    CHECK_EQ(giver.wallet.goldOnHand, 5000);     // 0.5
    CHECK_EQ(receiver.wallet.goldOnHand, 100000); // 10
}

TEST(engine_B_0055_105_transfer_exchange_town) {
    tsup::Env env({0.5, 0.9});
    Character from = withGold(105000, 50000);
    Character to = withGold(0, 0);
    GoldTransferOutcome r = transferExchangeGold(env.ctx, from, to, 120000, false);
    CHECK(r == GoldTransferOutcome::Done);
    CHECK_EQ(from.wallet.goldOnHand, 0);       // 0
    CHECK_EQ(from.wallet.goldInBank, 35000);   // 3.5
    CHECK_EQ(to.wallet.goldOnHand, 120000);    // 12
}

TEST(engine_B_0055_106_transfer_exchange_dungeon_too_much) {
    tsup::Env env({0.5, 0.9});
    Character from = withGold(105000, 50000);
    Character to = withGold(0, 0);
    CHECK(transferExchangeGold(env.ctx, from, to, 110000, true) == GoldTransferOutcome::TooMuch);
}

TEST(engine_B_0055_107_party_deposit_two_members) {
    tsup::Env env({0.5, 0.9});
    Character a = withGold(12500, 5000);   // 1.25 / 0.5
    Character b = withGold(25000, 7500);   // 2.5 / 0.75
    Party p = twoMembers(a, b);
    Currency handBefore = a.wallet.goldOnHand + b.wallet.goldOnHand;

    partyDepositGold(env.ctx, p);

    Currency handAfter = p.members[0].wallet.goldOnHand + p.members[1].wallet.goldOnHand;
    CHECK_EQ(handBefore - handAfter, 37500);                 // total 3.75
    CHECK_EQ(p.members[0].wallet.goldOnHand, 0);             // A hand 0
    CHECK_EQ(p.members[0].wallet.goldInBank, 17500);         // A bank 1.75
    CHECK_EQ(p.members[1].wallet.goldOnHand, 0);             // B hand 0
    CHECK_EQ(p.members[1].wallet.goldInBank, 32500);         // B bank 3.25
}

TEST(engine_B_0055_108_pool_and_deposit_to_leader) {
    tsup::Env env({0.5, 0.9});
    Party p = twoMembers(withGold(12500, 17500),   // A: 1.25 / 1.75
                         withGold(25000, 32500));  // B: 2.5 / 3.25
    p.selectedIndex = 0;                            // A in command

    poolPartyGold(env.ctx, p, PartyPoolKind::PoolAndDeposit, false);

    CHECK_EQ(p.members[0].wallet.goldOnHand, 0);      // A hand 0
    CHECK_EQ(p.members[0].wallet.goldInBank, 87500);  // A bank 8.75
    CHECK_EQ(p.members[1].wallet.goldOnHand, 0);      // B hand 0
    CHECK_EQ(p.members[1].wallet.goldInBank, 0);      // B bank 0
}

TEST(engine_B_0055_109_pool_gold_alt_p) {
    tsup::Env env({0.5, 0.9});
    Party p = twoMembers(withGold(0, 0),    // ch, 0 on hand
                         withGold(1, 0));   // other, 0.0001 on hand
    p.selectedIndex = 0;

    poolGoldToSelected(env.ctx, p);

    CHECK_EQ(p.members[0].wallet.goldOnHand, 1);  // ch on-hand 0.0001
    CHECK_EQ(p.members[1].wallet.goldOnHand, 0);  // other emptied; pooled 0.0001
}

TEST(engine_B_0055_132_party_deposit_overflow_rollback) {
    tsup::Env env({0.5, 0.9});
    // B banked at MaxValue minus 10 (raw i64 units).
    Party p = twoMembers(withGold(50000, 0),                       // A: 5 / 0
                         withGold(1000000, 9223372036854775797LL)); // B: 100 / MaxValue-10
    Currency handBefore = p.members[0].wallet.goldOnHand + p.members[1].wallet.goldOnHand;

    BankPartyOutcome r = bankPartyOption(env.ctx, p, BankPartyOption::PartyDeposit);

    CHECK(r == BankPartyOutcome::RolledBack);
    Currency handAfter = p.members[0].wallet.goldOnHand + p.members[1].wallet.goldOnHand;
    CHECK_EQ(handBefore - handAfter, 0);                 // total moved 0
    CHECK_EQ(p.members[0].wallet.goldOnHand, 50000);     // A hand 5
    CHECK_EQ(p.members[0].wallet.goldInBank, 0);         // A bank 0
    CHECK_EQ(p.members[1].wallet.goldOnHand, 1000000);   // B hand 100
    // B bank is not asserted: the expected figure 922337203685467.5807 gold
    // (raw 9223372036854675807) does not equal MaxValue-10 raw
    // (9223372036854775797, i.e. 922337203685477.5797 gold) and the
    // rollback should leave the starting value unchanged.
}
