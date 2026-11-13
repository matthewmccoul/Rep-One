#include "test_support.h"
#include "mordor/city1.h"

using namespace mordor;

static const Item* findStoreItem(const Vec<Item>& store, i32 id) {
    for (i32 i = 0; i < static_cast<i32>(store.size()); ++i) {
        if (store[i].id == id) return &store[i];
    }
    return nullptr;
}

// Constants stated directly in the SRD sections covered here.
TEST(constants_matchSrdValues) {
    CHECK_EQ(kTownDeepestFloor, 14);       // R8, R18: mc0118 - 1
    CHECK_EQ(kRestockChanceMin, 1);        // R8: chance clamp low
    CHECK_EQ(kRestockChanceMax, 50);       // R8: chance clamp high
    CHECK_EQ(kAddProbabilityPct, 60);      // R8: add when rnd*100 < 60
    CHECK_EQ(kCrestScanFirst, 1);          // R5: slot 0 not examined
    CHECK_EQ(kCrestScanLast, 39);          // R5: last pack slot scanned
    CHECK_EQ(kPenAlignCount, 3);           // R30: pen indices 0..2
    CHECK(kRestDaysDivisor == 3.25);       // R3 divisor
    // T-Q crest charges: Thief = 12, Villain = 6, Nomad = 0
    CHECK_EQ(kCrestCharges[static_cast<i32>(Guild::Thief)], 12);
    CHECK_EQ(kCrestCharges[static_cast<i32>(Guild::Villain)], 6);
    CHECK_EQ(kCrestCharges[static_cast<i32>(Guild::Nomad)], 0);
}

// R10: index = round_half_even(rnd * n), n = 4. Index n is avoided by these inputs.
TEST(roundedIndex_roundsHalfToEvenNotTruncates) {
    {
        tsup::Env env({0.1, 0.1});
        CHECK_EQ(roundedIndex(env.ctx, 4), 0);      // 0.4 -> 0
    }
    {
        tsup::Env env({0.2, 0.2});
        CHECK_EQ(roundedIndex(env.ctx, 4), 1);      // 0.8 -> 1
    }
    {
        tsup::Env env({0.375, 0.375});
        CHECK_EQ(roundedIndex(env.ctx, 4), 2);      // 1.5 -> 2 (half to even)
    }
    {
        tsup::Env env({0.625, 0.625});
        CHECK_EQ(roundedIndex(env.ctx, 4), 2);      // 2.5 -> 2 (half to even)
    }
}

// R15 step 1 and R41: sale goes to the seller's hand unless a mod names a payee.
TEST(townIncomePayee_defaultsToSellerUnlessModNamesPayee) {
    Party party;
    party.size = 3;
    TownModAnswers mods;
    CHECK_EQ(townIncomePayee(party, 2, mods), 2);
    mods.liveMods = 1;
    mods.incomeAnswered = true;
    mods.incomePayee = 0;
    CHECK_EQ(townIncomePayee(party, 2, mods), 0);
}

// R30: pen indices 0..2 = Good, Neutral, Evil.
TEST(penIndexOfItemAlign_goodNeutralEvilOrder) {
    CHECK_EQ(penIndexOfItemAlign(ItemAlign::Good), 0);
    CHECK_EQ(penIndexOfItemAlign(ItemAlign::Neutral), 1);
    CHECK_EQ(penIndexOfItemAlign(ItemAlign::Evil), 2);
}

// R16: a cursed item never shelves; an uncursed Good item shelves on an empty row.
TEST(storeShelvesItem_cursedNeverShelvesUncursedGoodDoes) {
    tsup::Env env({0.5, 0.9});
    Vec<Item> store{};
    Item good;
    good.alignment = ItemAlign::Good;
    CHECK(storeShelvesItem(env.ctx, good, store));
    Item cursed = good;
    cursed.cursed = CurseKind1;
    CHECK(!storeShelvesItem(env.ctx, cursed, store));
}

// R7 and T-C: artifacts 356..361 are stocked with 358 Good, 359 Evil, others Neutral.
TEST(createInitialStore_artifactsStockedWithSrdAlignments) {
    tsup::Env env(1234);
    Vec<Item> store{};
    createInitialStore(env.ctx, store);

    const i32 ids[6] = {356, 357, 358, 359, 360, 361};
    const ItemAlign want[6] = {ItemAlign::Neutral, ItemAlign::Neutral, ItemAlign::Good,
                               ItemAlign::Evil, ItemAlign::Neutral, ItemAlign::Neutral};
    for (i32 i = 0; i < 6; ++i) {
        const Item* it = findStoreItem(store, ids[i]);
        CHECK(it != nullptr);
        if (it != nullptr) CHECK(it->alignment == want[i]);
    }

    CHECK_EQ(kStartingArtifacts[2].itemId, 358);
    CHECK(kStartingArtifacts[2].alignment == ItemAlign::Good);
    CHECK_EQ(kStartingArtifacts[3].itemId, 359);
    CHECK(kStartingArtifacts[3].alignment == ItemAlign::Evil);
}

