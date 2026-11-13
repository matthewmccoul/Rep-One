#include "test_support.h"
#include "mordor/city1.h"
#include "mordor/city3a.h"

using namespace mordor;

// Counts vault (bank) entries whose Item::id is non-zero.
static i32 bankEntryCount(const Inventory& inv) {
    i32 n = 0;
    for (i32 i = 0; i < kBankSlots; ++i) {
        if (inv.bank[static_cast<u32>(i)].id != 0) {
            ++n;
        }
    }
    return n;
}

// Fills every vault entry with a non-empty item (ItemIds 1..kBankSlots).
static void fillVaultFull(Inventory& inv) {
    for (i32 i = 0; i < kBankSlots; ++i) {
        inv.bank[static_cast<u32>(i)].id = i + 1;
    }
}

// B_0065#98: realign fee for an item priced 4 (gold), Good to Good.
// Currency = gold * 10000: price 4 -> 40000, expected 3 -> 30000.
TEST(engine_B_0065_98_realign_fee_good_to_good) {
    tsup::Env env(1);
    CHECK_EQ(storeRealignFee(40000, ItemAlign::Good, ItemAlign::Good), static_cast<Currency>(30000));
}

// B_0067#1: deposit pack slot 5 (ItemId 7) into an empty vault returns true (Stored).
TEST(engine_B_0067_1_deposit_returns_true) {
    tsup::Env env(1);
    Inventory inv;
    inv.pack[5].id = 7;
    VaultDeposit r = depositToVault(env.ctx, inv, 5);
    CHECK(r == VaultDeposit::Stored);
}

// B_0067#2: after depositing slot 5 into an empty vault: 1 entry, first entry ItemId 7, slot 5 empty.
TEST(engine_B_0067_2_deposit_state_after) {
    tsup::Env env(1);
    Inventory inv;
    inv.pack[5].id = 7;
    depositToVault(env.ctx, inv, 5);
    CHECK_EQ(bankEntryCount(inv), 1);
    CHECK_EQ(inv.bank[0].id, 7);
    CHECK(inv.pack[5].id == 0);
}

// B_0067#3: vault [1, empty, 2], pack slot 6 holds ItemId 9; deposit returns true (not Refused).
TEST(engine_B_0067_3_deposit_into_gap_true) {
    tsup::Env env(1);
    Inventory inv;
    inv.bank[0].id = 1;
    inv.bank[1].id = 0;
    inv.bank[2].id = 2;
    inv.pack[6].id = 9;
    VaultDeposit r = depositToVault(env.ctx, inv, 6);
    CHECK(r != VaultDeposit::Refused);
}

// B_0067#4: depositing the Hands (pack slot 0) returns false (Refused).
TEST(engine_B_0067_4_hands_slot_refused) {
    tsup::Env env(1);
    Inventory inv;
    inv.pack[0].id = 5;
    VaultDeposit r = depositToVault(env.ctx, inv, 0);
    CHECK(r == VaultDeposit::Refused);
}

// B_0067#5: full vault, pack slot 7 holds ItemId 11; deposit returns false (Refused).
TEST(engine_B_0067_5_full_vault_refused) {
    tsup::Env env(1);
    Inventory inv;
    fillVaultFull(inv);
    inv.pack[7].id = 11;
    VaultDeposit r = depositToVault(env.ctx, inv, 7);
    CHECK(r == VaultDeposit::Refused);
}

// B_0067#6: after a refused deposit into a full vault, pack slot 7 still holds ItemId 11.
TEST(engine_B_0067_6_refused_keeps_pack_item) {
    tsup::Env env(1);
    Inventory inv;
    fillVaultFull(inv);
    inv.pack[7].id = 11;
    depositToVault(env.ctx, inv, 7);
    CHECK_EQ(inv.pack[7].id, 11);
}

// B_0067#7: full vault, pack slot 8 empty; deposit returns false (Refused).
TEST(engine_B_0067_7_full_vault_empty_slot_refused) {
    tsup::Env env(1);
    Inventory inv;
    fillVaultFull(inv);
    VaultDeposit r = depositToVault(env.ctx, inv, 8);
    CHECK(r == VaultDeposit::Refused);
}

// B_0067#8: vault full except entry 3 emptied; pack slot 8 empty.
// Deposit returns true (not Refused); vault entry 3 ItemId is at most 0 afterwards.
TEST(engine_B_0067_8_emptied_entry_accepts_empty_slot) {
    tsup::Env env(1);
    Inventory inv;
    fillVaultFull(inv);
    inv.bank[2].id = 0;
    VaultDeposit r = depositToVault(env.ctx, inv, 8);
    CHECK(r != VaultDeposit::Refused);
    CHECK(inv.bank[2].id <= 0);
}

// B_0067#13: no cursed items (Cursed != 0) in the initial store stock.
TEST(engine_B_0067_13_initial_store_no_cursed) {
    tsup::Env env(1);
    Vec<Item> store;
    createInitialStore(env.ctx, store);
    i32 cursed = 0;
    for (const Item& it : store) {
        if (it.cursed != CurseNone) {
            ++cursed;
        }
    }
    CHECK_EQ(cursed, 0);
}
