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

TEST(ParseGiveGoldAnswerZeroCases) {
    // Empty, sub-1 and non-numeric answers give 0.
    CHECK_EQ(parseGiveGoldAnswer(Str("")), 0);
    CHECK_EQ(parseGiveGoldAnswer(Str("abc")), 0);
    CHECK_EQ(parseGiveGoldAnswer(Str("0.5")), 0);
    CHECK_EQ(parseGiveGoldAnswer(Str("-4")), 0);
    CHECK_EQ(parseGiveGoldAnswer(Str("250")), 250);
}

TEST(ParseSwapRequestAccepts) {
    // T-1: typed input is the player slot (2-40). Internal index = player slot - 1.
    SwapRequest r = parseSwapRequest(Str("3,7"));
    CHECK(r.status == SwapParse::Swapped);
    CHECK_EQ(r.first, 2);
    CHECK_EQ(r.second, 6);

    // Player slot 40 is the highest valid slot, internal index 39.
    SwapRequest edge = parseSwapRequest(Str("2,40"));
    CHECK(edge.status == SwapParse::Swapped);
    CHECK_EQ(edge.first, 1);
    CHECK_EQ(edge.second, 39);
}

TEST(ParseSwapRequestRefusals) {
    // No comma.
    CHECK(parseSwapRequest(Str("37")).status == SwapParse::NoComma);
    // Slot 1 is Hands.
    CHECK(parseSwapRequest(Str("1,5")).status == SwapParse::Refused);
    // Slot past 40.
    CHECK(parseSwapRequest(Str("5,41")).status == SwapParse::Refused);
    // Input longer than 5 characters (7 here), although both slots are valid.
    CHECK(parseSwapRequest(Str("00003,4")).status == SwapParse::Refused);
}

TEST(DrawTheftSlotBounds) {
    // slot = Int(Rnd * 39) + 2
    tsup::Env lo({0.0, 0.0});
    CHECK_EQ(drawTheftSlot(lo.ctx), 2);

    tsup::Env mid({0.5, 0.5});
    CHECK_EQ(drawTheftSlot(mid.ctx), 21);  // Int(19.5) = 19, + 2

    tsup::Env hi({0.999999, 0.999999});
    CHECK_EQ(drawTheftSlot(hi.ctx), 40);  // Int(38.99996) = 38, + 2

    CHECK_EQ(kTheftSlotFirst, 2);
    CHECK_EQ(kLastPackIndex, 39);
    CHECK_EQ(kSwapInputMaxChars, 5);
}

TEST(LibraryGradeForIdLevel) {
    CHECK_EQ(libraryGradeFor(IdLevel::All), kLibraryGradeAll);
    CHECK_EQ(libraryGradeFor(IdLevel::All), 4);
    CHECK_EQ(libraryGradeFor(IdLevel::Mostly), kLibraryGradeMostly);
    CHECK_EQ(libraryGradeFor(IdLevel::Partial), kLibraryGradeNone);
    CHECK_EQ(libraryGradeFor(IdLevel::Barely), 0);
    CHECK_EQ(libraryGradeFor(IdLevel::Unknown), 0);
}

TEST(LibraryGradeAfterReadOnlyRises) {
    // The grade never falls on a re-read.
    CHECK_EQ(libraryGradeAfterRead(4, IdLevel::Unknown), 4);
    CHECK_EQ(libraryGradeAfterRead(3, IdLevel::Partial), 3);
    // It rises to the grade of the new level.
    CHECK_EQ(libraryGradeAfterRead(0, IdLevel::Mostly), 3);
    CHECK_EQ(libraryGradeAfterRead(3, IdLevel::All), 4);
    CHECK_EQ(libraryGradeAfterRead(0, IdLevel::Barely), 0);
}

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

TEST(BuyDepletesShelfRules) {
    CHECK(buyDepletesShelf(true, false));   // dungeon item
    CHECK(buyDepletesShelf(false, true));   // aligned copy of a floor-0 item
    CHECK(buyDepletesShelf(true, true));
    CHECK(!buyDepletesShelf(false, false));
}

TEST(CombineEnabledOnceNonEmpty) {
    Vec<i32> none;
    CHECK(!combineEnabled(none));

    Vec<i32> one{3};
    CHECK(combineEnabled(one));
}

TEST(ExchangeSlotBaseSides) {
    // Chars(0) occupies slots 0-39, Chars(1) occupies slots 40-79.
    CHECK_EQ(exchangeSlotBase(0), 0);
    CHECK_EQ(exchangeSlotBase(1), 40);
}

TEST(ExchangeRefusals) {
    tsup::Env env({0.5, 0.9});
    Inventory a;
    Inventory b;

    // Dropping a slot on itself does nothing.
    CHECK(exchangeSwap(env.ctx, a, 5, a, 5) == ExchangeOutcome::SameSlot);

    // Hands is internal index 0 (player slot 1, T-1; exchange window indexes from 0, R74).
    // R76: refused silently at either end.
    CHECK(exchangeSwap(env.ctx, a, 0, b, 5) == ExchangeOutcome::HandsSilent);
    CHECK(exchangeSwap(env.ctx, a, 5, b, 0) == ExchangeOutcome::HandsSilent);
}

TEST(GiveGoldRefusesCombatAndSelf) {
    tsup::Env env({0.5, 0.9});
    Character giver = purseChar(500 * kGoldUnit, 0);
    Character receiver = purseChar(0, 0);

    // Gold cannot be given during combat; nothing moves.
    CHECK(giveGold(env.ctx, giver, receiver, 0, 1, 10, true) == GiveResult::InCombat);
    CHECK_EQ(giver.wallet.goldOnHand, 500 * kGoldUnit);
    CHECK_EQ(receiver.wallet.goldOnHand, 0);

    // A character giving to themselves has no effect.
    CHECK(giveGold(env.ctx, giver, giver, 0, 0, 10, false) == GiveResult::SameCharacter);
    CHECK_EQ(giver.wallet.goldOnHand, 500 * kGoldUnit);
}

TEST(GiveGoldUsesHandOnly) {
    tsup::Env env({0.5, 0.9});
    Character giver = purseChar(100 * kGoldUnit, 1000 * kGoldUnit);
    Character receiver = purseChar(0, 0);

    // Hand holds 100 gold; the 1000 in the bank does not count.
    CHECK(giveGold(env.ctx, giver, receiver, 0, 1, 150, false) == GiveResult::NotEnoughGold);
    CHECK_EQ(giver.wallet.goldOnHand, 100 * kGoldUnit);
    CHECK_EQ(giver.wallet.goldInBank, 1000 * kGoldUnit);

    CHECK(giveGold(env.ctx, giver, receiver, 0, 1, 60, false) == GiveResult::Done);
    CHECK_EQ(giver.wallet.goldOnHand, 40 * kGoldUnit);
    CHECK_EQ(giver.wallet.goldInBank, 1000 * kGoldUnit);
    CHECK_EQ(receiver.wallet.goldOnHand, 60 * kGoldUnit);
}

TEST(GiveItemRangeAndCombatRefusals) {
    tsup::Env env({0.5, 0.9});
    Character giver;
    Character receiver;
    Inventory giverInv;
    Inventory receiverInv;

    // Slot 1 is Hands: no item to give.
    CHECK(giveItem(env.ctx, giver, giverInv, 0, receiver, receiverInv, 1, 1, false)
          == GiveResult::NoItem);
    // Slot above 40 is refused.
    CHECK(giveItem(env.ctx, giver, giverInv, 0, receiver, receiverInv, 1, 41, false)
          == GiveResult::SlotOutOfRange);
    // Nothing can be given during combat.
    CHECK(giveItem(env.ctx, giver, giverInv, 0, receiver, receiverInv, 1, 5, true)
          == GiveResult::InCombat);
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

TEST(ParalysisBlocksPackActionAndBufferedUse) {
    Character paralysed;
    paralysed.ailments.paralysed = true;
    CHECK(paralysedBlocksPackAction(paralysed));
    CHECK(paralysedBlocksUse(paralysed, false));
    // An item already buffered is used without the paralysis test.
    CHECK(!paralysedBlocksUse(paralysed, true));

    Character free;
    CHECK(!paralysedBlocksPackAction(free));
    CHECK(!paralysedBlocksUse(free, false));
}

TEST(CanBankAndWearableRules) {
    Item equipped;
    equipped.equipped = true;
    CHECK(!canBankItem(equipped));

    Item plain;
    CHECK(canBankItem(plain));

    // Potions and scrolls are not wearable.
    CHECK(!isWearableType(ItemType::Potion));
    CHECK(!isWearableType(ItemType::Scroll));
}

TEST(EquipAndChestCurseFlags) {
    // T-5, R11, R22: only an opposed-fit (two-step) or cursed item takes the slot curse on equip.
    // Adjacent-fit and unaligned clean items do not.
    // FIXED: fixtureCtx() is not declared anywhere; use the same tsup::Env fixture as the other tests in this file.
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;

    Item fine;
    applyEquipCurse(ctx, fine, Align::Neutral);
    CHECK(!fine.slotCursed);

    Item equipped;
    equipped.cursed = CurseKind1;
    applyEquipCurse(ctx, equipped, Align::Neutral);
    CHECK(equipped.slotCursed);

    // Opposed fit: Good item worn by Evil wearer (two steps) -> curse.
    Item opposed;
    opposed.aligned = true;
    opposed.alignment = Good;
    applyEquipCurse(ctx, opposed, Align::Evil);
    CHECK(opposed.slotCursed);

    // Adjacent fit: Neutral item worn by Evil wearer (one step) -> no curse.
    Item adjacent;
    adjacent.aligned = true;
    adjacent.alignment = Neutral;
    applyEquipCurse(ctx, adjacent, Align::Evil);
    CHECK(!adjacent.slotCursed);

    Item cursed1;
    cursed1.cursed = CurseKind1;
    applyChestDropCurse(cursed1);
    CHECK(cursed1.slotCursed);

    Item cursed2;
    cursed2.cursed = CurseKind2;
    applyChestDropCurse(cursed2);
    CHECK(cursed2.slotCursed);

    Item clean;
    applyChestDropCurse(clean);
    CHECK(!clean.slotCursed);
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

TEST(DropOnVaultHandsRefused) {
    tsup::Env env({0.5, 0.9});
    Inventory inv;
    // Hands never go into the vault.
    CHECK(dropOnVault(env.ctx, inv, 1) == DragOutcome::HandsRefused);
}

TEST(WithdrawPackFullRefused) {
    tsup::Env env({0.5, 0.9});
    Inventory inv;
    for (i32 i = 0; i < kPackSlots; ++i) {
        inv.pack[i].id = 9;  // non-empty
    }
    inv.bank[0].id = 77;

    Character c;
    CHECK(withdrawFromBank(env.ctx, c, inv, 0) == WithdrawOutcome::PackFull);
    // The item stays in the Bank.
    CHECK_EQ(inv.bank[0].id, 77);
}

TEST(WithdrawToFreePackSlot) {
    tsup::Env env({0.5, 0.9});
    Inventory inv;
    inv.bank[0].id = 77;

    Character c;
    CHECK(withdrawFromBank(env.ctx, c, inv, 0) == WithdrawOutcome::Withdrawn);

    i32 found = 0;
    for (i32 i = 0; i < kPackSlots; ++i) {
        if (inv.pack[i].id == 77) {
            ++found;
        }
    }
    CHECK_EQ(found, 1);
}

TEST(BindCompanionRefusals) {
    tsup::Env env({0.5, 0.9});
    Currency rich = 1000000 * kGoldUnit;  // plenty, so cost is not the limit
    Character payer = purseChar(rich, 0);

    // Free will (BindingLevel < 0) is refused.
    Companion comp;
    comp.bindingLevel = -1;
    CHECK(bindCompanion(env.ctx, payer, comp) == BindOutcome::FreeWillRefused);
    CHECK_EQ(payer.purse(), rich);

    // At Binding.Max the companion is already bound.
    comp.bindingLevel = kBindingMax;
    CHECK(bindCompanion(env.ctx, payer, comp) == BindOutcome::AlreadyBound);
    CHECK_EQ(payer.purse(), rich);

    // Above the ceiling is also already bound.
    comp.bindingLevel = kBindingMax + 1;
    CHECK(bindCompanion(env.ctx, payer, comp) == BindOutcome::AlreadyBound);
    CHECK_EQ(payer.purse(), rich);
}

TEST(SellCompanionRefusedAtZeroHits) {
    tsup::Env env({0.5, 0.9});
    const MonsterDef& def = tsup::data().monsters[0];
    Character seller = purseChar(50 * kGoldUnit, 0);

    Companion comp;
    comp.hits = 0;
    CHECK(!sellCompanion(env.ctx, seller, comp, def));
    CHECK_EQ(seller.purse(), 50 * kGoldUnit);
}

TEST(CanOfferRealignAndOptions) {
    // Realign is offered only at IdLevel 0 (Completely).
    Companion comp;
    comp.alignment = CompanionAlignment::Good;

    comp.idLevel = OwnerIdLevel::Completely;
    CHECK(canOfferRealign(comp));

    comp.idLevel = OwnerIdLevel::Mostly;
    CHECK(!canOfferRealign(comp));

    comp.idLevel = OwnerIdLevel::Partially;
    CHECK(!canOfferRealign(comp));

    // Options are the two alignments the companion does not have.
    auto opts = realignOptions(comp);
    const CompanionAlignment o0 = opts[0];
    const CompanionAlignment o1 = opts[1];
    CHECK(o0 != CompanionAlignment::Good);
    CHECK(o1 != CompanionAlignment::Good);
    CHECK(o0 != o1);
    CHECK((o0 == CompanionAlignment::Neutral && o1 == CompanionAlignment::Evil) ||
          (o0 == CompanionAlignment::Evil && o1 == CompanionAlignment::Neutral));
}
