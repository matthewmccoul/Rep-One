#include "test_support.h"
#include "mordor/items3a.h"

#include <cstring>

using namespace mordor;
using tsup::Env;

static bool sameText(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
}

// ---- Store fee: round_half_even(0.8 * Price / 0.5) = round_half_even(1.6 * Price)

TEST(storeFeeRoundsOnePointSixTimesPriceHalfEven) {
    CHECK_EQ(storeFeeFromPrice(0), 0);
    CHECK_EQ(storeFeeFromPrice(1), 2);     // 1.6 -> 2
    CHECK_EQ(storeFeeFromPrice(2), 3);     // 3.2 -> 3
    CHECK_EQ(storeFeeFromPrice(10), 16);
    CHECK_EQ(storeFeeFromPrice(15), 24);   // 24.0 exactly
    CHECK_EQ(storeFeeFromPrice(100), 160);
}

// ---- Shelf counts: Align(0..3) = counts for {U, G, N, E}

TEST(shelfCountForMapsUnalignedGoodNeutralEvilToCounts) {
    Shelf s;
    s.alignCounts[0] = 4;
    s.alignCounts[1] = 5;
    s.alignCounts[2] = 6;
    s.alignCounts[3] = 7;
    CHECK_EQ(shelfCountFor(s, ItemAlign::Unaligned), 4);
    CHECK_EQ(shelfCountFor(s, ItemAlign::Good), 5);
    CHECK_EQ(shelfCountFor(s, ItemAlign::Neutral), 6);
    CHECK_EQ(shelfCountFor(s, ItemAlign::Evil), 7);
}

// ---- Floor price line: only floor 0 with unaligned stock shows list price

TEST(floorStocksUnalignedNeedsFloorZeroAndPositiveUnalignedCount) {
    Shelf s;
    s.floor = 0;
    s.alignCounts[0] = 3;
    CHECK(floorStocksUnaligned(s));

    s.alignCounts[0] = 0;
    CHECK(!floorStocksUnaligned(s));

    s.alignCounts[0] = 3;
    s.floor = 1;
    CHECK(!floorStocksUnaligned(s));
}

// ---- Free will: BindingLevel < 0 cannot be bound

TEST(isFreeWillOnlyWhenBindingLevelIsNegative) {
    Companion c;             // default bindingLevel is -10
    CHECK(isFreeWill(c));
    c.bindingLevel = -1;
    CHECK(isFreeWill(c));
    c.bindingLevel = 0;
    CHECK(!isFreeWill(c));
    c.bindingLevel = 1;
    CHECK(!isFreeWill(c));
}

// ---- bindingTarget: min(level + step, kBindingMax), kBindingMax = 100

TEST(bindingTargetAddsStepAndCapsAtBindingMax) {
    CHECK_EQ(bindingTarget(0, 1), 1);
    CHECK_EQ(bindingTarget(50, 5), 55);
    CHECK_EQ(bindingTarget(90, 10), 100);   // exactly the cap
    CHECK_EQ(bindingTarget(95, 5), 100);    // exactly the cap
    CHECK_EQ(bindingTarget(95, 10), 100);   // 105 clamps to 100
    CHECK_EQ(bindingTarget(99, 1), 100);
    CHECK_EQ(bindingTarget(98, 2), 100);
}

// ---- Stat items: need 10 Wisdom and 10 Intelligence

TEST(meetsStatItemRequirementsAcceptsTenWisAndTenInt) {
    Character c;
    c.abilities.stats[static_cast<u32>(StatIndex::Wis)] = 10;
    c.abilities.stats[static_cast<u32>(StatIndex::Int)] = 10;
    CHECK(meetsStatItemRequirements(c));
}

TEST(meetsStatItemRequirementsRejectsNineInEitherStat) {
    Character c;
    c.abilities.stats[static_cast<u32>(StatIndex::Wis)] = 10;
    c.abilities.stats[static_cast<u32>(StatIndex::Int)] = 9;
    CHECK(!meetsStatItemRequirements(c));

    c.abilities.stats[static_cast<u32>(StatIndex::Wis)] = 9;
    c.abilities.stats[static_cast<u32>(StatIndex::Int)] = 10;
    CHECK(!meetsStatItemRequirements(c));
}

// ---- payFromPurse: hand first, rest from bank; refusal if hand + bank < amount

TEST(payFromPurseTakesFromHandThenSpillsToBank) {
    // Currency is i64 in 1/10000 gold, so whole-gold amounts are scaled by 10000.
    Purse p;
    p.goldOnHand = 1000 * 10000;
    p.goldInBank = 500 * 10000;

    CHECK(payFromPurse(p, 300 * 10000));          // hand covers it
    CHECK_EQ(p.goldOnHand, 700 * 10000);
    CHECK_EQ(p.goldInBank, 500 * 10000);

    CHECK(payFromPurse(p, 900 * 10000));          // rest = 900 - 700 = 200 gold from bank (R: CLng(amount - hand))
    CHECK_EQ(p.goldOnHand, 0);
    CHECK_EQ(p.goldInBank, 300 * 10000);
}

TEST(payFromPurseExactTotalSucceedsAndOneOverIsRefused) {
    // Currency is i64 in 1/10000 gold, so whole-gold amounts are scaled by 10000.
    Purse q;
    q.goldOnHand = 1000 * 10000;
    q.goldInBank = 500 * 10000;

    CHECK(!payFromPurse(q, 1501 * 10000));        // refused: one gold over hand + bank
    CHECK_EQ(q.goldOnHand, 1000 * 10000);         // unchanged
    CHECK_EQ(q.goldInBank, 500 * 10000);

    CHECK(payFromPurse(q, 1500 * 10000));         // exactly hand + bank
    CHECK_EQ(q.goldOnHand, 0);
    CHECK_EQ(q.goldInBank, 0);
}

// ---- drawTheftSlot: Int(Rnd * 39) + 2, so slots 2..40; slot 40 is one past the pack

TEST(drawTheftSlotRangeStartsAtTwoAndReachesFortyScripted) {
    Env low({0.0, 0.0});
    CHECK_EQ(drawTheftSlot(low.ctx), 2);

    Env mid({0.5, 0.9});
    CHECK_EQ(drawTheftSlot(mid.ctx), 21);      // Int(19.5) + 2

    Env high({0.99, 0.9});
    CHECK_EQ(drawTheftSlot(high.ctx), 40);     // Int(38.61) + 2
}

TEST(drawTheftSlotBoundaryBetweenSlotTwoAndThree) {
    Env below({0.0256, 0.9});                  // 0.9984 -> 0 -> slot 2
    CHECK_EQ(drawTheftSlot(below.ctx), 2);

    Env above({0.0257, 0.9});                  // 1.0023 -> 1 -> slot 3
    CHECK_EQ(drawTheftSlot(above.ctx), 3);
}

// ---- Guild crest tables (crest rows indexed by Guild)

TEST(crestPaladinRowMatchesSrdTable) {
    const i32 p = static_cast<i32>(Guild::Paladin);
    const i32 expectMods[kStatCount] = {2, 0, 0, 1, 2, 1};
    CHECK_EQ(kCrestAttack[p], 6);
    CHECK_EQ(kCrestDefence[p], 12);
    CHECK_EQ(kCrestAbilityMask[p], 4u);
    for (i32 s = 0; s < kStatCount; ++s) {
        CHECK_EQ(kCrestMods[p][s], expectMods[s]);
    }
    CHECK(sameText(kCrestSpellName[p], "Minor Heal"));
}

TEST(crestMageAndWizardRowsMatchSrdTable) {
    const i32 m = static_cast<i32>(Guild::Mage);
    const i32 mageMods[kStatCount] = {0, 2, 2, 0, 2, 0};
    CHECK_EQ(kCrestAttack[m], 0);              // attack 0 is a real value here
    CHECK_EQ(kCrestDefence[m], 12);
    CHECK_EQ(kCrestAbilityMask[m], 0xAu);
    for (i32 s = 0; s < kStatCount; ++s) {
        CHECK_EQ(kCrestMods[m][s], mageMods[s]);
    }
    CHECK(sameText(kCrestSpellName[m], "Soul Entrapment"));

    const i32 w = static_cast<i32>(Guild::Wizard);
    const i32 wizardMods[kStatCount] = {0, 4, 2, 0, 0, 0};
    CHECK_EQ(kCrestAttack[w], 3);
    CHECK_EQ(kCrestDefence[w], 12);
    CHECK_EQ(kCrestAbilityMask[w], 0xEu);
    for (i32 s = 0; s < kStatCount; ++s) {
        CHECK_EQ(kCrestMods[w][s], wizardMods[s]);
    }
    CHECK(sameText(kCrestSpellName[w], "Power"));
}

TEST(crestNinjaHasHighestDefenceAndNoSpell) {
    const i32 n = static_cast<i32>(Guild::Ninja);
    CHECK_EQ(kCrestDefence[n], 18);
    for (i32 g = 0; g < kGuildCount; ++g) {
        CHECK(kCrestDefence[g] <= 18);
    }
    CHECK(sameText(kCrestSpellName[n], ""));
}

TEST(crestSpellNamesBlankForNomadWarriorNinjaAndShareCharm) {
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Nomad)], ""));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Warrior)], ""));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Ninja)], ""));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Thief)], "Charm of Opening"));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Scavenger)], "Charm of Opening"));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Healer)], "Restoration"));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Seeker)], "Displacement"));
}

// ---- bindCompanion: refusals leave state and purse untouched

TEST(bindCompanionRefusesFreeWillWithoutCharging) {
    Env env({0.5, 0.9});
    Companion comp;
    comp.bindingLevel = -5;
    Purse purse;
    purse.goldOnHand = 1000000;
    purse.goldInBank = 0;

    (void)bindCompanion(env.ctx, comp, purse, 5, 5);

    CHECK_EQ(comp.bindingLevel, -5);
    CHECK_EQ(purse.goldOnHand, 1000000);
    CHECK_EQ(purse.goldInBank, 0);
}

TEST(bindCompanionRefusesAlreadyBoundAtMaxWithoutCharging) {
    Env env({0.5, 0.9});
    Companion comp;
    comp.bindingLevel = 100;
    Purse purse;
    purse.goldOnHand = 1000000;

    (void)bindCompanion(env.ctx, comp, purse, 5, 5);

    CHECK_EQ(comp.bindingLevel, 100);
    CHECK_EQ(purse.goldOnHand, 1000000);
}

TEST(bindCompanionRaisesByStepAndPaysBindingCharge) {
    Env env({0.5, 0.9});
    Companion comp;
    comp.bindingLevel = 10;
    const Currency start = 1000000000000;
    Purse purse;
    purse.goldOnHand = start;
    purse.goldInBank = 0;

    const i64 pay = bindingCharge(bindingCost(5, 10));
    (void)bindCompanion(env.ctx, comp, purse, 5, 5);

    CHECK_EQ(comp.bindingLevel, 15);
    CHECK_EQ(purse.goldOnHand, start - pay);
    CHECK_EQ(purse.goldInBank, 0);
}

TEST(bindCompanionCapsNewBindingAtBindingMax) {
    Env env({0.5, 0.9});
    Companion comp;
    comp.bindingLevel = 98;
    Purse purse;
    purse.goldOnHand = 1000000000000;

    (void)bindCompanion(env.ctx, comp, purse, 5, 5);

    CHECK_EQ(comp.bindingLevel, 100);
}

// ---- Item helpers: guild marker boundary and no-spell index

TEST(guildItemMarkerBoundaryAtFiveHundred) {
    Item it;
    it.ailments = 500;
    CHECK(it.isGuildItem());
    CHECK_EQ(it.guildIndex(), 0);

    it.ailments = 499;
    CHECK(!it.isGuildItem());

    it.ailments = 503;
    CHECK(it.isGuildItem());
    CHECK_EQ(it.guildIndex(), 3);
}

TEST(itemSpellIndexIsMinusOneWhenItemHasNoSpell) {
    Env env({0.5, 0.9});
    Item it;
    it.spellId = 0;
    CHECK_EQ(itemSpellIndex(env.ctx, it), -1);
}

// ---- Alignment bitmask constants (wiki 0x28 field)

TEST(alignmentBitConstantsMatchWikiMask) {
    CHECK_EQ(static_cast<int>(kAlignBitUnaligned), 0x01);
    CHECK_EQ(static_cast<int>(kAlignBitGood), 0x02);
    CHECK_EQ(static_cast<int>(kAlignBitNeutral), 0x04);
    CHECK_EQ(static_cast<int>(kAlignBitEvil), 0x08);
    const int all = kAlignBitUnaligned | kAlignBitGood | kAlignBitNeutral | kAlignBitEvil;
    CHECK_EQ(all, 0x0F);
}
