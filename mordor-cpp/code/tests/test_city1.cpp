#include "test_support.h"
#include "mordor/city1.h"

using namespace mordor;

static const Item* findStoreItem(const Vec<Item>& store, i32 id) {
    for (i32 i = 0; i < static_cast<i32>(store.size()); ++i) {
        if (store[i].id == id) return &store[i];
    }
    return nullptr;
}

// R3 formula: days = round_half_even((100 - cur/max*100) / 3.25).
// Only cur == 0 or cur == max is used, so integer and float division agree.
TEST(cityRestDays_zeroHitsAgesThirtyOneAndFullHitsAgesNothing) {
    // (100 - 0) / 3.25 = 30.77 -> 31
    CHECK_EQ(cityRestDays(0, 15), 31);
    CHECK_EQ(cityRestDays(0, 30), 31);
    // cur == max: no rest days
    CHECK_EQ(cityRestDays(15, 15), 0);
}

// R2: the city rest runs only if DungeonLevel = 0 and the character is not dead.
TEST(cityRestGate_livingAboveGroundPassesOtherwiseBlocks) {
    Character ch;
    CHECK(cityRestGatePasses(ch));

    Character down;
    down.location.dungeonLevel = 1;
    CHECK(!cityRestGatePasses(down));

    Character dead;
    dead.deathState = DeathState::Dead;
    CHECK(!cityRestGatePasses(dead));
}

// R4: only the first party member's rest replenishes store and pens.
TEST(replenishesOnRest_onlyFirstMemberTriggers) {
    CHECK(replenishesOnRest(0));
    CHECK(!replenishesOnRest(1));
    CHECK(!replenishesOnRest(3));
}

// R18: shelf floor cap is min(deepestReached, 14); floor 15 is never stocked.
TEST(storeShelfFloorCap_clampsAtFourteen) {
    CHECK_EQ(storeShelfFloorCap(1), 1);
    CHECK_EQ(storeShelfFloorCap(14), 14);
    CHECK_EQ(storeShelfFloorCap(15), 14);
    CHECK_EQ(storeShelfFloorCap(30), 14);
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

// R11/R12: none enabled keeps previous choice; one enabled is taken at once;
// more than one enabled asks (chosen = -1).
TEST(alignmentChoice_noneSingleAndMultipleCases) {
    Arr<i32, 4> none{};
    AlignChoiceRow r0 = alignmentChoice(none, 2);
    CHECK_EQ(r0.stocked, 0);
    CHECK_EQ(r0.last, -1);
    CHECK(!r0.asks);
    CHECK_EQ(r0.chosen, 2);

    Arr<i32, 4> one{};
    one[1] = 3;
    AlignChoiceRow r1 = alignmentChoice(one, 0);
    CHECK_EQ(r1.stocked, 1);
    CHECK_EQ(r1.last, 1);
    CHECK(!r1.asks);
    CHECK(r1.enabled[1]);
    CHECK(!r1.enabled[0]);
    CHECK_EQ(r1.chosen, 1);

    Arr<i32, 4> two{};
    two[0] = 2;
    two[2] = 5;
    AlignChoiceRow r2 = alignmentChoice(two, 1);
    CHECK_EQ(r2.stocked, 2);
    CHECK_EQ(r2.last, 2);
    CHECK(r2.asks);
    CHECK_EQ(r2.chosen, -1);
    CHECK(r2.enabled[0]);
    CHECK(!r2.enabled[1]);
    CHECK(r2.enabled[2]);
    CHECK(!r2.enabled[3]);

    // Store Buy row click follows the same rule (R12).
    AlignChoiceRow rb = storeBuyRowClick(two, 1);
    CHECK(rb.asks);
    CHECK_EQ(rb.chosen, -1);
}

// R14: confinement uses the same logic; a single Evil pen is taken at once.
TEST(confinementRowClick_singleEvilIsTakenAtOnce) {
    Arr<i32, 4> pens{};
    pens[2] = 3;
    AlignChoiceRow r = confinementRowClick(pens, 0);
    CHECK(!r.asks);
    CHECK_EQ(r.last, 2);
    CHECK_EQ(r.chosen, 2);
}

// R13 step 2: a sold-out or hidden highlighted row empties the buy state and withdraws choosing.
TEST(settleStoreSelection_soldOutWithdrawsChoiceAndGreysButtons) {
    StoreSelection sel;
    sel.row = 3;
    sel.choosing = true;
    sel.buyEnabled = true;
    sel.infoEnabled = true;
    settleStoreSelection(sel, true);
    CHECK(!sel.choosing);
    CHECK(!sel.buyEnabled);
    CHECK(!sel.infoEnabled);
}

// R13 step 1: while the frame is up, another row click is refused unless a mod allows it (R40 default refused).
TEST(storeRowClickAllowed_refusedWhileChoosingUnlessModAllows) {
    StoreSelection sel;
    sel.row = 1;
    sel.choosing = true;
    TownModAnswers mods;
    mods.liveMods = 1;
    CHECK(!storeRowClickAllowed(sel, 2, mods));
    mods.rowChangeAllowed = true;
    CHECK(storeRowClickAllowed(sel, 2, mods));
    sel.choosing = false;
    mods.rowChangeAllowed = false;
    CHECK(storeRowClickAllowed(sel, 2, mods));
}

// R38: a row a StoreRowVisible mod answered false for is not visible.
TEST(storeRowVisible_modHiddenRowIsNotVisible) {
    TownModAnswers mods;
    mods.liveMods = 1;
    mods.hiddenRows = Vec<i32>{2};
    CHECK(!storeRowVisible(mods, 2));
    CHECK(storeRowVisible(mods, 1));
}

// R39 and R44: a row-order mismatch refuses the order; disabling sets the session flag.
TEST(storeRowOrderAccepted_mismatchRefusedAndDisableFlagSet) {
    Vec<i32> defaults{0, 1, 2};
    Vec<i32> same{0, 1, 2};
    Vec<i32> shorter{0, 1};
    CHECK(storeRowOrderAccepted(defaults, same));
    CHECK(!storeRowOrderAccepted(defaults, shorter));

    TownModAnswers mods;
    CHECK(!mods.rowOrderDisabled);
    disableRowOrderForSession(mods);
    CHECK(mods.rowOrderDisabled);
}

// R40 and R43: row change and alignment choice unlock both default to refused.
TEST(storeRowChangeAndAlignChoice_defaultRefused) {
    TownModAnswers mods;
    CHECK(!storeRowChangeAllowed(mods));
    CHECK(!storeAlignChoiceUnlocked(mods));
    mods.rowChangeAllowed = true;
    mods.alignChoiceUnlocked = true;
    CHECK(storeRowChangeAllowed(mods));
    CHECK(storeAlignChoiceUnlocked(mods));
}

// R42: no answer gives the default message window; a kept answer replaces it.
TEST(messageWindowLines_defaultUnlessAnswered) {
    TownModAnswers mods;
    CHECK_EQ(messageWindowLines(mods), kDefaultMessageLines);
    mods.liveMods = 1;
    mods.messagesKept = 7;
    CHECK_EQ(messageWindowLines(mods), 7);
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

// R17 (wiki): General Store buys at 50% of base value.
TEST(generalStorePayout_paysHalfBaseValue) {
    CHECK_EQ(generalStorePayout(20000), 10000);
    CHECK_EQ(generalStorePayout(0), 0);
}

// R6: leader above ground and alive: UPDATE visits Store, Confinement, Morgue, Guild, Bank in order.
TEST(townSwitchOrder_aboveGroundLeaderVisitsSectionsInOrder) {
    Party party;
    party.size = 1;
    party.dungeonLevel = 0;
    Vec<TownSection> order = townSwitchOrder(party);
    CHECK_EQ(static_cast<i32>(order.size()), 5);
    CHECK(order[0] == TownSection::Store);
    CHECK(order[1] == TownSection::Confinement);
    CHECK(order[2] == TownSection::Morgue);
    CHECK(order[3] == TownSection::Guild);
    CHECK(order[4] == TownSection::Bank);
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

// R5: scanning is pack slots 1..39 only, and only guild-issued items (Ailments 500..511) are refilled.
TEST(refillGuildCrests_slotZeroAndNonGuildItemsUntouched) {
    tsup::Env env({0.5, 0.9});
    Inventory inv;
    Str owner{};
    // Guild crest in slot 0: slot 0 is not examined.
    inv.pack[0].id = 300;
    inv.pack[0].ailments = 502;
    inv.pack[0].charges = 0;
    // Same id but not a guild item (Ailments below 500): not refilled.
    inv.pack[1].id = 300;
    inv.pack[1].ailments = 0;
    inv.pack[1].charges = 0;
    refillGuildCrests(env.ctx, inv, owner, false);
    CHECK_EQ(inv.pack[0].charges, 0);
    CHECK_EQ(inv.pack[1].charges, 0);
}

// R1 steps 1, 3, 4: lost position and level are cleared, facing is kept, and the member rests
// (R3: CurHits = MaxHits, AilmentSteps = 0).
TEST(arriveInTown_clearsLostPositionAndLevelKeepsFacingAndRests) {
    tsup::Env env({0.5, 0.9});
    Party party;
    party.size = 1;
    Character& ch = party.members[0];
    ch.lost.position = true;
    ch.lost.level = true;
    ch.lost.facing = true;
    ch.vitals.curHits = 0;
    ch.vitals.maxHits = 15;
    ch.ailments.ailmentSteps = 3;

    TownStock town;
    i64 mostExperience = 0;
    TownModAnswers mods;
    ArrivalResult res = arriveInTown(env.ctx, party, town, mostExperience, mods);

    CHECK(!ch.lost.position);
    CHECK(!ch.lost.level);
    CHECK(ch.lost.facing);
    CHECK(res.rested[0]);
    CHECK_EQ(ch.vitals.curHits, 15);
    CHECK_EQ(ch.ailments.ailmentSteps, 0);
}
