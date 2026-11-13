#include "test_support.h"
#include "mordor/items2b.h"
#include "mordor/city1.h"  // FIXED: storeShelvesItem(const Ctx&, const Item&, const Vec<Item>&) is declared in city1.h

using namespace mordor;

namespace {

// Currency is stored in 1/10000 gold.
constexpr Currency kGoldUnit = 10000;

Character purseChar(Currency hand, Currency bank) {
    Character c;
    c.wallet.goldOnHand = hand;
    c.wallet.goldInBank = bank;
    return c;
}

}  // namespace

TEST(NeedsIdentifyFeeOnlyWhenNotFullyKnown) {
    Item unknownItem;  // default idLevel is Unknown
    CHECK(needsIdentifyFee(unknownItem));

    Item partial;
    partial.idLevel = IdLevel::Partial;
    CHECK(needsIdentifyFee(partial));

    Item mostly;
    mostly.idLevel = IdLevel::Mostly;
    CHECK(needsIdentifyFee(mostly));

    Item fullyKnown;
    fullyKnown.idLevel = IdLevel::All;
    CHECK(!needsIdentifyFee(fullyKnown));
}

TEST(ChargePurseRules) {
    tsup::Env env({0.5, 0.9});

    // Hand pays exactly when it covers the amount.
    Character a = purseChar(500, 300);
    CHECK(chargePurse(a, 200));
    CHECK_EQ(a.wallet.goldOnHand, 300);
    CHECK_EQ(a.wallet.goldInBank, 300);

    // Otherwise hand is emptied and the rest comes from the bank.
    Character b = purseChar(500, 300);
    CHECK(chargePurse(b, 600));
    CHECK_EQ(b.wallet.goldOnHand, 0);
    CHECK_EQ(b.wallet.goldInBank, 200);

    // Hand plus bank short of the amount: refused, nothing taken.
    Character c = purseChar(500, 300);
    CHECK(!chargePurse(c, 900));
    CHECK_EQ(c.wallet.goldOnHand, 500);
    CHECK_EQ(c.wallet.goldInBank, 300);

    // Exactly the whole purse is enough.
    Character d = purseChar(500, 300);
    CHECK(chargePurse(d, 800));
    CHECK_EQ(d.wallet.goldOnHand, 0);
    CHECK_EQ(d.wallet.goldInBank, 0);
}

TEST(StoreShelvesCursedNever) {
    // R61: a cursed item is never shelved. Call the real signature (ctx, item, store).
    tsup::Env env({0.5, 0.9});
    Vec<Item> store;

    Item cursed;
    cursed.cursed = CurseKind1;
    cursed.alignment = ItemAlign::Good;
    CHECK(!storeShelvesItem(env.ctx, cursed, store));

    cursed.alignment = ItemAlign::Evil;
    CHECK(!storeShelvesItem(env.ctx, cursed, store));
}

