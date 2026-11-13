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
